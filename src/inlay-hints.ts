// This file implements the client side of clangd's inlay hints
// extension to LSP: https://clangd.llvm.org/extensions#inlay-hints
//
// The feature allows the server to provide the client with inline
// annotations to display for e.g. parameter names at call sites.
// The client-side implementation is adapted from rust-analyzer's.

import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext, isClangdDocument} from './clangd-context';

export function activate(context: ClangdContext) {
  const feature = new InlayHintsFeature(context);
  context.client.registerFeature(feature);
}

/// Protocol ///

enum InlayHintKind {
  Parameter = 'parameter',
  Type = 'type'
}

interface InlayHint {
  range: vscodelc.Range;
  position?: vscodelc.Position; // omitted by old servers, see hintSide.
  kind: InlayHintKind|string;
  label: string;
}

interface InlayHintsParams {
  textDocument: vscodelc.TextDocumentIdentifier;
}

namespace InlayHintsRequest {
export const type =
    new vscodelc.RequestType<InlayHintsParams, InlayHint[], void>(
        'clangd/inlayHints');
}

/// Conversion to VSCode decorations ///

function hintsToDecorations(hints: InlayHint[],
                            conv: vscodelc.Protocol2CodeConverter) {
  var result = new Map<string, vscode.DecorationOptions[]>();
  for (const hint of hints) {
    if (!result.has(hint.kind))
      result.set(hint.kind, []);
    result.get(hint.kind)!.push({
      range: conv.asRange(hint.range),
      renderOptions: {[hintSide(hint)]: {contentText: hint.label}}
    })
  }
  return result;
}

function positionEq(a: vscodelc.Position, b: vscodelc.Position) {
  return a.character == b.character && a.line == b.line;
}

function hintSide(hint: InlayHint): 'before'|'after' {
  // clangd only sends 'position' values that are one of the range endpoints.
  if (hint.position) {
    if (positionEq(hint.position, hint.range.start))
      return 'before';
    if (positionEq(hint.position, hint.range.end))
      return 'after';
  }
  // An earlier version of clangd's protocol sent range, but not position.
  // The kind should determine whether the position is the start or end.
  // These kinds were emitted at the time.
  if (hint.kind == InlayHintKind.Parameter)
    return 'before';
  if (hint.kind == InlayHintKind.Type)
    return 'after';
  // This shouldn't happen: old servers shouldn't send any other kinds.
  return 'before';
}

function decorationType(_kind: string) {
  // FIXME: kind ignored for now.
  const fg = new vscode.ThemeColor('clangd.inlayHints.foreground');
  const bg = new vscode.ThemeColor('clangd.inlayHints.background');
  const attachmentOpts = {
    color: fg,
    backgroundColor: bg,
    fontStyle: 'normal',
    fontWeight: 'normal',
    textDecoration: ';font-size:smaller',
  };
  return vscode.window.createTextEditorDecorationType(
      {before: attachmentOpts, after: attachmentOpts});
}

/// Feature state and triggering.

// Inlay information for an individual file.
// - created when the file is first made visible (foreground)
// - updated when the file is edited or made visible again
// - destroyed when the file is closed
interface FileState {
  // Last applied decorations.
  cachedDecorations: Map<string, vscode.DecorationOptions[]>|null;
  cachedDecorationsVersion: number|null;

  // Source of the token to cancel in-flight inlay hints request if any.
  inlaysRequest: vscode.CancellationTokenSource|null;
}

const enabledSetting = 'editor.inlayHints.enabled';

class InlayHintsFeature implements vscodelc.StaticFeature {
  private enabled = false;
  private commandRegistered = false;
  private sourceFiles = new Map<string, FileState>(); // keys are URIs
  private decorationTypes = new Map<string, vscode.TextEditorDecorationType>();
  private readonly disposables: vscode.Disposable[] = [];

  constructor(private readonly context: ClangdContext) {}

  fillClientCapabilities(_capabilities: vscodelc.ClientCapabilities) {}
  fillInitializeParams(_params: vscodelc.InitializeParams) {}

  initialize(capabilities: vscodelc.ServerCapabilities,
             _documentSelector: vscodelc.DocumentSelector|undefined) {
    const serverCapabilities: vscodelc.ServerCapabilities&
        {clangdInlayHintsProvider?: boolean} = capabilities;
    if (serverCapabilities.clangdInlayHintsProvider) {
      if (!this.commandRegistered) {
        // The command provides a quick way to toggle inlay hints
        // (key-bindable).
        // FIXME: this is a core VSCode setting, ideally they provide the
        // command. We toggle it globally, language-specific is nicer but
        // undiscoverable.
        this.commandRegistered = true;
        this.context.subscriptions.push(
            vscode.commands.registerCommand('clangd.inlayHints.toggle', () => {
              const current = vscode.workspace.getConfiguration().get<boolean>(
                  enabledSetting, false);
              vscode.workspace.getConfiguration().update(
                  enabledSetting, !current, vscode.ConfigurationTarget.Global);
            }));
      }
      vscode.workspace.onDidChangeConfiguration(e => {
        if (e.affectsConfiguration(enabledSetting))
          this.checkEnabled()
      });
      this.checkEnabled();
    }
  }

