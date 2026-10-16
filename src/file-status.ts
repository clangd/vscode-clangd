import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext} from './clangd-context';
import type {ClangdContextManager} from './clangd-context-manager';

// Creates the file status bar item, shared by all contexts.
export function registerCommands(manager: ClangdContextManager):
    vscode.Disposable[] {
  const status = new FileStatus(manager, 'clangd.openOutputPanel');
  return [
    vscode.commands.registerCommand(
        'clangd.openOutputPanel',
        () => manager.activeContext?.client.outputChannel.show()),
    status,
    vscode.window.onDidChangeActiveTextEditor(() => { status.updateStatus(); }),
    manager.onDidAddContext((context) => { status.addContext(context); }),
    manager.onDidRemoveContext((context) => { status.removeContext(context); }),
  ];
}

class FileStatus implements vscode.Disposable {
  // The file statuses (by file path) reported by each context.
  private statuses = new Map<ClangdContext, Map<string, any>>();
  private readonly statusBarItem =
      vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 10);

  constructor(private readonly manager: ClangdContextManager,
              onClickCommand: string) {
    this.statusBarItem.command = onClickCommand;
  }

  addContext(context: ClangdContext) {
    this.statuses.set(context, new Map());
    context.subscriptions.push(context.client.onDidChangeState(({newState}) => {
      if (newState === vscodelc.State.Running) {
        // clangd starts or restarts after crash.
        context.client.onNotification(
            'textDocument/clangd.fileStatus',
            (fileStatus) => { this.onFileUpdated(context, fileStatus); });
      } else if (newState === vscodelc.State.Stopped) {
        // Clear all cached statuses when clangd crashes.
        this.clear(context);
      }
    }));
  }

  removeContext(context: ClangdContext) {
    this.clear(context);
    this.statuses.delete(context);
  }

  onFileUpdated(context: ClangdContext, fileStatus: any) {
    const filePath = vscode.Uri.parse(fileStatus.uri);
    this.statuses.get(context)?.set(filePath.fsPath, fileStatus);
    this.updateStatus();
  }

  updateStatus() {
    const activeDoc = vscode.window.activeTextEditor?.document;
    // Work around https://github.com/microsoft/vscode/issues/58869
    // Don't hide the status when activeTextEditor is output panel.
    // This aligns with the behavior of other panels, e.g. problems.
    if (!activeDoc || activeDoc.uri.scheme === 'output')
      return;
    const context = this.manager.contextFor(activeDoc.uri);
    const status =
        context && this.statuses.get(context)?.get(activeDoc.fileName);
    if (!status) {
      this.statusBarItem.hide();
      return;
    }
    this.statusBarItem.text = `clangd: ` + status.state;
    this.statusBarItem.show();
  }

  // Hides the status even while the output panel is active, then shows the
  // status of the active editor's context, if any.
  clear(context: ClangdContext) {
    this.statuses.get(context)?.clear();
    this.statusBarItem.hide();
    this.updateStatus();
  }

  dispose() { this.statusBarItem.dispose(); }
}
