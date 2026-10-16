import * as vscode from 'vscode';

import {ClangdContext, registerFeatureCommands} from './clangd-context';
import * as install from './install';

// Owns the clangd session: locates (or installs) clangd, creates the
// ClangdContext and implements restart and shutdown.
//
// The commands, views and status bar item of the features are registered once
// per session and find the context to act on through the manager. Only the
// manager creates and disposes contexts.
export class ClangdContextManager implements vscode.Disposable {
  private readonly contextMap = new Map<string, ClangdContext>();
  // Disposables that live as long as the session, e.g. the install commands.
  // Undefined while there is no session (before start() and after stop()).
  private session: vscode.Disposable[]|undefined;
  private starting: Promise<void>|undefined;
  private active: ClangdContext|undefined;

  private readonly onDidAddContextEmitter =
      new vscode.EventEmitter<ClangdContext>();
  readonly onDidAddContext = this.onDidAddContextEmitter.event;
  private readonly onDidRemoveContextEmitter =
      new vscode.EventEmitter<ClangdContext>();
  readonly onDidRemoveContext = this.onDidRemoveContextEmitter.event;

  constructor(private readonly globalStoragePath: string,
              private readonly outputChannel: vscode.LogOutputChannel) {}

  get contexts(): readonly ClangdContext[] {
    return Array.from(this.contextMap.values());
  }

  // Returns the context serving `uri`.
  contextFor(_uri: vscode.Uri|undefined): ClangdContext|undefined {
    return this.contextMap.get('');
  }

  get activeContext(): ClangdContext|undefined { return this.active; }

  isRunning(): boolean { return this.contexts.some(c => c.clientIsRunning()); }

  isStarting(): boolean {
    return this.starting !== undefined ||
           this.contexts.some(c => c.clientIsStarting());
  }

  async start(): Promise<void> {
    if (this.session)
      return;
    const session: vscode.Disposable[] = [];
    this.session = session;
    const starting = this.startSession(session);
    this.starting = starting;
    try {
      await starting;
    } finally {
      if (this.starting === starting)
        this.starting = undefined;
    }
  }

  async restart(): Promise<void> {
    // clangd.restart can be called when the extension is not yet activated.
    // In such a case, vscode will activate the extension and then run this
    // handler. Detect this situation and bail out (doing an extra stop/start
    // cycle in this situation is pointless, and doesn't work anyways because
    // the client can't be stop()-ped when it's still in the Starting state).
    if (this.isStarting())
      return;
    this.stopSession();
    await this.start();
  }

  async stop(): Promise<void> {
    if (this.isStarting())
      return;
    this.stopSession();
  }

  dispose() {
    this.stopSession();
    this.onDidAddContextEmitter.dispose();
    this.onDidRemoveContextEmitter.dispose();
  }

  private async startSession(session: vscode.Disposable[]) {
    const clangdPath = await install.activate(session, this.globalStoragePath);
    if (this.session !== session) {
      // The session was torn down meanwhile; dispose what was added since.
      session.forEach((d) => { d.dispose(); });
      return;
    }
    if (!clangdPath) {
      this.disposeSession();
      return;
    }

    const context =
        await ClangdContext.create(clangdPath, this.outputChannel, this);
    if (this.session !== session) {
      context.dispose();
      return;
    }
    session.push(...registerFeatureCommands(this));
    this.addContext('', context);
    await context.startClient();
  }

  private addContext(key: string, context: ClangdContext) {
    this.contextMap.set(key, context);
    this.active ??= context;
    this.onDidAddContextEmitter.fire(context);
  }

  private removeContext(key: string) {
    const context = this.contextMap.get(key);
    if (!context)
      return;
    this.contextMap.delete(key);
    if (this.active === context)
      this.active = undefined;
    this.onDidRemoveContextEmitter.fire(context);
    context.dispose();
  }

  private stopSession() {
    for (const key of Array.from(this.contextMap.keys()))
      this.removeContext(key);
    this.disposeSession();
  }

  private disposeSession() {
    this.session?.forEach((d) => { d.dispose(); });
    this.session = undefined;
  }
}
