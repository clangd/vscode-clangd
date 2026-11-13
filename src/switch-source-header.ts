import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {contextForCommand} from './clangd-context';
import type {ClangdContextManager} from './clangd-context-manager';

export function registerCommands(manager: ClangdContextManager):
    vscode.Disposable[] {
  return [vscode.commands.registerCommand('clangd.switchheadersource',
                                          () => switchSourceHeader(manager))];
}

namespace SwitchSourceHeaderRequest {
export const type =
    new vscodelc
        .RequestType<vscodelc.TextDocumentIdentifier, string|undefined, void>(
            'textDocument/switchSourceHeader');
}

async function switchSourceHeader(manager: ClangdContextManager):
    Promise<void> {
  if (!vscode.window.activeTextEditor)
    return;
  const context =
      contextForCommand(manager, vscode.window.activeTextEditor.document.uri);
  if (!context)
    return;
  const uri = vscode.Uri.file(vscode.window.activeTextEditor.document.fileName);

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