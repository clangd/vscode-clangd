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
                if (!vscode.workspace.workspaceFolders) {
                    vscode.window.showInformationMessage("Open a folder/workspace first");
                    return;
                }
                const backupURI = vscode.Uri.parse(vscode.workspace.workspaceFolders[0].uri.path+'/backup');
                const originalURI = vscode.workspace.workspaceFolders[0].uri.path+'/backup/';
                vscode.workspace.fs.createDirectory(backupURI);
                const files = await vscode.workspace.findFiles('**/*.*');
                files.forEach(function(file){
                    const fileName = file.path.substring(file.path.lastIndexOf("/")+1);
                    const FileURI = vscode.Uri.parse(originalURI+'/backup/'+fileName);
                    vscode.workspace.fs.copy(file,FileURI,{overwrite:true});
                });
                const usage = 
                    await context.client.sendRequest(Run3CRequest, {
                        textDocument:
                            converter.asTextDocumentIdentifier(editor.document),
                    });
                vscode.window.showInformationMessage('Running 3C');
                console.log('Running the 3C command');
                const result = usage.details;
                vscode.window.showInformationMessage(result);
                console.log(result);
            }

        );

        context.subscriptions.push(disposable);
    
}

