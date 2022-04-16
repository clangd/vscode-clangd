import * as vscode from 'vscode';
import * as lc from 'vscode-languageclient';

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
        clangdContext.dispose();
        await clangdContext.activate(context.globalStorageUri.path,
                                     outputChannel, context.workspaceState);
      }));

  await clangdContext.activate(context.globalStorageUri.path, outputChannel,
                               context.workspaceState);

  context.subscriptions.push(vscode.commands.registerCommand(
      'clangd.action.showReferences',
      async (argument: {
        uri: string; position: lc.Position; locations: lc.Location[];
      }) => {
        const client = clangdContext.client;
        if (client) {
          await vscode.commands.executeCommand(
              'editor.action.showReferences',
              vscode.Uri.parse(argument.uri),
              client.protocol2CodeConverter.asPosition(argument.position),
              argument.locations.map(client.protocol2CodeConverter.asLocation),
          );
        }
      }));

  const shouldCheck = vscode.workspace.getConfiguration('clangd').get(
      'detectExtensionConflicts');
  if (shouldCheck) {
    const interval = setInterval(function() {
      const cppTools = vscode.extensions.getExtension('ms-vscode.cpptools');
      if (cppTools && cppTools.isActive) {
        const cppToolsConfiguration =
            vscode.workspace.getConfiguration('C_Cpp');
        const cppToolsEnabled = cppToolsConfiguration.get('intelliSenseEngine');
        if (cppToolsEnabled !== 'Disabled') {
          vscode.window
              .showWarningMessage(
                  'You have both the Microsoft C++ (cpptools) extension and ' +
                      'clangd extension enabled. The Microsoft IntelliSense features ' +
                      'conflict with clangd\'s code completion, diagnostics etc.',
                  'Disable IntelliSense', 'Never show this warning')
              .then(selection => {
                if (selection == 'Disable IntelliSense') {
                  cppToolsConfiguration.update(
                      'intelliSenseEngine', 'Disabled',
                      vscode.ConfigurationTarget.Global);
                } else if (selection == 'Never show this warning') {
                  vscode.workspace.getConfiguration('clangd').update(
                      'detectExtensionConflicts', false,
                      vscode.ConfigurationTarget.Global);
                  clearInterval(interval);
                }
              });
        }
      }
    }, 5000);
  }
}
