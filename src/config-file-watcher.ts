import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext} from './clangd-context';
import * as config from './config';

export async function activate(context: ClangdContext) {
  if (await config.get<string>('onConfigChanged', context.workspaceFolder) !==
      'ignore') {
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

  async initialize(capabilities: vscodelc.ServerCapabilities,
                   _documentSelector: vscodelc.DocumentSelector|undefined) {
    if (!await config.get<boolean>('onConfigChangedForceEnable',
                                   this.context.workspaceFolder) &&
        (capabilities as ClangdClientCapabilities)
            .compilationDatabase?.automaticReload) {
      return;
    }
    this.context.subscriptions.push(new ConfigFileWatcher(this.context));
  }
  getState(): vscodelc.FeatureState { return {kind: 'static'}; }
  clear() {}
}

class ConfigFileWatcher implements vscode.Disposable {
  private databaseWatcher?: vscode.FileSystemWatcher;
  private debounceTimer?: NodeJS.Timeout;

  dispose() {
    if (this.databaseWatcher)
      this.databaseWatcher.dispose();
  }

  constructor(private context: ClangdContext) {
    this.createFileSystemWatcher();
    // Only watch for workspace folder changes if this is the global context
    // (no workspaceFolder). Per-folder contexts don't need to update their
    // watcher since they only watch their specific folder.
    if (!context.workspaceFolder) {
      context.subscriptions.push(vscode.workspace.onDidChangeWorkspaceFolders(
          () => { this.createFileSystemWatcher(); }));
    }
  }

  createFileSystemWatcher() {
    if (this.databaseWatcher)
      this.databaseWatcher.dispose();

    let foldersToWatch: readonly vscode.WorkspaceFolder[];
    if (this.context.workspaceFolder) {
      foldersToWatch = [this.context.workspaceFolder];
    } else if (vscode.workspace.workspaceFolders) {
      foldersToWatch = vscode.workspace.workspaceFolders;
    } else {
      return;
    }

    if (foldersToWatch.length > 0) {
      this.databaseWatcher = vscode.workspace.createFileSystemWatcher(
          '{' + foldersToWatch.map(f => f.uri.fsPath).join(',') +
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

    switch (await config.get<string>('onConfigChanged',
                                     this.context.workspaceFolder)) {
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
