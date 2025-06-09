import * as vscode from 'vscode';

import {ClangdContext} from './clangd-context';

export class FileStatus {
  private statuses = new Map<string, any>();
  private statusBarItem: vscode.StatusBarItem;

  constructor(context: ClangdContext) {
    context.subscriptions.push(vscode.commands.registerCommand(
      'clangd.openOutputPanel', () => context.getActiveClient()?.outputChannel.show()));
    this.statusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 10);
    this.statusBarItem.command = 'clangd.openOutputPanel'
    context.subscriptions.push(this.statusBarItem);
    context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(
      () => { this.updateStatus(); }));
  }

  onFileUpdated(fileStatus: any) {
    const filePath = vscode.Uri.parse(fileStatus.uri);
    this.statuses.set(filePath.fsPath, fileStatus);
    this.updateStatus();
  }

  updateStatus() {
    const activeDoc = vscode.window.activeTextEditor?.document;
    // Work around https://github.com/microsoft/vscode/issues/58869
    // Don't hide the status when activeTextEditor is output panel.
    // This aligns with the behavior of other panels, e.g. problems.
    if (!activeDoc || activeDoc.uri.scheme === 'output')
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
