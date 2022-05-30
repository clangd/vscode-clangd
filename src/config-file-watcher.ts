import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext} from './clangd-context';
import * as config from './config';

export function activate(context: ClangdContext) {
  if (config.get<string>('onConfigChanged') !== 'ignore') {
    context.client.registerFeature(new ConfigFileWatcherFeature(context));
  }
}

// Clangd extension capabilities.
interface ClangdClientCapabilities {
  compilationDatabase?: {automaticReload?: boolean;},
}

class ConfigFileWatcherFeature implements vscodelc.StaticFeature {
  constructor(private context: ClangdContext) {}
  fillClientCapabilities(capabilities: vscodelc.ClientCapabilities) {}

  initialize(capabilities: vscodelc.ServerCapabilities,
             _documentSelector: vscodelc.DocumentSelector|undefined) {
    if ((capabilities as ClangdClientCapabilities)
            .compilationDatabase?.automaticReload)
      return;
    this.context.subscriptions.push(new ConfigFileWatcher(this.context));
  }
  getState(): vscodelc.FeatureState { return {kind: 'static'}; }
  dispose() {}
}

class ConfigFileWatcher implements vscode.Disposable {
  private databaseWatcher?: vscode.FileSystemWatcher;
  private debounceTimer?: NodeJS.Timer;

  dispose() {
    if (this.databaseWatcher)
      this.databaseWatcher.dispose();
  }

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

    this.debounceTimer = setTimeout(async () => {
      await this.handleConfigFilesChanged(uri);
      this.debounceTimer = undefined;
    }, 2000);
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
