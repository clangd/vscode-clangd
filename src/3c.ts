import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';
import * as path from 'path';
import {ClangdContext} from './clangd-context';
interface _3CParams {
    textDocument: vscodelc.TextDocumentIdentifier;
}
interface Reply{
    details: string;
}
export const Run3CRequest = 
    new vscodelc.RequestType<_3CParams,Reply,void>('textDocument/run3c');
export function activate(
    context : ClangdContext) {

        let disposable = vscode.commands.registerTextEditorCommand(
            'clangd.run3c', async (editor,_edit) => {
                const converter = context.client.code2ProtocolConverter;
                
                const usage = 
                    await context.client.sendRequest(Run3CRequest, {
                        textDocument:
                            converter.asTextDocumentIdentifier(editor.document),
                    });
                vscode.window.showInformationMessage('Running 3C on this project');
                console.log('Running the 3C command');
                const result = usage.details;
                vscode.window.showInformationMessage(result);
                console.log(result);
            }

        );

        context.subscriptions.push(disposable);
    
}

