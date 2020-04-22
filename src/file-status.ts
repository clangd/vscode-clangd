import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient';

export function activate(client : vscodelc.LanguageClient, context : vscode.ExtensionContext) {
  const status = new FileStatus();
  context.subscriptions.push(vscode.Disposable.from(status));
  context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(
      () => { status.updateStatus(); }));
  context.subscriptions.push(client.onDidChangeState(({newState}) => {
    if (newState == vscodelc.State.Running) {
      // clangd starts or restarts after crash.
      client.onNotification(
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
  
    onFileUpdated(fileStatus: any) {
      const filePath = vscode.Uri.parse(fileStatus.uri);
      this.statuses.set(filePath.fsPath, fileStatus);
      this.updateStatus();
    }
  
    updateStatus() {
      const path = vscode.window.activeTextEditor.document.fileName;
      const status = this.statuses.get(path);
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
