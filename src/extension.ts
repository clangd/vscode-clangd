import * as vscode from 'vscode';

import {ClangdExtension} from '../api/vscode-clangd';

import {ClangdExtensionImpl} from './api';
import {ClangdContext, isClangdDocument} from './clangd-context';
import {get, update} from './config';
import * as install from './install';

let apiInstance: ClangdExtensionImpl|undefined;

/**
 *  This method is called when the extension is activated. The extension is
 *  activated the very first time a command is executed.
 */
export async function activate(context: vscode.ExtensionContext):
    Promise<ClangdExtension> {
  const outputChannel = vscode.window.createOutputChannel('clangd');
  context.subscriptions.push(outputChannel);

  const clangdContext = new ClangdContext(outputChannel);
  context.subscriptions.push(clangdContext);
  await install.activate(context.subscriptions, context.globalStoragePath);

  // An empty place holder for the activate command, otherwise we'll get an
  // "command is not registered" error.
  context.subscriptions.push(
      vscode.commands.registerCommand('clangd.activate', async () => {}));
  context.subscriptions.push(
      vscode.commands.registerCommand('clangd.restart', async () => {
        const folder = clangdContext.getActiveFolder();
        if (folder === undefined) {
          return;
        }

        if (!get<boolean>('enable', folder)) {
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

        const client = clangdContext.getActiveClient();
        if (client === undefined) {
          return;
        }

        // clangd.restart can be called when the extension is not yet activated.
        // In such a case, vscode will activate the extension and then run this
        // handler. Detect this situation and bail out (doing an extra
        // stop/start cycle in this situation is pointless, and doesn't work
        // anyways because the client can't be stop()-ped when it's still in the
        // Starting state).
        if (client.isStarting()) {
            return;
        }

        clangdContext.removeClient(folder);
        clangdContext.addClient(folder);
      }));

  function didOpenTextDocument(document: vscode.TextDocument) {
    if (!isClangdDocument(document) || document.uri.scheme !== 'file') {
      return;
    }

    let folder = vscode.workspace.getWorkspaceFolder(document.uri);
    if (folder === undefined) {
      return;
    }

    if (!clangdContext.hasClient(folder)) {
      clangdContext.addClient(folder);
    }
  }

  vscode.workspace.onDidOpenTextDocument(didOpenTextDocument);
	vscode.workspace.textDocuments.forEach(didOpenTextDocument);
	vscode.workspace.onDidChangeWorkspaceFolders((event) => {
		for (const folder of event.removed) {
			clangdContext.removeClient(folder);
		}
	});

  let shouldCheck = false;

  if (vscode.workspace.getConfiguration('clangd').get<boolean>('enable')) {
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

  apiInstance = new ClangdExtensionImpl(clangdContext);
  return apiInstance;
}
