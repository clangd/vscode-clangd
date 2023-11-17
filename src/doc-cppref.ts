import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext} from './clangd-context';

export function activate(context: ClangdContext) {
  const feature = new DocCppReferenceFeature(context);
  context.client.registerFeature(feature);
}

namespace protocol {

export interface SymbolInfo {
  name: string;
  containerName: string;
  usr: string;
  id?: string;
}

export interface SymbolInfoParams {
  textDocument: vscodelc.TextDocumentIdentifier;
  position: vscodelc.Position;
}

export namespace SymbolInfoRequest {
export const type =
    new vscodelc.RequestType<SymbolInfoParams, SymbolInfo[], void>(
        'textDocument/symbolInfo');
}

} // namespace protocol

class DocCppReferenceFeature implements vscodelc.StaticFeature {
  private url = 'https://duckduckgo.com/?q=!ducky+site:cppreference.com';
  private commandRegistered = false;

  constructor(private readonly context: ClangdContext) {}

  fillClientCapabilities(_capabilities: vscodelc.ClientCapabilities) {}
  fillInitializeParams(_params: vscodelc.InitializeParams) {}

  async getSymbolsUnderCursor(): Promise<Array<string>> {
    const editor = vscode.window.activeTextEditor;
    if (!editor)
      return [];

    const document = editor.document;
    const position = editor.selection.active;
    const request: protocol.SymbolInfoParams = {
      textDocument: {uri: document.uri.toString()},
      position: position,
    };

    const reply = await this.context.client.sendRequest(
        protocol.SymbolInfoRequest.type, request);

    let result: Array<string> = [];
    reply.forEach(symbol => {
      if (symbol.name === null || symbol.name === undefined)
        return;

      if (symbol.containerName === null || symbol.containerName === undefined) {
        result.push(symbol.name);
        return;
      }

      const needComas =
          !symbol.containerName.endsWith('::') && !symbol.name.startsWith('::');
      result.push(symbol.containerName + (needComas ? '::' : '') + symbol.name);
    });

    return result;
  }

  async openDocumentation() {
    const symbols = await this.getSymbolsUnderCursor();
    symbols.forEach(symbol => {
      if (!symbol.startsWith('std::'))
        return;

      vscode.commands.executeCommand('vscode.open',
                                     vscode.Uri.parse(this.url + '+' + symbol));
    });
  }

  initialize(capabilities: vscodelc.ServerCapabilities,
             _documentSelector: vscodelc.DocumentSelector|undefined) {
    if (this.commandRegistered)
      return;
    this.commandRegistered = true;

    this.context.subscriptions.push(vscode.commands.registerCommand(
        'clangd.openDocumentation', this.openDocumentation, this));
  }
  getState(): vscodelc.FeatureState { return {kind: 'static'}; }
  dispose() {}
}