import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext} from './clangd-context';
import {ClangdClientProvider} from './client-provider';

let status: FileStatus|undefined;

export function activateGlobal(provider: ClangdClientProvider,
                               subscriptions: vscode.Disposable[]) {
  status = new FileStatus('clangd.openOutputPanel');
  subscriptions.push(
      status,
      vscode.commands.registerCommand(
          'clangd.openOutputPanel',
          () => { provider.activeContext()?.client.outputChannel.show(); }),
      vscode.window.onDidChangeActiveTextEditor(() => status?.updateStatus()));
}

export function activate(context: ClangdContext) {
  context.subscriptions.push(context.client.onDidChangeState(({newState}) => {
    if (newState === vscodelc.State.Running) {
      // clangd starts or restarts after crash.
      context.client.onNotification(
          'textDocument/clangd.fileStatus',
          (fileStatus) => { status?.onFileUpdated(fileStatus); });
    } else if (newState === vscodelc.State.Stopped) {
      status?.clear(context.folder);
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
    if (this.statuses.size)
      this.updateStatus();
    else
      this.statusBarItem.hide();
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

  clear(folder?: vscode.WorkspaceFolder) {
    if (!folder) {
      this.statuses.clear();
    } else {
      for (const file of this.statuses.keys()) {
        if (vscode.workspace.getWorkspaceFolder(vscode.Uri.file(file)) ===
            folder)
          this.statuses.delete(file);
      }
    }
    this.updateStatus();
  }

  dispose() { this.statusBarItem.dispose(); }
}
