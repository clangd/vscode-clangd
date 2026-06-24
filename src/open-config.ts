import * as os from 'os'
import * as path from 'path'
import * as vscode from 'vscode';

import {ClangdContextManager} from './clangd-context-manager';

/**
 * @returns The path that corresponds to llvm::sys::path::user_config_directory.
 */
function getUserConfigDirectory(): string|undefined {
  switch (os.platform()) {
  case 'win32':
    if (process.env.LocalAppData)
      return process.env.LocalAppData;
    break;
  case 'darwin':
    if (process.env.HOME)
      return path.join(process.env.HOME, 'Library', 'Preferences');
    break;
  default:
    if (process.env.XDG_CONFIG_HOME)
      return process.env.XDG_CONFIG_HOME;
    if (process.env.HOME)
      return path.join(process.env.HOME, '.config');
    break;
  }
  return undefined;
}

function getUserConfigFile(): string|undefined {
  const dir = getUserConfigDirectory();
  if (!dir)
    return undefined;
  return path.join(dir, 'clangd', 'config.yaml');
}

async function openConfigFile(path: vscode.Uri) {
  let p = path;
  try {
    await vscode.workspace.fs.stat(path);
  } catch {
    // File doesn't exist, create a scratch file.
    p = path.with({scheme: 'untitled'});
  }
  vscode.workspace.openTextDocument(p).then((a => {
    vscode.languages.setTextDocumentLanguage(a, 'yaml');
    vscode.window.showTextDocument(a);
  }));
}

export function activate(manager: ClangdContextManager) {
  // Create a command to open the project root .clangd configuration file.
  manager.subscriptions.push(
      vscode.commands.registerCommand('clangd.projectConfig', async () => {
        const context = manager.getActiveContext();
        if (context?.workspaceFolder) {
          openConfigFile(
              vscode.Uri.joinPath(context.workspaceFolder.uri, '.clangd'));
        } else if (vscode.workspace.workspaceFolders?.length) {
          const folder = vscode.workspace.workspaceFolders[0];
          openConfigFile(vscode.Uri.joinPath(folder.uri, '.clangd'));
        } else {
          vscode.window.showErrorMessage('No project is open');
        }
      }));

  manager.subscriptions.push(
      vscode.commands.registerCommand('clangd.userConfig', () => {
        const file = getUserConfigFile();
        if (file) {
          openConfigFile(vscode.Uri.file(file));
        } else {
          vscode.window.showErrorMessage(
              'Couldn\'t get global configuration directory');
        }
      }));
}
