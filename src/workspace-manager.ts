import * as vscode from 'vscode';

import {ClangdContext, isClangdDocument} from './clangd-context';
import {
  ClangdClientProvider,
  ClangdFeature,
} from './client-provider';
import {ExecuteCommandRouter} from './execute-command';
import {ClangdInstaller} from './install';
import {workspaceFolderForUri} from './workspace-folder';

const DEFAULT_CLIENT = '<default>';
export type ClangdContextFactory = typeof ClangdContext.create;

export class WorkspaceManager implements ClangdClientProvider,
                                         vscode.Disposable {
  readonly executeCommandRouter = new ExecuteCommandRouter(this);
  private contexts = new Map<string, ClangdContext>();
  private starting = new Map<string, Promise<ClangdContext|null>>();
  private featureSupport =
      new Map<ClangdContext, Map<ClangdFeature, boolean>>();
  private folders = new Map<string, vscode.WorkspaceFolder|undefined>();
  private generation = new Map<string, number>();
  private disposables: vscode.Disposable[] = [];
  private automaticStartEnabled = false;
  private removeContextEmitter = new vscode.EventEmitter<ClangdContext>();
  readonly onDidRemoveContext = this.removeContextEmitter.event;

  constructor(private installer: ClangdInstaller,
              private globalStoragePath: string,
              private outputChannel: vscode.LogOutputChannel,
              private onActiveContextChanged: (context: ClangdContext|
                                               undefined) => void,
              private contextFactory:
                  ClangdContextFactory = ClangdContext.create) {
    this.disposables.push(
        vscode.workspace.onDidOpenTextDocument(
            document => this.runInBackground(this.ensureForDocument(document))),
        vscode.workspace.onDidChangeWorkspaceFolders(
            event => this.runInBackground(this.workspaceFoldersChanged(event))),
        vscode.window.onDidChangeActiveTextEditor(
            () => { this.updateActiveContext(); }));
  }

  async start(): Promise<void> {
    this.automaticStartEnabled = true;
    await this.settle(vscode.workspace.textDocuments.map(
        document => this.ensureForDocument(document)));
    this.updateActiveContext();
  }

  contextForUri(uri: vscode.Uri): ClangdContext|undefined {
    return this.contexts.get(this.key(workspaceFolderForUri(uri)));
  }

  activeContext(): ClangdContext|undefined {
    const uri = vscode.window.activeTextEditor?.document.uri;
    return uri ? this.contextForUri(uri) : undefined;
  }

  firstContext(): ClangdContext|undefined {
    return this.activeContext() ?? this.contexts.values().next().value;
  }

  async restart(folder?: vscode.WorkspaceFolder): Promise<void> {
    this.automaticStartEnabled = true;
    const targets = folder ? [folder] : Array.from(this.folders.values());
    if (!targets.length) {
      await this.start();
      return;
    }
    await Promise.all(targets.map(target => this.remove(target)));
    await this.settle(targets.map(target => this.startForFolder(target)));
    this.updateActiveContext();
  }

  async shutdown(): Promise<void> {
    this.automaticStartEnabled = false;
    await Promise.all(
        Array.from(this.folders.values(), folder => this.remove(folder)));
    this.starting.clear();
    this.folders.clear();
    this.featureSupport.clear();
    this.updateActiveContext();
  }

  setFeatureSupport(context: ClangdContext, feature: ClangdFeature,
                    supported: boolean): void {
    let features = this.featureSupport.get(context);
    if (!features) {
      features = new Map();
      this.featureSupport.set(context, features);
    }
    features.set(feature, supported);
    if (context === this.activeContext())
      this.updateFeatureContexts(context);
  }

  dispose(): void {
    void this.shutdown();
    this.disposables.forEach(disposable => disposable.dispose());
    this.executeCommandRouter.dispose();
    this.removeContextEmitter.dispose();
    this.disposables = [];
  }

  private async ensureForDocument(document: vscode.TextDocument):
      Promise<ClangdContext|null> {
    if (!this.automaticStartEnabled || !isClangdDocument(document))
      return null;
    return this.startForFolder(workspaceFolderForUri(document.uri));
  }

  private async workspaceFoldersChanged(
      _event: vscode.WorkspaceFoldersChangeEvent): Promise<void> {
    // Adding or removing a more-specific folder can transfer ownership of open
    // documents. Restart all clients so the old owner is stopped and the new
    // owner starts with didOpen.
    await Promise.all(
        Array.from(this.folders.values(), folder => this.remove(folder)));
    if (!this.automaticStartEnabled)
      return;
    await this.settle(vscode.workspace.textDocuments.map(
        document => this.ensureForDocument(document)));
  }

  async startForFolder(folder
                       ?: vscode.WorkspaceFolder): Promise<ClangdContext|null> {
    // A client without a folder has a broad selector, so it is only safe when
    // the window has no workspace folders.
    if (!folder && vscode.workspace.workspaceFolders?.length)
      return null;

    const key = this.key(folder);
    this.folders.set(key, folder);
    const existing = this.contexts.get(key);
    if (existing)
      return existing;
    const pending = this.starting.get(key);
    if (pending)
      return pending;

    const generation = this.generation.get(key) ?? 0;
    const starting = this.contextFactory(this.installer, this.globalStoragePath,
                                         this.outputChannel, this, folder);
    this.starting.set(key, starting);
    try {
      const context = await starting;
      if (context && generation === (this.generation.get(key) ?? 0)) {
        this.contexts.set(key, context);
        return context;
      }
      await context?.stop();
      return null;
    } finally {
      if (this.starting.get(key) === starting)
        this.starting.delete(key);
      this.updateActiveContext();
    }
  }

  private async remove(folder?: vscode.WorkspaceFolder): Promise<void> {
    const key = this.key(folder);
    this.generation.set(key, (this.generation.get(key) ?? 0) + 1);
    const pending = this.starting.get(key);
    this.starting.delete(key);
    this.folders.delete(key);
    const context = this.contexts.get(key);
    if (context) {
      this.contexts.delete(key);
      this.featureSupport.delete(context);
      this.removeContextEmitter.fire(context);
    }
    this.updateActiveContext();
    if (context) {
      await context.stop();
    } else if (pending) {
      try {
        await (await pending)?.stop();
      } catch {
        // Failed starts are already reported by the operation that created it.
      }
    }
  }

  private key(folder?: vscode.WorkspaceFolder): string {
    return folder?.uri.toString() ?? DEFAULT_CLIENT;
  }

  private runInBackground(task: Promise<unknown>): void {
    void task.catch(error => this.outputChannel.error(
                        'clangd client operation failed', error));
  }

  private async settle(tasks: Array<Promise<unknown>>): Promise<void> {
    for (const result of await Promise.allSettled(tasks)) {
      if (result.status === 'rejected')
        this.outputChannel.error('clangd client operation failed',
                                 result.reason);
    }
  }

  private updateActiveContext(): void {
    const active = this.activeContext();
    this.onActiveContextChanged(active);
    this.updateFeatureContexts(active);
  }

  private updateFeatureContexts(context?: ClangdContext): void {
    const supported = context ? this.featureSupport.get(context) : undefined;
    void vscode.commands.executeCommand('setContext', 'clangd.ast.supported',
                                        supported?.get('ast') ?? false);
    void vscode.commands.executeCommand('setContext',
                                        'clangd.inlayHints.supported',
                                        supported?.get('inlayHints') ?? false);
    void vscode.commands.executeCommand('setContext',
                                        'clangd.memoryUsage.supported',
                                        supported?.get('memoryUsage') ?? false);
    void vscode.commands.executeCommand(
        'setContext', 'clangd.enableTypeHierarchy',
        supported?.get('typeHierarchy') ?? false);
  }
}
