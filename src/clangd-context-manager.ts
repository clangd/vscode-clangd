import * as vscode from 'vscode';

import {ClangdContext, isClangdDocument} from './clangd-context';
import * as config from './config';
import * as fileStatus from './file-status';
import * as install from './install';

/**
 * Manages ClangdContext instances for multi-root workspace support.
 *
 * When enablePerFolderServer is false (default): Single global context handles
 * all folders When enablePerFolderServer is true: One context per workspace
 * folder
 */
export class ClangdContextManager implements vscode.Disposable {
  // When enablePerFolderServer is false: single global context
  private globalContext: ClangdContext|undefined = undefined;

  // Per-folder contexts for when enablePerFolderServer = true
  // Map key is folder.uri.toString()
  private folderContexts: Map<string, ClangdContext> = new Map();

  readonly subscriptions: vscode.Disposable[] = [];

  // Event emitters for context lifecycle
  private readonly _onDidCreateContext =
      new vscode.EventEmitter<ClangdContext>();
  readonly onDidCreateContext = this._onDidCreateContext.event;

  private readonly _onDidDisposeContext =
      new vscode.EventEmitter<ClangdContext>();
  readonly onDidDisposeContext = this._onDidDisposeContext.event;

  constructor(private readonly globalStoragePath: string) {}

  async activate(): Promise<void> {
    install.activate(this);

    fileStatus.activate(this);

    this.registerWorkspaceFolderHandlers();

    // Create contexts lazily based on already-open documents
    for (const document of vscode.workspace.textDocuments) {
      await this.maybeCreateContextForDocument(document);
    }
  }

  /**
   * Create a context for the document if:
   * - The document matches the clangd document selector
   * - No appropriate context exists yet
   *
   * In per-folder mode: creates a context for the document's workspace folder.
   * In global mode: creates the single global context.
   */
  private async maybeCreateContextForDocument(document: vscode.TextDocument):
      Promise<void> {
    if (!isClangdDocument(document)) {
      return;
    }

    if (await this.isPerFolderModeEnabled()) {
      const folder = vscode.workspace.getWorkspaceFolder(document.uri);
      if (!folder) {
        return;
      }

      const key = folder.uri.toString();
      if (this.folderContexts.has(key)) {
        return;
      }

      await this.createContext(folder);
    } else {
      if (this.globalContext) {
        return;
      }

      await this.createContext(null);
    }
  }

  /**
   * Check if per-folder mode is enabled (workspace-level setting).
   */
  private async isPerFolderModeEnabled(): Promise<boolean> {
    return config.get<boolean>('enablePerFolderServer');
  }

  /**
   * Create a context. If folder is provided, creates a per-folder context.
   * If folder is undefined, creates the global context.
   * Throws if a context already exists for the given folder.
   */
  async createContext(folder: vscode.WorkspaceFolder|
                      null): Promise<ClangdContext|undefined> {
    if (folder) {
      const key = folder.uri.toString();

      if (this.folderContexts.has(key)) {
        throw new Error(`A context already exists for folder: ${folder.name}`);
      }

      const context =
          await ClangdContext.create(this.globalStoragePath, folder);
      if (context) {
        this.folderContexts.set(key, context);
        this.subscriptions.push(context);
        this._onDidCreateContext.fire(context);
      }
      return context;
    } else {
      if (this.globalContext) {
        throw new Error('A global context already exists');
      }

      const context = await ClangdContext.create(this.globalStoragePath, null);
      if (context) {
        this.globalContext = context;
        this.subscriptions.push(context);
        this._onDidCreateContext.fire(context);
      }
      return context;
    }
  }

  disposeContext(folder: vscode.WorkspaceFolder|null): void {
    if (folder) {
      const key = folder.uri.toString();
      const context = this.folderContexts.get(key);
      if (context) {
        this._onDidDisposeContext.fire(context);
        context.dispose();
        this.folderContexts.delete(key);
      }
    } else {
      if (this.globalContext) {
        this._onDidDisposeContext.fire(this.globalContext);
        this.globalContext.dispose();
        this.globalContext = undefined;
      }
    }
  }

  getContextForDocument(document: vscode.TextDocument): ClangdContext
      |undefined {
    return this.getContextForUri(document.uri);
  }

  getContextForUri(uri: vscode.Uri): ClangdContext|undefined {
    const folder = vscode.workspace.getWorkspaceFolder(uri);
    if (folder) {
      return this.getContextForFolder(folder);
    }
    return this.globalContext ?? undefined;
  }

  getContextForFolder(folder: vscode.WorkspaceFolder): ClangdContext|undefined {
    const key = folder.uri.toString();
    const perFolderContext = this.folderContexts.get(key);
    if (perFolderContext) {
      return perFolderContext;
    }
    return this.globalContext ?? undefined;
  }

  getActiveContext(): ClangdContext|undefined {
    const activeEditor = vscode.window.activeTextEditor;
    if (activeEditor) {
      return this.getContextForDocument(activeEditor.document);
    }
    return this.globalContext ?? undefined;
  }

  getAllContexts(): ClangdContext[] {
    const contexts: ClangdContext[] = [];
    if (this.globalContext) {
      contexts.push(this.globalContext);
    }
    for (const context of this.folderContexts.values()) {
      contexts.push(context);
    }
    return contexts;
  }

  private registerWorkspaceFolderHandlers(): void {
    // Handle document open events to lazily create contexts
    this.subscriptions.push(
        vscode.workspace.onDidOpenTextDocument(async (document) => {
          await this.maybeCreateContextForDocument(document);
        }));

    this.subscriptions.push(
        vscode.workspace.onDidChangeWorkspaceFolders(async (event) => {
          if (!(await this.isPerFolderModeEnabled())) {
            return;
          }

          // Note: We don't create contexts for added folders here.
          // Contexts are created lazily when a matching document is opened.

          for (const folder of event.removed) {
            this.disposeContext(folder);
          }
        }));

    this.subscriptions.push(
        vscode.workspace.onDidChangeConfiguration(async (event) => {
          if (event.affectsConfiguration('clangd.enablePerFolderServer')) {
            // Mode changed - need to dispose all contexts
            const folders = vscode.workspace.workspaceFolders || [];
            const perFolderMode = await this.isPerFolderModeEnabled();

            // Note we only dispose contexts here, contexts will be created
            // lazily as needed.

            if (perFolderMode) {
              if (this.globalContext) {
                this.disposeContext(null);
              }
            } else {
              for (const folder of folders) {
                this.disposeContext(folder);
              }
            }

            // Create contexts for already-open documents
            for (const document of vscode.workspace.textDocuments) {
              await this.maybeCreateContextForDocument(document);
            }
          }
        }));
  }

  isAnyContextStarting(): boolean {
    if (this.globalContext?.clientIsStarting()) {
      return true;
    }
    for (const context of this.folderContexts.values()) {
      if (context.clientIsStarting()) {
        return true;
      }
    }
    return false;
  }

  isAnyContextRunning(): boolean {
    if (this.globalContext?.clientIsRunning()) {
      return true;
    }
    for (const context of this.folderContexts.values()) {
      if (context.clientIsRunning()) {
        return true;
      }
    }
    return false;
  }

  dispose(): void {
    if (this.globalContext) {
      this.globalContext.dispose();
      this.globalContext = undefined;
    }
    for (const context of this.folderContexts.values()) {
      context.dispose();
    }
    this.folderContexts.clear();

    this.subscriptions.forEach((d) => d.dispose());
    this.subscriptions.length = 0;

    this._onDidCreateContext.dispose();
    this._onDidDisposeContext.dispose();
  }
}
