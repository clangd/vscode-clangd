import * as path from 'path';
import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient';

import * as config from './config';

export function activate(context: vscode.ExtensionContext, args: string[]) {
  let compileCommandsFolder: string|undefined = undefined;

  for (let arg of args) {
    if (arg.startsWith('--compile-commands-dir')) {
      let match = arg.match(/--compile-commands-dir="?([^"]*)"?/);
      compileCommandsFolder = match[1] || undefined;
    }
  }

  const watcher = new ConfigFileWatcher(context, compileCommandsFolder);
}

class ConfigFileWatcher {
  private databaseWatcher = vscode.workspace.createFileSystemWatcher(
      '**/{compile_commands.json,compile_flags.txt,.clang-tidy}');

  constructor(private context: vscode.ExtensionContext,
              private compileCommandsFolder: string|undefined) {
    this.databaseWatcher.onDidChange(this.handleConfigFilesChanged.bind(this));
    this.databaseWatcher.onDidCreate(this.handleConfigFilesChanged.bind(this));
    context.subscriptions.push(this.databaseWatcher);
  }

  async handleConfigFilesChanged(uri: vscode.Uri) {
    if ((await vscode.workspace.fs.readFile(uri)).length) {
      let relativePath = vscode.workspace.asRelativePath(uri, false);
      let baseName = path.basename(uri.fsPath);
      let restart = false;

      if (baseName == 'compile_commands.json') {
        if (this.compileCommandsFolder &&
            relativePath == path.join(this.compileCommandsFolder, baseName))
          restart = true;
        else if (!this.compileCommandsFolder && relativePath == baseName)
          restart = true;
      } else {
        if (relativePath == baseName)
          restart = true;
      }

      if (restart) {
        switch (config.get<string>('onConfigChanged')) {
        case 'restart':
          vscode.commands.executeCommand('clangd.restart');
          break;
        case 'ignore':
          break;
        case 'prompt':
        default:
          switch (await vscode.window.showInformationMessage(
              'Clangd configuration file at \'' + uri.fsPath +
                  '\' has been changed. Do you want to restart it?',
              'Yes', 'Yes, always', 'No, never')) {
          case 'Yes':
            vscode.commands.executeCommand('clangd.restart');
            break;
          case 'Yes, always':
            vscode.commands.executeCommand('clangd.restart');
            config.update<string>('onConfigChanged', 'restart');
            break;
          case 'No, never':
            config.update<string>('onConfigChanged', 'ignore');
            break;
          default:
            break;
          }
          break;
        }
      }
    }
  }
}
