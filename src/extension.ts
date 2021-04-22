import * as os from 'os'
import * as path from 'path'
import * as vscode from 'vscode';

import {ClangdContext} from './clangd-context';

function getConfigFolder(): string {
  switch (os.platform()) {
  case 'win32':
    if (process.env.LocalAppData)
      return process.env.LocalAppData;
    break;
  case 'darwin':
    if (process.env.HOME)
      return path.join(process.env.HOME, 'Library', 'Preferences');
    break;
  case 'linux':
    if (process.env.XDG_CONFIG_HOME)
      return process.env.XDG_CONFIG_HOME;
    if (process.env.HOME)
      return path.join(process.env.HOME, '.config');
    break;
  default:
    break;
  }
  return null;
}

function getConfigFile(): string {
  const Folder = getConfigFolder();
  if (!Folder)
    return null;
  return path.join(Folder, 'clangd', 'config.yaml');
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
        await clangdContext.activate(context.globalStoragePath, outputChannel,
                                     context.workspaceState);
      }));

  // Create a command to open the project root .clangd configuration file.
  context.subscriptions.push(
      vscode.commands.registerCommand('clangd.projectSettings', () => {
        if (vscode.workspace.workspaceFolders) {
          const folder = vscode.workspace.workspaceFolders[0];
          openConfigFile(vscode.Uri.joinPath(folder.uri, '.clangd'))
        } else {
          vscode.window.showErrorMessage('No project is open');
        }
      }));

  context.subscriptions.push(
      vscode.commands.registerCommand('clangd.globalSettings', () => {
        const file = getConfigFile();
        if (file) {
          openConfigFile(vscode.Uri.file(file));
        } else {
          vscode.window.showErrorMessage(
              'Couldn\'t get global configuration directory');
        }
      }));

  await clangdContext.activate(context.globalStoragePath, outputChannel,
                               context.workspaceState);

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
