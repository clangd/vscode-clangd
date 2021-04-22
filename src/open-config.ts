import * as os from 'os'
import * as path from 'path'
import * as vscode from 'vscode';

/**
 * @returns The path that corresponds to llvm::sys::path::user_config_directory.
 */
function getUserConfigDirectory(): string {
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
  return null;
}

function getUserConfigFile(): string {
  const dir = getUserConfigDirectory();
  if (!dir)
    return null;
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

export function activate(context: vscode.ExtensionContext) {
  // Create a command to open the project root .clangd configuration file.
  context.subscriptions.push(
      vscode.commands.registerCommand('clangd.projectConfig', () => {
        if (vscode.workspace.workspaceFolders.length > 0) {
          const folder = vscode.workspace.workspaceFolders[0];
          openConfigFile(vscode.Uri.joinPath(folder.uri, '.clangd'))
        } else {
          vscode.window.showErrorMessage('No project is open');
        }
      }));

  context.subscriptions.push(
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