import * as vscode from 'vscode';

import {ClangdContext} from './clangd-context';
import * as ConfigWatcher from './config-watcher';

/**
 *  This method is called when the extension is activated. The extension is
 *  activated the very first time a command is executed.
 */
export async function activate(context: vscode.ExtensionContext) {
  const outputChannel = vscode.window.createOutputChannel('clangd');
  context.subscriptions.push(outputChannel);

  const clangdContext = new ClangdContext;
  context.subscriptions.push(clangdContext);

  // Listen for settings changes
  ConfigWatcher.activate(context);

  // An empty place holder for the activate command, otherwise we'll get an
  // "command is not registered" error.
  context.subscriptions.push(
      vscode.commands.registerCommand('clangd.activate', async () => {}));
  context.subscriptions.push(
      vscode.commands.registerCommand('clangd.restart', async () => {
        await clangdContext.dispose();
        await clangdContext.activate(context.globalStoragePath, outputChannel);
      }));

  await clangdContext.activate(context.globalStoragePath, outputChannel);
}
