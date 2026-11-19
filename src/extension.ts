import * as vscode from 'vscode';

import {ClangdExtension} from '../api/vscode-clangd';

import {ClangdExtensionImpl} from './api';
import * as ast from './ast';
import {get, update} from './config';
import * as fileStatus from './file-status';
import * as inlayHints from './inlay-hints';
import * as install from './install';
import * as memoryUsage from './memory-usage';
import * as openConfig from './open-config';
import * as switchSourceHeader from './switch-source-header';
import * as typeHierarchy from './type-hierarchy';
import {WorkspaceManager} from './workspace-manager';

let apiInstance: ClangdExtensionImpl|undefined;

/**
 *  This method is called when the extension is activated. The extension is
 *  activated the very first time a command is executed.
 */
export async function activate(context: vscode.ExtensionContext):
    Promise<ClangdExtension> {
  const outputChannel =
      vscode.window.createOutputChannel('clangd', {log: true});
  context.subscriptions.push(outputChannel);

  const installer =
      await install.activate(context.subscriptions, context.globalStoragePath);
  const manager = new WorkspaceManager(
      installer, context.globalStoragePath, outputChannel, clangdContext => {
        if (apiInstance)
          apiInstance.client = clangdContext?.client;
      });
  context.subscriptions.push(manager);

  ast.activateGlobal(manager, context.subscriptions);
  fileStatus.activateGlobal(manager, context.subscriptions);
  inlayHints.activateGlobal(context.subscriptions);
  memoryUsage.activateGlobal(manager, context.subscriptions);
  openConfig.activate(manager, context.subscriptions);
  switchSourceHeader.activate(manager, context.subscriptions);
  typeHierarchy.activateGlobal(manager, context.subscriptions);

  context.subscriptions.push(
      vscode.commands.registerCommand('clangd.activate', async () => {
        const clangdContext = manager.activeContext();
        if (clangdContext && (clangdContext.clientIsStarting() ||
                              clangdContext.clientIsRunning())) {
          return;
        }
        await manager.start();
      }));
  context.subscriptions.push(
      vscode.commands.registerCommand('clangd.restart', async () => {
        if (!await get<boolean>('enable')) {
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

        await manager.restart();
      }));
  context.subscriptions.push(vscode.commands.registerCommand(
      'clangd.shutdown', async () => { await manager.shutdown(); }));

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

  apiInstance = new ClangdExtensionImpl(manager.firstContext()?.client);
  return apiInstance;
}
