import * as vscode from 'vscode';

import {ClangdContext} from './clangd-context';
import * as config from './config';

async function promtRestart(settingName: string, promptMessage: string) {
  switch (config.get<string>(settingName)) {
    case 'restart':
      vscode.commands.executeCommand('clangd.restart');
      break;
    case 'ignore':
      break;
    case 'prompt':
    default:
      switch (await vscode.window.showInformationMessage(
        promptMessage,
        'Yes', 'Yes, always', 'No, never')) {
        case 'Yes':
          vscode.commands.executeCommand('clangd.restart');
          break;
        case 'Yes, always':
          vscode.commands.executeCommand('clangd.restart');
          config.update<string>(settingName, 'restart',
            vscode.ConfigurationTarget.Global);
          break;
        case 'No, never':
          config.update<string>(settingName, 'ignore',
            vscode.ConfigurationTarget.Global);
          break;
        default:
          break;
      }
      break;
  }
}

export function activate(context: ClangdContext) {
  if (config.get<string>('onConfigChanged') != 'ignore') {
    const watcher = new ConfigFileWatcher(context);
  }
  vscode.workspace.onDidChangeConfiguration(event => {
    let Settings: string[] = ['clangd.path', 'clangd.arguments'];
    Settings.forEach(element => {
      if (event.affectsConfiguration(element)) {
        promtRestart('onSettingsChanged', `setting '${
                     element}' has changed. Do you want to reload the server?`);
      }
    });
  });
}

class ConfigFileWatcher {
  private databaseWatcher: vscode.FileSystemWatcher = undefined;
  private debounceTimer: NodeJS.Timer = undefined;

  constructor(private context: ClangdContext) {
    this.createFileSystemWatcher();
    context.subscriptions.push(vscode.workspace.onDidChangeWorkspaceFolders(
        () => { this.createFileSystemWatcher(); }));
  }

  createFileSystemWatcher() {
    if (this.databaseWatcher)
      this.databaseWatcher.dispose();
    if (vscode.workspace.workspaceFolders) {
      this.databaseWatcher = vscode.workspace.createFileSystemWatcher(
          '{' +
          vscode.workspace.workspaceFolders.map(f => f.uri.fsPath).join(',') +
          '}/{build/compile_commands.json,compile_commands.json,compile_flags.txt}');
      this.context.subscriptions.push(this.databaseWatcher.onDidChange(
          this.debouncedHandleConfigFilesChanged.bind(this)));
      this.context.subscriptions.push(this.databaseWatcher.onDidCreate(
          this.debouncedHandleConfigFilesChanged.bind(this)));
      this.context.subscriptions.push(this.databaseWatcher);
    }
  }

  async debouncedHandleConfigFilesChanged(uri: vscode.Uri) {
    if (this.debounceTimer) {
      clearTimeout(this.debounceTimer);
    }
    // Sometimes the tools that generate the compilation database, before
    // writing to it, they create a new empty file or they clear the existing
    // one, and after the compilation they write the new content. In this cases
    // the server is not supposed to restart
    if ((await vscode.workspace.fs.stat(uri)).size <= 0) {
      clearTimeout(this.debounceTimer);
      return;
    }
    this.debounceTimer = setTimeout(async () => {
      await promtRestart('onConfigChanged', 
      `Clangd configuration file at '${
        uri.fsPath}' has been changed. Do you want to restart it?`);
      this.debounceTimer = undefined;
    }, 2000);
  }
}