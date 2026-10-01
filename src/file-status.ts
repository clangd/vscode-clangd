import * as path from 'path';
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
    if (newState === vscodelc.State.Running) {
      // clangd starts or restarts after crash.
      context.client.onNotification(
          'textDocument/clangd.fileStatus',
          (fileStatus) => { status.onFileUpdated(fileStatus); });
    } else if (newState === vscodelc.State.Stopped) {
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

async function fileExists(uri: vscode.Uri): Promise<boolean> {
  try {
    await vscode.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}

// Blocks until the compilation database at `dbPath` exists or the user asks to
// proceed anyway. If the context is disposed while waiting, it resolves to
// `false`
export async function waitForCompilationDatabase(
    context: ClangdContext, dbPath: string): Promise<boolean> {
  const resolved =
      path.isAbsolute(dbPath)
          ? dbPath
          : path.join(vscode.workspace.rootPath ?? process.cwd(), dbPath);
  const uri = vscode.Uri.file(resolved);
  if (await fileExists(uri))
    return true;

  const skipCommand = 'clangd.skipWaitForCompilationDatabase';
  const statusBarItem =
      vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 10);
  statusBarItem.text = '$(sync~spin) clangd: waiting for compilation database';
  statusBarItem.tooltip = `Waiting for '${
      resolved}' to exist before starting clangd. Click to start clangd now.`;
  statusBarItem.command = skipCommand;
  statusBarItem.show();

  const watcher =
      vscode.workspace.createFileSystemWatcher(new vscode.RelativePattern(
          vscode.Uri.file(path.dirname(resolved)), path.basename(resolved)));

  return new Promise<boolean>((resolve) => {
           let done = false;
           const finish = (proceed: boolean) => {
             if (done)
               return;
             done = true;
             resolve(proceed);
           };
           const onFileChanged = async () => {
             if (await fileExists(uri))
               finish(true);
           };
           const disposables: vscode.Disposable[] = [
             statusBarItem,
             watcher,
             vscode.commands.registerCommand(skipCommand, () => finish(true)),
             watcher.onDidCreate(onFileChanged),
             watcher.onDidChange(onFileChanged),
             // If the context is disposed (e.g. clangd.restart) while waiting,
             // stop.
             {dispose: () => finish(false)},
           ];
           context.subscriptions.push(...disposables);
           // make sure file hasn't appeared while we were setting up the
           // watcher.
           onFileChanged();
         })
      .finally(() => { statusBarItem.dispose(); });
}
