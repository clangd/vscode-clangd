import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext} from './clangd-context';

export function activate(context: ClangdContext) {
  context.subscriptions.push(vscode.commands.registerCommand(
      'clangd.openOutputPanel', () => context.client.outputChannel.show()));
  const status = new FileStatus('clangd.openOutputPanel');
  context.subscriptions.push(vscode.Disposable.from(status));
  context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(
      () => { status.updateStatus(); }));
  context.subscriptions.push(context.client.onDidChangeState(({newState}) => {
    if (newState == vscodelc.State.Running) {
      // clangd starts or restarts after crash.
      context.client.onNotification(
          'textDocument/clangd.fileStatus',
          (fileStatus) => { status.onFileUpdated(fileStatus); });
    } else if (newState == vscodelc.State.Stopped) {
      // Clear all cached statuses when clangd crashes.
      status.clear();
    }
  }));
}

class FileStatus {
  private statuses = new Map<string, any>();
  private readonly statusBarItem =
      vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 10);

  constructor(onClickCommand: string) {
    this.statusBarItem.command = onClickCommand;
  }

  onFileUpdated(fileStatus: any) {
    const filePath = vscode.Uri.parse(fileStatus.uri);
    this.statuses.set(filePath.fsPath, fileStatus);
    this.updateStatus();
  }

  updateStatus() {
    const activeDoc = vscode.window.activeTextEditor.document;
    // Work around https://github.com/microsoft/vscode/issues/58869
    // Don't hide the status when activeTextEditor is output panel.
    // This aligns with the behavior of other panels, e.g. problems.
    if (activeDoc.uri.scheme == 'output')
      return;
    const status = this.statuses.get(activeDoc.fileName);
    if (!status) {
      this.statusBarItem.hide();
      return;
    }
    this.statusBarItem.text = `clangd: ` + status.state;
    this.statusBarItem.show();
  }

  clear() {
    this.statuses.clear();
    this.statusBarItem.hide();
  }

  dispose() { this.statusBarItem.dispose(); }
}
