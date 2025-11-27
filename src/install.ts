// Automatically install clangd binary releases from GitHub.
// This wraps `@clangd/install` in the VSCode UI. See that package for more.

import * as common from '@clangd/install';
import * as path from 'path';
import * as vscode from 'vscode';

import * as config from './config';

// Returns the clangd path to be used, or null if clangd is not installed.
export async function activate(disposables: vscode.Disposable[],
                               globalStoragePath: string):
    Promise<string|null> {
  const ui = await UI.create(globalStoragePath);
  disposables.push(vscode.commands.registerCommand(
      'clangd.install', async () => common.installLatest(ui)));
  disposables.push(vscode.commands.registerCommand(
      'clangd.update', async () => common.checkUpdates(true, ui)));
  const status =
      await common.prepare(ui, await config.get<boolean>('checkUpdates'));
  return status.clangdPath;
}

class UI {
  static async create(globalStoragePath: string): Promise<UI> {
    const ui = new UI(globalStoragePath);
    await ui.resolveClangdPath();
    return ui;
  }

  private constructor(private globalStoragePath: string) {}

  get storagePath(): string { return this.globalStoragePath; }
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
      cancellable: cancel !== null,
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
  localize(message: string, ...args: Array<string|number|boolean>): string {
    let ret = message;
    for (const i in args) {
      ret = ret.replace(`{${i}}`, args[i].toString());
    }
    return ret;
  }
  error(s: string) { vscode.window.showErrorMessage(s); }
  info(s: string) { vscode.window.showInformationMessage(s); }

  async shouldReuse(release: string): Promise<boolean|undefined> {
    const message = `clangd ${release} is already installed!`;
    const use = 'Use the installed version';
    const reinstall = 'Delete it and reinstall';
    const response =
        await vscode.window.showInformationMessage(message, use, reinstall);
    if (response === use) {
      // Find clangd within the existing directory.
      return true;
    } else if (response === reinstall) {
      // Remove the existing installation.
      return false;
    } else {
      // User dismissed prompt, bail out.
      return undefined;
    }
  }

  private _pathUpdated: Promise<void>|null = null;

  async promptReload(message: string) {
    vscode.window.showInformationMessage(message);
    await this._pathUpdated;
    this._pathUpdated = null;
    vscode.commands.executeCommand('clangd.restart');
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
        await vscode.window.showInformationMessage(message, update, dontCheck);
    if (response === update) {
      common.installLatest(this);
    } else if (response === dontCheck) {
      config.update('checkUpdates', false, vscode.ConfigurationTarget.Global);
    }
  }

  async promptInstall(version: string) {
    const p = this.clangdPath;
    let message = '';
    if (p.indexOf(path.sep) < 0) {
      message += `The '${p}' language server was not found on your PATH.\n`;
    } else {
      message += `The clangd binary '${p}' was not found.\n`;
    }
    message += `Would you like to download and install clangd ${version}?`;
    if (await vscode.window.showInformationMessage(message, 'Install'))
      common.installLatest(this);
  }

  async resolveClangdPath() {
    let p = await config.get<string>('path');
    // Backwards compatibility: if it's a relative path with a slash, interpret
    // relative to project root.
    if (!path.isAbsolute(p) && p.includes(path.sep) &&
        vscode.workspace.rootPath !== undefined) {
      p = path.join(vscode.workspace.rootPath, p);
    }

    this._clangdPath = p;
  }

  private _clangdPath?: string = undefined;

  get clangdPath(): string { return this._clangdPath as string; }
  set clangdPath(p: string) {
    this._pathUpdated = new Promise(resolve => {
      config.update('path', p, vscode.ConfigurationTarget.Global).then(() => {
        this._clangdPath = p;
        resolve();
      });
    });
  }
}
