// Automatically install clangd binary releases from GitHub.
// This wraps `@clangd/install` in the VSCode UI. See that package for more.

import * as common from '@clangd/install';
import AbortController from 'abort-controller';
import * as path from 'path';
import * as vscode from 'vscode';

import * as config from './config';

// Returns the clangd path to be used, or null if clangd is not installed.
export async function activate(context: vscode.ExtensionContext):
    Promise<string> {
  const ui = new UI(context);
  context.subscriptions.push(vscode.commands.registerCommand(
      'clangd.install', async () => common.installLatest(ui)));
  context.subscriptions.push(vscode.commands.registerCommand(
      'clangd.update', async () => common.checkUpdates(true, ui)));
  const status = await common.prepare(ui, config.get<boolean>('checkUpdates'));
  return status.clangdPath;
}

class UI {
  constructor(private context: vscode.ExtensionContext) {}

  get storagePath(): string { return this.context.globalStoragePath; }
  async choose(prompt: string, options: string[]): Promise<string|undefined> {
    return await vscode.window.showInformationMessage(prompt, ...options);
  }
  slow<T>(title: string, result: Promise<T>) {
    const opts = {
      location: vscode.ProgressLocation.Notification,
      title: title,
      cancellable: false,
    };
    return Promise.resolve(vscode.window.withProgress(opts, () => result));
  }
  progress<T>(title: string, cancel: AbortController|null,
              body: (progress: (fraction: number) => void) => Promise<T>) {
    const opts = {
      location: vscode.ProgressLocation.Notification,
      title: title,
      cancellable: cancel != null,
    };
    const result = vscode.window.withProgress(opts, async (progress, canc) => {
      if (cancel)
        canc.onCancellationRequested((_) => cancel.abort());
      let lastFraction = 0;
      return body(fraction => {
        if (fraction > lastFraction) {
          progress.report({increment: 100 * (fraction - lastFraction)});
          lastFraction = fraction;
        }
      });
    });
    return Promise.resolve(result); // Thenable to real promise.
  }
  error(s: string) { vscode.window.showErrorMessage(s); }
  info(s: string) { vscode.window.showInformationMessage(s); }
  command(name: string, body: () => any) {
    this.context.subscriptions.push(
        vscode.commands.registerCommand(name, body));
  }

  async shouldReuse(release: string): Promise<boolean> {
    const message = `clangd ${release} is already installed!`;
    const use = 'Use the installed version';
    const reinstall = 'Delete it and reinstall';
    const response =
        await vscode.window.showInformationMessage(message, use, reinstall);
    if (response == use) {
      // Find clangd within the existing directory.
      return true;
    } else if (response == reinstall) {
      // Remove the existing installation.
      return false;
    } else {
      // User dismissed prompt, bail out.
      return undefined;
    }
  }

  async promptReload(message: string) {
    if (await vscode.window.showInformationMessage(message, 'Reload window'))
      vscode.commands.executeCommand('workbench.action.reloadWindow');
  }

  async showHelp(message: string, url: string) {
    if (await vscode.window.showInformationMessage(message, 'Open website'))
      vscode.env.openExternal(vscode.Uri.parse(url));
  }

  async promptUpdate(oldVersion: string, newVersion: string) {
    const message = 'An updated clangd language server is available.\n ' +
                    `Would you like to upgrade to clangd ${newVersion}? ` +
                    `(from ${oldVersion})`;
    const update = `Install clangd ${newVersion}`;
    const dontCheck = 'Don\'t ask again';
    const response =
        await vscode.window.showInformationMessage(message, update, dontCheck)
    if (response == update) {
      common.installLatest(this);
    }
    else if (response == dontCheck) {
      config.update('checkUpdates', false, vscode.ConfigurationTarget.Global);
    }
  }

  async promptInstall(version: string) {
    const message = 'The clangd language server was not found on your PATH.\n' +
                    `Would you like to download and install clangd ${version}?`;
    if (await vscode.window.showInformationMessage(message, 'Install'))
      common.installLatest(this);
  }

  get clangdPath(): string {
    let p = config.get<string>('path');
    // Backwards compatibility: if it's a relative path with a slash, interpret
    // relative to project root.
    if (!path.isAbsolute(p) && p.indexOf(path.sep) != -1 &&
        vscode.workspace.rootPath !== undefined)
      p = path.join(vscode.workspace.rootPath, p);
    return p;
  }
  set clangdPath(p: string) {
    config.update('path', p, vscode.ConfigurationTarget.Global);
  }
}
