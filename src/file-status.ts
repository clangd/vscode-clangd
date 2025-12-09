import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext} from './clangd-context';
import {ClangdContextManager} from './clangd-context-manager';

export function activate(manager: ClangdContextManager) {
  manager.subscriptions.push(vscode.commands.registerCommand(
      'clangd.openOutputPanel',
      () => { manager.getActiveContext()?.client.outputChannel.show(); }));

  const status = new FileStatus(manager);
  manager.subscriptions.push(vscode.Disposable.from(status));

  for (const context of manager.getAllContexts()) {
    status.registerContext(context);
  }

  manager.subscriptions.push(manager.onDidCreateContext(
      (context) => { status.registerContext(context); }));

  manager.subscriptions.push(manager.onDidDisposeContext(
      (context) => { status.unregisterContext(context); }));

  manager.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(
      () => { status.updateStatus(); }));
}

class FileStatus {
  // Map from context to its file statuses (file path -> status)
  private statuses = new Map<ClangdContext, Map<string, any>>();
  private subscriptions = new Map<ClangdContext, vscode.Disposable[]>();
  private readonly statusBarItem =
      vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 10);

  constructor(private readonly manager: ClangdContextManager) {
    this.statusBarItem.command = 'clangd.openOutputPanel';
  }

  registerContext(context: ClangdContext) {
    this.statuses.set(context, new Map());
    const subs: vscode.Disposable[] = [];

    subs.push(context.client.onDidChangeState(({newState}) => {
      if (newState === vscodelc.State.Running) {
        // clangd starts or restarts after crash.
        context.client.onNotification(
            'textDocument/clangd.fileStatus',
            (fileStatus) => { this.onFileUpdated(context, fileStatus); });
      } else if (newState === vscodelc.State.Stopped) {
        // Clear cached statuses for this context when clangd crashes.
        this.clearContext(context);
      }
    }));

    this.subscriptions.set(context, subs);
  }

  unregisterContext(context: ClangdContext) {
    const subs = this.subscriptions.get(context);
    if (subs) {
      subs.forEach(d => d.dispose());
      this.subscriptions.delete(context);
    }

    this.statuses.delete(context);
    this.updateStatus();
  }

  private onFileUpdated(context: ClangdContext, fileStatus: any) {
    const filePath = vscode.Uri.parse(fileStatus.uri);
    const statuses = this.statuses.get(context);
    if (statuses) {
      statuses.set(filePath.fsPath, fileStatus);
    }
    this.updateStatus();
  }

  updateStatus() {
    const activeDoc = vscode.window.activeTextEditor?.document;
    // Work around https://github.com/microsoft/vscode/issues/58869
    // Don't hide the status when activeTextEditor is output panel.
    // This aligns with the behavior of other panels, e.g. problems.
    if (!activeDoc || activeDoc.uri.scheme === 'output')
      return;

    const context = this.manager.getContextForDocument(activeDoc);
    if (!context) {
      this.statusBarItem.hide();
      return;
    }

    const statuses = this.statuses.get(context);
    const status = statuses?.get(activeDoc.fileName);
    if (!status) {
      this.statusBarItem.hide();
      return;
    }

    if (context.workspaceFolder) {
      this.statusBarItem.text =
          `clangd (${context.workspaceFolder.name}): ${status.state}`;
    } else {
      this.statusBarItem.text = `clangd: ${status.state}`;
    }
    this.statusBarItem.show();
  }

  private clearContext(context: ClangdContext) {
    const statuses = this.statuses.get(context);
    if (statuses) {
      statuses.clear();
    }
    this.updateStatus();
  }

  dispose() {
    for (const subs of this.subscriptions.values()) {
      subs.forEach(d => d.dispose());
    }
    this.subscriptions.clear();
    this.statuses.clear();
    this.statusBarItem.dispose();
  }
}
