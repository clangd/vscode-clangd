import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContextManager} from './clangd-context-manager';

export function activate(manager: ClangdContextManager) {
  manager.subscriptions.push(vscode.commands.registerCommand(
      'clangd.switchheadersource', () => switchSourceHeader(manager)));
}

namespace SwitchSourceHeaderRequest {
export const type =
    new vscodelc
        .RequestType<vscodelc.TextDocumentIdentifier, string|undefined, void>(
            'textDocument/switchSourceHeader');
}

async function switchSourceHeader(manager: ClangdContextManager):
    Promise<void> {
  const editor = vscode.window.activeTextEditor;
  if (!editor)
    return;

  const context = manager.getContextForDocument(editor.document);
  if (!context) {
    vscode.window.showInformationMessage(
        'No clangd instance available for this document');
    return;
  }

  const uri = vscode.Uri.file(editor.document.fileName);

  const docIdentifier = vscodelc.TextDocumentIdentifier.create(uri.toString());
  const sourceUri = await context.client.sendRequest(
      SwitchSourceHeaderRequest.type, docIdentifier);
  if (!sourceUri) {
    vscode.window.showInformationMessage('Didn\'t find a corresponding file.');
    return;
  }
  const doc =
      await vscode.workspace.openTextDocument(vscode.Uri.parse(sourceUri));
  vscode.window.showTextDocument(doc);
}