  checkEnabled() {
    const enabled =
        vscode.workspace.getConfiguration().get<boolean>(enabledSetting, false);
    if (enabled == this.enabled)
      return;
    this.enabled = enabled;
    enabled ? this.startShowingHints() : this.stopShowingHints();
  }

  onDidCloseTextDocument(document: vscode.TextDocument) {
    if (!this.enabled)
      return;

    // Drop state for any file that is now closed.
    const uri = document.uri.toString();
    const file = this.sourceFiles.get(uri);
    if (file) {
      file.inlaysRequest?.cancel();
      this.sourceFiles.delete(uri);
    }
  }

  onDidChangeVisibleTextEditors() {
    if (!this.enabled)
      return;

    // When an editor is made visible we have no inlay hints.
    // Obtain and render them, either from the cache or by issuing a request.
    // (This is redundant for already-visible editors, we could detect that).
    this.context.visibleClangdEditors.forEach(
        async editor => { this.update(editor.document); });
  }

  onDidChangeTextDocument({contentChanges,
                           document}: vscode.TextDocumentChangeEvent) {
    if (!this.enabled || contentChanges.length === 0 ||
        !isClangdDocument(document))
      return;
    this.update(document);
  }

  dispose() { this.stopShowingHints(); }

  private startShowingHints() {
    vscode.window.onDidChangeVisibleTextEditors(
        this.onDidChangeVisibleTextEditors, this, this.disposables);
    vscode.workspace.onDidCloseTextDocument(this.onDidCloseTextDocument, this,
                                            this.disposables);
    vscode.workspace.onDidChangeTextDocument(this.onDidChangeTextDocument, this,
                                             this.disposables);
    this.onDidChangeVisibleTextEditors();
  }

  private stopShowingHints() {
    this.sourceFiles.forEach(file => file.inlaysRequest?.cancel());
    this.context.visibleClangdEditors.forEach(
        editor => this.renderDecorations(editor, new Map()));
    this.disposables.forEach(d => d.dispose());
  }

  private renderDecorations(editor: vscode.TextEditor,
                            decorations:
                                Map<string, vscode.DecorationOptions[]>) {
    for (const kind of decorations.keys())
      if (!this.decorationTypes.has(kind))
        this.decorationTypes.set(kind, decorationType(kind));
    // Overwrite every kind we ever saw, so we are sure to clear stale hints.
    for (const [kind, type] of this.decorationTypes)
      editor.setDecorations(type, decorations.get(kind) || []);
  }

  // Update cached and displayed inlays for a document.
  private async update(document: vscode.TextDocument) {
    const uri = document.uri.toString();
    if (!this.sourceFiles.has(uri))
      this.sourceFiles.set(uri, {
        cachedDecorations: null,
        cachedDecorationsVersion: null,
        inlaysRequest: null
      });
    const file = this.sourceFiles.get(uri)!;

    // Invalidate the cache if the file content doesn't match.
    if (document.version != file.cachedDecorationsVersion)
      file.cachedDecorations = null;

    // Fetch inlays if we don't have them.
    if (!file.cachedDecorations) {
      const requestVersion = document.version;
      await this.fetchHints(uri, file).then(hints => {
        if (!hints)
          return;
        file.cachedDecorations = hintsToDecorations(
            hints, this.context.client.protocol2CodeConverter);
        file.cachedDecorationsVersion = requestVersion;
      });
    }

    // And apply them to the editor.
    for (const editor of this.context.visibleClangdEditors) {
      if (editor.document.uri.toString() == uri) {
        if (file.cachedDecorations &&
            file.cachedDecorationsVersion == editor.document.version)
          this.renderDecorations(editor, file.cachedDecorations);
      }
    }
  }

  private async fetchHints(uri: string,
                           file: FileState): Promise<InlayHint[]|null> {
    file.inlaysRequest?.cancel();

    const tokenSource = new vscode.CancellationTokenSource();
    file.inlaysRequest = tokenSource;

    const request = {textDocument: {uri}};

    return this.context.client.sendRequest(InlayHintsRequest.type, request,
                                           tokenSource.token);
  }
}
