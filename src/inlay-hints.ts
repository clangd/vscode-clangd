// This file implements the client side of the proposed inlay hints
// extension to LSP. The proposal is based on the one at
// https://github.com/microsoft/language-server-protocol/issues/956,
// with some modifications that reflect discussions in that issue.
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

// Currently, only one hint kind (parameter hints) are supported,
// but others (e.g. type hints) may be added in the future.
enum InlayHintKind {
  Parameter = 'parameter',
  Type = 'type'
}

interface InlayHint {
  range: vscodelc.Range;
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

interface InlayDecorations {
  // Hints are grouped based on their InlayHintKind, because different kinds
  // require different decoration types.
  // A future iteration of the API may have free-form hint kinds, and instead
  // specify style-related information (e.g. before vs. after) explicitly.
  // With such an API, we could group hints based on unique presentation styles
  // instead.
  parameterHints: vscode.DecorationOptions[];
  typeHints: vscode.DecorationOptions[];
}

interface HintStyle {
  decorationType: vscode.TextEditorDecorationType;

  toDecoration(hint: InlayHint,
               conv: vscodelc.Protocol2CodeConverter): vscode.DecorationOptions;
}

const parameterHintStyle = createHintStyle('before');
const typeHintStyle = createHintStyle('after');

function createHintStyle(position: 'before'|'after'): HintStyle {
  const fg = new vscode.ThemeColor('clangd.inlayHints.foreground');
  const bg = new vscode.ThemeColor('clangd.inlayHints.background');
  return {
    decorationType: vscode.window.createTextEditorDecorationType({
      [position]: {
        color: fg,
        backgroundColor: bg,
        fontStyle: 'normal',
        fontWeight: 'normal',
        textDecoration: ';font-size:smaller'
      }
    }),
    toDecoration(hint: InlayHint, conv: vscodelc.Protocol2CodeConverter):
        vscode.DecorationOptions {
          return {
            range: conv.asRange(hint.range),
            renderOptions: {[position]: {contentText: hint.label}}
          };
        }
  };
}

interface FileEntry {
  document: vscode.TextDocument;

  // Last applied decorations.
  cachedDecorations: InlayDecorations|null;

  // Source of the token to cancel in-flight inlay hints request if any.
  inlaysRequest: vscode.CancellationTokenSource|null;
}

class InlayHintsFeature implements vscodelc.StaticFeature {
  private enabled = false;
  private sourceFiles = new Map<string, FileEntry>(); // keys are URIs
  private readonly disposables: vscode.Disposable[] = [];

  constructor(private readonly context: ClangdContext) {}

  fillClientCapabilities(_capabilities: vscodelc.ClientCapabilities) {}
  fillInitializeParams(_params: vscodelc.InitializeParams) {}

  initialize(capabilities: vscodelc.ServerCapabilities,
             _documentSelector: vscodelc.DocumentSelector|undefined) {
    const serverCapabilities: vscodelc.ServerCapabilities&
        {clangdInlayHintsProvider?: boolean} = capabilities;
    if (serverCapabilities.clangdInlayHintsProvider) {
      this.enabled = true;
      this.startShowingHints();
    }
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

        file.cachedDecorations = this.hintsToDecorations(hints);
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

    for (const uri in this.sourceFiles.keys)
      this.update(uri);
  }

  private stopShowingHints() {
    this.sourceFiles.forEach(file => file.inlaysRequest?.cancel());
    this.context.visibleClangdEditors.forEach(
        editor => this.renderDecorations(editor,
                                         {parameterHints: [], typeHints: []}));
    this.disposables.forEach(d => d.dispose());
  }

  private renderDecorations(editor: vscode.TextEditor,
                            decorations: InlayDecorations) {
    editor.setDecorations(parameterHintStyle.decorationType,
                          decorations.parameterHints);
    editor.setDecorations(typeHintStyle.decorationType, decorations.typeHints);
  }

  private hintsToDecorations(hints: InlayHint[]): InlayDecorations {
    const decorations: InlayDecorations = {parameterHints: [], typeHints: []};
    const conv = this.context.client.protocol2CodeConverter;
    for (const hint of hints) {
      switch (hint.kind) {
      case InlayHintKind.Parameter: {
        decorations.parameterHints.push(
            parameterHintStyle.toDecoration(hint, conv));
        continue;
      }
      case InlayHintKind.Type: {
        decorations.typeHints.push(typeHintStyle.toDecoration(hint, conv));
        continue;
      }
        // Don't handle unknown hint kinds because we don't know how to style
        // them. This may change in a future version of the protocol.
      }
    }
    return decorations;
  }

  private update(uri: string) {
    const file = this.sourceFiles.get(uri);
    if (!file)
      return;
    this.fetchHints(file).then(hints => {
      if (!hints)
        return;

      file.cachedDecorations = this.hintsToDecorations(hints);

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