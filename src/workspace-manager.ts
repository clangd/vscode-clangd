import * as vscode from 'vscode';

import {ClangdContext, isClangdDocument} from './clangd-context';
import {
  ClangdClientProvider,
  ClangdFeature,
} from './client-provider';
import {ClangdInstaller} from './install';

const DEFAULT_CLIENT = '<default>';
export type ClangdContextFactory = typeof ClangdContext.create;

export class WorkspaceManager implements ClangdClientProvider,
                                         vscode.Disposable {
  private contexts = new Map<string, ClangdContext>();
  private starting = new Map<string, Promise<ClangdContext|null>>();
  private featureSupport =
      new Map<ClangdContext, Map<ClangdFeature, boolean>>();
  private folders = new Map<string, vscode.WorkspaceFolder|undefined>();
  private generation = new Map<string, number>();
  private disposables: vscode.Disposable[] = [];

  constructor(private installer: ClangdInstaller,
              private globalStoragePath: string,
              private outputChannel: vscode.LogOutputChannel,
              private onActiveContextChanged: (context: ClangdContext|
                                               undefined) => void,
              private contextFactory:
                  ClangdContextFactory = ClangdContext.create) {
    this.disposables.push(
        vscode.workspace.onDidOpenTextDocument(
            document => void this.ensureForDocument(document)),
        vscode.workspace.onDidChangeWorkspaceFolders(
            event => void this.workspaceFoldersChanged(event)),
        vscode.window.onDidChangeActiveTextEditor(
            () => { this.updateActiveContext(); }));
  }

  async start(): Promise<void> {
    await Promise.all(vscode.workspace.textDocuments.map(
        document => this.ensureForDocument(document)));
    this.updateActiveContext();
  }

  contextForUri(uri: vscode.Uri): ClangdContext|undefined {
    return this.contexts.get(
        this.key(vscode.workspace.getWorkspaceFolder(uri)));
  }

  activeContext(): ClangdContext|undefined {
    const uri = vscode.window.activeTextEditor?.document.uri;
    return uri ? this.contextForUri(uri) : undefined;
  }

  firstContext(): ClangdContext|undefined {
    return this.activeContext() ?? this.contexts.values().next().value;
  }

  async restart(folder?: vscode.WorkspaceFolder): Promise<void> {
    const targets = folder ? [folder] : Array.from(this.folders.values());
    if (!targets.length) {
      await this.start();
      return;
    }
    await Promise.all(targets.map(target => this.remove(target)));
    await Promise.all(targets.map(target => this.startForFolder(target)));
    this.updateActiveContext();
  }

  async shutdown(): Promise<void> {
    for (const key of this.folders.keys())
      this.generation.set(key, (this.generation.get(key) ?? 0) + 1);
    await Promise.all(
        Array.from(this.contexts.values(), context => context.stop()));
    this.contexts.clear();
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
    this.disposables = [];
  }

  private async ensureForDocument(document: vscode.TextDocument):
      Promise<ClangdContext|null> {
    if (!isClangdDocument(document))
      return null;
    return this.startForFolder(
        vscode.workspace.getWorkspaceFolder(document.uri));
  }

  private async workspaceFoldersChanged(
      event: vscode.WorkspaceFoldersChangeEvent): Promise<void> {
    await Promise.all(event.removed.map(folder => this.remove(folder)));
    // The default client's broad selector would overlap folder clients.
    if (event.added.length && vscode.workspace.workspaceFolders?.length)
      await this.remove(undefined);
    await Promise.all(vscode.workspace.textDocuments.map(
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
    this.starting.delete(key);
    this.folders.delete(key);
    const context = this.contexts.get(key);
    if (context) {
      this.contexts.delete(key);
      this.featureSupport.delete(context);
    }
    this.updateActiveContext();
    await context?.stop();
  }

  private key(folder?: vscode.WorkspaceFolder): string {
    return folder?.uri.toString() ?? DEFAULT_CLIENT;
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
