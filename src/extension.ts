import * as vscode from 'vscode';

import {ClangdContext} from './clangd-context';

let clangdContext: ClangdContext|null;

async function startClient(globalStoragePath: string,
                           outputChannel: vscode.OutputChannel) {
  clangdContext = new ClangdContext;
  clangdContext.activate(globalStoragePath, outputChannel);
}

async function stopClient() {
  await clangdContext?.dispose();
  clangdContext = null;
}

/**
 *  This method is called when the extension is activated. The extension is
 *  activated the very first time a command is executed.
 */
export async function activate(context: vscode.ExtensionContext) {
  const outputChannel =
      vscode.window.createOutputChannel('Clangd Language Server');
  context.subscriptions.push(outputChannel);

  // An empty place holder for the activate command, otherwise we'll get an
  // "command is not registered" error.
  context.subscriptions.push(
      vscode.commands.registerCommand('clangd.activate', async () => {}));
  context.subscriptions.push(
      vscode.commands.registerCommand('clangd.restart', async () => {
        await stopClient();
        await startClient(context.globalStoragePath, outputChannel);
      }));

  startClient(context.globalStoragePath, outputChannel);
}

export function deactivate() { stopClient(); }
