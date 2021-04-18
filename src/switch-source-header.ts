import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext} from './clangd-context';

export function activate(context: ClangdContext) {
  context.subscriptions.push(vscode.commands.registerCommand(
      'clangd.switchheadersource', () => switchSourceHeader(context.client)));
}

namespace SwitchSourceHeaderRequest {
export const type =
    new vscodelc
        .RequestType<vscodelc.TextDocumentIdentifier, string|undefined, void>(
            'textDocument/switchSourceHeader');
}

async function switchSourceHeader(client: vscodelc.LanguageClient):
    Promise<void> {
  if (!vscode.window.activeTextEditor)
    return;
  const uri = vscode.Uri.file(vscode.window.activeTextEditor.document.fileName);

  const docIdentifier = vscodelc.TextDocumentIdentifier.create(uri.toString());
  const sourceUri =
      await client.sendRequest(SwitchSourceHeaderRequest.type, docIdentifier);
  if (!sourceUri) {
    vscode.window.showInformationMessage('Didn\'t find a corresponding file.');
    return;
  }
  const doc =
      await vscode.workspace.openTextDocument(vscode.Uri.parse(sourceUri));
  vscode.window.showTextDocument(doc);
}