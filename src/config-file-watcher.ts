import * as vscode from 'vscode';

import {ClangdContext} from './clangd-context';
import * as config from './config';

const RESTART_DEBOUNCE = 2000;

export function activate(context: ClangdContext) {
  if (config.get<string>('onConfigChanged') != 'ignore') {
    const watcher = new ConfigFileWatcher(context);
  }
}

class ConfigFileWatcher {
  private databaseWatcher: vscode.FileSystemWatcher = undefined;
  private pendingRestartTimer: NodeJS.Timer = undefined;

  constructor(private context: ClangdContext) {
    this.createFileSystemWatcher();
    context.subscriptions.push(vscode.workspace.onDidChangeWorkspaceFolders(
        () => { this.createFileSystemWatcher(); }));
  }

  createFileSystemWatcher() {
    if (this.databaseWatcher)
      this.databaseWatcher.dispose();
    this.databaseWatcher = vscode.workspace.createFileSystemWatcher(
        '{' +
        vscode.workspace.workspaceFolders.map(f => f.uri.fsPath).join(',') +
        '}/{build/compile_commands.json,compile_commands.json,compile_flags.txt,.clang-tidy}');
    this.context.subscriptions.push(this.databaseWatcher.onDidChange(
        this.debouncedHandleConfigFilesChanged.bind(this)));
    this.context.subscriptions.push(this.databaseWatcher.onDidCreate(
        this.debouncedHandleConfigFilesChanged.bind(this)));
    this.context.subscriptions.push(this.databaseWatcher);
  }

  async debouncedHandleConfigFilesChanged(uri: vscode.Uri) {
    if (this.pendingRestartTimer) {
      clearTimeout(this.pendingRestartTimer);
    }

    this.pendingRestartTimer = setTimeout(async () => {
      await this.handleConfigFilesChanged(uri);
      this.pendingRestartTimer = undefined;
    }, RESTART_DEBOUNCE);
  }

  async handleConfigFilesChanged(uri: vscode.Uri) {
    // Sometimes the tools that generate the compilation database, before
    // writing to it, they create a new empty file or they clear the existing
    // one, and after the compilation they write the new content. In this cases
    // the server is not supposed to restart
    if ((await vscode.workspace.fs.stat(uri)).size <= 0)
      return;

    switch (config.get<string>('onConfigChanged')) {
    case 'restart':
      vscode.commands.executeCommand('clangd.restart');
      break;
    case 'ignore':
      break;
    case 'prompt':
    default:
      switch (await vscode.window.showInformationMessage(
          `Clangd configuration file at '${
              uri.fsPath}' has been changed. Do you want to restart it?`,
          'Yes', 'Yes, always', 'No, never')) {
      case 'Yes':
        vscode.commands.executeCommand('clangd.restart');
        break;
      case 'Yes, always':
        vscode.commands.executeCommand('clangd.restart');
        config.update<string>('onConfigChanged', 'restart',
                              vscode.ConfigurationTarget.Global);
        break;
      case 'No, never':
        config.update<string>('onConfigChanged', 'ignore',
                              vscode.ConfigurationTarget.Global);
        break;
      default:
        break;
      }
      break;
    }
  }
}
