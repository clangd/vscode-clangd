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
  const contextManager = new ClangdContextManager(context.globalStoragePath);
  context.subscriptions.push(contextManager);

  // Create the API instance that wraps the context manager
  const apiInstance = new ClangdExtensionImpl(contextManager);
  context.subscriptions.push(apiInstance);

  context.subscriptions.push(
      vscode.commands.registerCommand('clangd.activate', async () => {
        if (contextManager.isAnyContextStarting() ||
            contextManager.isAnyContextRunning()) {
          return;
        }
        vscode.commands.executeCommand('clangd.restart');
      }));
  context.subscriptions.push(
      vscode.commands.registerCommand('clangd.restart', async () => {
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

        // clangd.restart can be called when the extension is not yet activated.
        // In such a case, vscode will activate the extension and then run this
        // handler. Detect this situation and bail out (doing an extra
        // stop/start cycle in this situation is pointless, and doesn't work
        // anyways because the client can't be stop()-ped when it's still in the
        // Starting state).
        if (contextManager.isAnyContextStarting()) {
          return;
        }

        // For now, restart the active context. In future phases, we might
        // add an option to restart all contexts or prompt the user.
        const activeContext = contextManager.getActiveContext();
        if (activeContext) {
          const folder = activeContext.workspaceFolder;
          contextManager.disposeContext(folder);
          await contextManager.createContext(folder);
        } else {
          // No active context, restart global
          contextManager.disposeContext(null);
          await contextManager.createContext(null);
        }
      }));
  context.subscriptions.push(
      vscode.commands.registerCommand('clangd.shutdown', async () => {
        if (contextManager.isAnyContextStarting()) {
          return;
        }
        // Shutdown the active context
        const activeContext = contextManager.getActiveContext();
        if (activeContext) {
          contextManager.disposeContext(activeContext.workspaceFolder);
        }
      }));

  let shouldCheck = false;

  if (vscode.workspace.getConfiguration('clangd').get<boolean>('enable')) {
    await contextManager.activate();
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

  return apiInstance;
}
