import * as vscode from 'vscode';

import {ClangdContext} from './clangd-context';

/**
 *  This method is called when the extension is activated. The extension is
 *  activated the very first time a command is executed.
 */
export async function activate(context: vscode.ExtensionContext) {
  const outputChannel = vscode.window.createOutputChannel('clangd');
  context.subscriptions.push(outputChannel);

  const clangdContext = new ClangdContext;
  context.subscriptions.push(clangdContext);

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

  setInterval(function() {
    const cppTools = vscode.extensions.getExtension('ms-vscode.cpptools');
    if (cppTools && cppTools.isActive) {
      const cppToolsConfiguration = vscode.workspace.getConfiguration('C_Cpp');
      const cppToolsEnabled = cppToolsConfiguration.get('intelliSenseEngine');
      if (cppToolsEnabled !== 'Disabled') {
        vscode.window
            .showWarningMessage(
                'You have both the Microsoft C++ (cpptools) extension and ' +
                    'clangd extension enabled. The Microsoft IntelliSense features ' +
                    'conflict with clangd\'s code completion, diagnostics etc.',
                'Disable IntelliSense')
            .then(_ => {
              cppToolsConfiguration.update('intelliSenseEngine', 'Disabled',
                                           vscode.ConfigurationTarget.Global);
            });
      }
    }
  }, 5000);
}
