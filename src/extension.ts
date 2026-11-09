import * as vscode from 'vscode';

import {ClangdExtension} from '../api/vscode-clangd';

import {ClangdExtensionImpl} from './api';
import {ClangdContextManager} from './clangd-context-manager';
import {get, update} from './config';

/**
 *  This method is called when the extension is activated. The extension is
 *  activated the very first time a command is executed.
 */
export async function activate(context: vscode.ExtensionContext):
    Promise<ClangdExtension> {
  const outputChannel =
      vscode.window.createOutputChannel('clangd', {log: true});
  context.subscriptions.push(outputChannel);

  const manager =
      new ClangdContextManager(context.globalStoragePath, outputChannel);
  context.subscriptions.push(manager);

  context.subscriptions.push(
      vscode.commands.registerCommand('clangd.activate', async () => {
        if (manager.isStarting() || manager.isRunning()) {
          return;
        }
        vscode.commands.executeCommand('clangd.restart');
      }));
  context.subscriptions.push(vscode.commands.registerCommand(
      'clangd.restart', async (arg?: unknown) => {
        if (!get<boolean>('enable')) {
          vscode.window
              .showInformationMessage(
                  'Language features from Clangd are currently disabled. Would you like to enable them?',
                  'Enable', 'Close')
              .then(async (choice) => {
                if (choice === 'Enable') {
                  await update<boolean>('enable', true);
                  vscode.commands.executeCommand('clangd.restart');
                }
              });
          return;
        }

        // With a Uri (e.g. from the config file watcher), only the clangd
        // serving it restarts.
        await manager.restart(arg instanceof vscode.Uri ? arg : undefined);
      }));
  context.subscriptions.push(vscode.commands.registerCommand(
      'clangd.shutdown', async (arg?: unknown) => {
        await manager.stop(arg instanceof vscode.Uri ? arg : undefined);
      }));
  context.subscriptions.push(vscode.workspace.onDidChangeConfiguration((e) => {
    if (e.affectsConfiguration('clangd.enablePerFolderServer') &&
        vscode.workspace.getConfiguration('clangd').get<boolean>('enable'))
      vscode.commands.executeCommand('clangd.restart');
  }));

  let shouldCheck = false;

  if (vscode.workspace.getConfiguration('clangd').get<boolean>('enable')) {
    await manager.start();

    shouldCheck = vscode.workspace.getConfiguration('clangd').get<boolean>(
                      'detectExtensionConflicts') ??
                  false;
  }

  if (shouldCheck) {
    const interval = setInterval(function() {
      const cppTools = vscode.extensions.getExtension('ms-vscode.cpptools');
      if (cppTools && cppTools.isActive) {
        const cppToolsConfiguration =
            vscode.workspace.getConfiguration('C_Cpp');
        const cppToolsEnabled =
            cppToolsConfiguration.get<string>('intelliSenseEngine');
        if (cppToolsEnabled?.toLowerCase() !== 'disabled') {
          vscode.window
              .showWarningMessage(
                  'You have both the Microsoft C++ (cpptools) extension and ' +
                      'clangd extension enabled. The Microsoft IntelliSense features ' +
                      'conflict with clangd\'s code completion, diagnostics etc.',
                  'Disable IntelliSense', 'Never show this warning')
              .then(selection => {
                if (selection == 'Disable IntelliSense') {
                  cppToolsConfiguration.update(
                      'intelliSenseEngine', 'disabled',
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

  return new ClangdExtensionImpl(manager);
}
