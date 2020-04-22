import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient';

export function activate(client : vscodelc.LanguageClient, context: vscode.ExtensionContext) {
    context.subscriptions.push(vscode.commands.registerCommand(
        'clangd.switchheadersource', () => switchSourceHeader(client)));    
}

namespace SwitchSourceHeaderRequest {
    export const type =
        new vscodelc.RequestType<vscodelc.TextDocumentIdentifier, string | undefined,
            void, void>('textDocument/switchSourceHeader');
}

async function switchSourceHeader(client: vscodelc.LanguageClient): Promise<void> {
    const uri =
        vscode.Uri.file(vscode.window.activeTextEditor.document.fileName);
    if (!uri) return;

    const docIdentifier =
        vscodelc.TextDocumentIdentifier.create(uri.toString());
    const sourceUri = await client.sendRequest(
        SwitchSourceHeaderRequest.type, docIdentifier);
    if (!sourceUri) return;
    const doc = await vscode.workspace.openTextDocument(
        vscode.Uri.parse(sourceUri));
    vscode.window.showTextDocument(doc);
}