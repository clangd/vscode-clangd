import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext} from './clangd-context';
import {ClangdClientProvider} from './client-provider';
import * as config from './config';

export async function activate(context: ClangdContext,
                               provider: ClangdClientProvider) {
  if (await config.get<string>('onConfigChanged', context.folder) !==
      'ignore') {
    context.client.registerFeature(
        new ConfigFileWatcherFeature(context, provider));
  }
}

// Clangd extension capabilities.
interface ClangdClientCapabilities {
  compilationDatabase?: {automaticReload?: boolean;},
}

class ConfigFileWatcherFeature implements vscodelc.StaticFeature {
  constructor(private context: ClangdContext,
              private provider: ClangdClientProvider) {}
  fillClientCapabilities(capabilities: vscodelc.ClientCapabilities) {}

  async initialize(capabilities: vscodelc.ServerCapabilities,
                   _documentSelector: vscodelc.DocumentSelector|undefined) {
    if (!await config.get<boolean>('onConfigChangedForceEnable',
                                   this.context.folder) &&
        (capabilities as ClangdClientCapabilities)
            .compilationDatabase?.automaticReload) {
      return;
    }
    this.context.subscriptions.push(
        new ConfigFileWatcher(this.context, this.provider));
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
    if (this.debounceTimer)
      clearTimeout(this.debounceTimer);
  }

  constructor(private context: ClangdContext,
              private provider: ClangdClientProvider) {
    this.createFileSystemWatcher();
  }

  createFileSystemWatcher() {
    if (this.databaseWatcher)
      this.databaseWatcher.dispose();
    if (this.context.folder) {
      this.databaseWatcher =
          vscode.workspace.createFileSystemWatcher(new vscode.RelativePattern(
              this.context.folder,
              '{build/compile_commands.json,compile_commands.json,compile_flags.txt}'));
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

    switch (await config.get<string>('onConfigChanged', this.context.folder)) {
    case 'restart':
      await this.provider.restart(this.context.folder);
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
        await this.provider.restart(this.context.folder);
        break;
      case 'Yes, always':
        await this.provider.restart(this.context.folder);
        config.update<string>('onConfigChanged', 'restart',
                              this.context.folder
                                  ? vscode.ConfigurationTarget.WorkspaceFolder
                                  : vscode.ConfigurationTarget.Global,
                              this.context.folder);
        break;
      case 'No, never':
        config.update<string>('onConfigChanged', 'ignore',
                              this.context.folder
                                  ? vscode.ConfigurationTarget.WorkspaceFolder
                                  : vscode.ConfigurationTarget.Global,
                              this.context.folder);
        break;
      default:
        break;
      }
      break;
    }
  }
}
