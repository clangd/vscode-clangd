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

interface FileEntry {
  document: vscode.TextDocument;

  // Last applied decorations.
  cachedDecorations: Map<string, vscode.DecorationOptions[]>|null;

  // Source of the token to cancel in-flight inlay hints request if any.
  inlaysRequest: vscode.CancellationTokenSource|null;
}

const enabledSetting = 'editor.inlayHints.enabled';

class InlayHintsFeature implements vscodelc.StaticFeature {
  private enabled = false;
  private sourceFiles = new Map<string, FileEntry>(); // keys are URIs
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
      vscode.workspace.onDidChangeConfiguration(e => {
        if (e.affectsConfiguration(enabledSetting))
          this.checkEnabled()
      });
      this.checkEnabled();
    }
  }

  checkEnabled() {
    const enabled = vscode.workspace.getConfiguration().get<boolean>(enabledSetting, false);
    if (enabled == this.enabled)
      return;
    this.enabled = enabled;
    enabled ? this.startShowingHints() : this.stopShowingHints();
  }

  onDidChangeVisibleTextEditors() {
    if (!this.enabled)
      return;

    const newSourceFiles = new Map<string, FileEntry>();

    // Rerender all, even up-to-date editors for simplicity
    this.context.visibleClangdEditors.forEach(async editor => {
      const uri = editor.document.uri.toString();
      const file = this.sourceFiles.get(uri) ?? {
        document: editor.document,
        cachedDecorations: null,
        inlaysRequest: null
      };
      newSourceFiles.set(uri, file);

      // No text documents changed, so we may try to use the cache
      if (!file.cachedDecorations) {
        const hints = await this.fetchHints(file);
        if (!hints)
          return;

        file.cachedDecorations = hintsToDecorations(
            hints, this.context.client.protocol2CodeConverter);
      }

      this.renderDecorations(editor, file.cachedDecorations);
    });

    // Cancel requests for no longer visible (disposed) source files
    this.sourceFiles.forEach((file, uri) => {
      if (!newSourceFiles.has(uri)) {
        file.inlaysRequest?.cancel();
      }
    });

    this.sourceFiles = newSourceFiles;
  }

  onDidChangeTextDocument({contentChanges,
                           document}: vscode.TextDocumentChangeEvent) {
    if (!this.enabled || contentChanges.length === 0 ||
        !isClangdDocument(document))
      return;
    this.update(document.uri.toString());
  }

  dispose() { this.stopShowingHints(); }

  private startShowingHints() {
    vscode.window.onDidChangeVisibleTextEditors(
        this.onDidChangeVisibleTextEditors, this, this.disposables);
    vscode.workspace.onDidChangeTextDocument(this.onDidChangeTextDocument, this,
                                             this.disposables);

    // Set up initial cache shape
    this.context.visibleClangdEditors.forEach(
        editor => this.sourceFiles.set(editor.document.uri.toString(), {
          document: editor.document,
          inlaysRequest: null,
          cachedDecorations: null
        }));

    for (const uri of this.sourceFiles.keys())
      this.update(uri);
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

  private update(uri: string) {
    const file = this.sourceFiles.get(uri);
    if (!file)
      return;
    this.fetchHints(file).then(hints => {
      if (!hints)
        return;

      file.cachedDecorations =
          hintsToDecorations(hints, this.context.client.protocol2CodeConverter);

      for (const editor of this.context.visibleClangdEditors) {
        if (editor.document.uri.toString() == uri) {
          this.renderDecorations(editor, file.cachedDecorations);
        }
      }
    });
  }

  private async fetchHints(file: FileEntry): Promise<InlayHint[]|null> {
    file.inlaysRequest?.cancel();

    const tokenSource = new vscode.CancellationTokenSource();
    file.inlaysRequest = tokenSource;

    const request = {textDocument: {uri: file.document.uri.toString()}};

    return this.context.client.sendRequest(InlayHintsRequest.type, request,
                                           tokenSource.token);
  }
}
