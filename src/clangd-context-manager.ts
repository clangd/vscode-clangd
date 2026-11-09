import * as vscode from 'vscode';

import {
  ClangdContext,
  isClangdDocument,
  registerFeatureCommands
} from './clangd-context';
import * as install from './install';
import {
  ClangdProject,
  ClangdProjectsChangeEvent,
  projectKey,
  projectLabel,
  projectName,
  ProjectSet
} from './project';
import {ServerCommandRegistry} from './server-commands';

// Returns a view of `channel` that drops all calls once closed() is true: a
// client still logs after it has stopped, e.g. its server's last stderr lines,
// and a disposed channel throws.
function guardedChannel(channel: vscode.LogOutputChannel,
                        closed: () => boolean): vscode.LogOutputChannel {
  return new Proxy(channel, {
    get(target, property) {
      const value = Reflect.get(target, property);
      if (typeof value !== 'function' || property === 'onDidChangeLogLevel')
        return value;
      return (...args: unknown[]) =>
                 closed() ? undefined : value.apply(target, args);
    }
  });
}

function multiProjectSetting(): boolean {
  return vscode.workspace.getConfiguration('clangd').get<boolean>(
             'enablePerFolderServer') ??
         false;
}

// Owns the clangd session: locates (or installs) clangd, creates the
// ClangdContexts and implements restart and shutdown.
//
// By default there is a single context serving all files. With
// clangd.enablePerFolderServer, every project gets its own context, created
// when one of its files is shown in an editor.
//
// The commands, views and status bar item of the features are registered once
// per session and find the context to act on through the manager. Only the
// manager creates and disposes contexts.
export class ClangdContextManager implements vscode.Disposable {
  // The contexts by project key; the single context of the default mode has
  // the key ''.
  private readonly contextMap = new Map<string, ClangdContext>();
  // Contexts being created, by project key. An entry is set before the
  // creation awaits anything, so that a project gets only one context.
  // Deleting the entry abandons the creation.
  private readonly pending =
      new Map<string, Promise<ClangdContext|undefined>>();
  // Disposables that live as long as the session, e.g. the install commands.
  // Undefined while there is no session (before start() and after stop()).
  private session: vscode.Disposable[]|undefined;
  // Set while the session starts: until clangd is located and the session
  // is set up.
  private starting: Promise<void>|undefined;
  // Set while all clients restart.
  private restarting: Promise<void>|undefined;
  private active: ClangdContext|undefined;
  private multiProjectEnabled = false;
  // The clangd executable, set once a multi-project session is set up.
  private clangdPath: string|undefined;
  // Projects shut down with clangd.shutdown, until they are restarted.
  private readonly stopped = new Set<string>();
  // The output channels of the projects, kept across restarts, and the views
  // of them given to the clients.
  private readonly channels = new Map<
      string,
      {channel: vscode.LogOutputChannel, guarded: vscode.LogOutputChannel}>();
  // The projects; kept across restarts, cleared in the default mode.
  private readonly projectSet: ProjectSet;
  // The server commands of the clients.
  readonly serverCommands =
      new ServerCommandRegistry((uri) => this.contextFor(uri)?.client);

  private readonly onDidAddContextEmitter =
      new vscode.EventEmitter<ClangdContext>();
  readonly onDidAddContext = this.onDidAddContextEmitter.event;
  private readonly onDidRemoveContextEmitter =
      new vscode.EventEmitter<ClangdContext>();
  readonly onDidRemoveContext = this.onDidRemoveContextEmitter.event;
  // Fired when a client is added or removed, or starts or stops running.
  private readonly onDidChangeClientsEmitter = new vscode.EventEmitter<void>();
  readonly onDidChangeClients = this.onDidChangeClientsEmitter.event;

  constructor(private readonly globalStoragePath: string,
              private readonly outputChannel: vscode.LogOutputChannel) {
    this.projectSet = new ProjectSet(outputChannel);
    this.projectSet.onDidChange((diff) => { this.projectsChanged(diff); });
  }

  // Whether every project gets its own clangd. Read when the session starts.
  get multiProject(): boolean { return this.multiProjectEnabled; }

  get contexts(): readonly ClangdContext[] {
    return Array.from(this.contextMap.values());
  }

  // The projects served in multi-project mode; empty otherwise.
  get projects(): readonly ClangdProject[] { return this.projectSet.projects; }

  projectFor(uri: vscode.Uri): ClangdProject|undefined {
    return this.projectSet.projectFor(uri);
  }

  // Returns the context serving `uri`. In the default mode, that is the single
  // context, whatever the uri.
  contextFor(uri: vscode.Uri|undefined): ClangdContext|undefined {
    if (!this.multiProject)
      return this.contextMap.get('');
    const project = uri && this.projectFor(uri);
    return project && this.contextMap.get(projectKey(project.uri));
  }

  // The context of the active editor if it shows a clangd document, else the
  // one of the clangd editor focused last. In the default mode, the single
  // context.
  get currentContext(): ClangdContext|undefined {
    const document = vscode.window.activeTextEditor?.document;
    return document && isClangdDocument(document)
               ? this.contextFor(document.uri)
               : this.active;
  }

  isRunning(): boolean { return this.contexts.some(c => c.clientIsRunning()); }

  isStarting(): boolean {
    return this.starting !== undefined ||
           this.contexts.some(c => c.clientIsStarting());
  }

  // Resolves once no start or restart of the session is in progress.
  async ready(): Promise<void> {
    let busy: Promise<void>|undefined;
    while ((busy = this.starting ?? this.restarting))
      await busy.catch(() => {});
  }

  async start(): Promise<void> {
    if (this.session)
      return;
    const session: vscode.Disposable[] = [];
    this.session = session;
    this.multiProjectEnabled = multiProjectSetting();
    this.outputChannel.info(`Starting clangd (per-folder servers: ${
        this.multiProject ? 'on' : 'off'})`);
    if (!this.multiProject) {
      this.projectSet.clear();
      this.disposeAllChannels();
    }
    const starting = this.startSession(session);
    this.starting = starting;
    try {
      await starting;
    } finally {
      if (this.starting === starting)
        this.starting = undefined;
    }
  }

  // Restarts all clients, or with `uri` in multi-project mode, the client of
  // the project containing `uri`.
  async restart(uri?: vscode.Uri): Promise<void> {
    if (uri && this.multiProject && this.session)
      return this.restartProject(uri);
    const restarting = this.restartAll(this.restarting ?? this.starting);
    this.restarting = restarting;
    try {
      await restarting;
    } finally {
      if (this.restarting === restarting)
        this.restarting = undefined;
    }
  }

  // Stops all clients, or with `uri` in multi-project mode, the client of the
  // project containing `uri`. That project stays stopped until it is
  // restarted.
  async stop(uri?: vscode.Uri): Promise<void> {
    if (uri && this.multiProject && this.session) {
      const project = this.projectFor(uri);
      if (!project)
        return;
      const key = projectKey(project.uri);
      this.stopped.add(key);
      this.removeContext(key);
      return;
    }
    if (this.startingInDefaultMode())
      return;
    this.stopSession();
  }

  // Returns the context of the project containing `uri`, creating and
  // starting it if needed. Returns undefined if there is no such project, or
  // it was stopped.
  async ensureContext(uri: vscode.Uri): Promise<ClangdContext|undefined> {
    if (!this.multiProject)
      return this.contextFor(uri);
    const project = this.projectFor(uri);
    if (!project || !this.clangdPath)
      return undefined;
    const key = projectKey(project.uri);
    if (this.stopped.has(key))
      return undefined;
    const context = this.contextMap.get(key);
    if (context)
      return context;
    if (!this.pending.has(key)) {
      const creation: Promise<ClangdContext|undefined> =
          this.createContext(project, () => this.pending.get(key) === creation);
      this.pending.set(key, creation);
    }
    // A creation that is abandoned, e.g. because the project restarts, may be
    // superseded by a newer one.
    let created: ClangdContext|undefined;
    while (!created && this.pending.has(key))
      created = await this.pending.get(key);
    return created ?? this.contextMap.get(key);
  }

  dispose() {
    this.stopSession();
    this.projectSet.dispose();
    this.disposeAllChannels();
    this.serverCommands.dispose();
    this.onDidAddContextEmitter.dispose();
    this.onDidRemoveContextEmitter.dispose();
    this.onDidChangeClientsEmitter.dispose();
  }

  private async restartAll(previous: Promise<void>|undefined) {
    // A restart requested while the session starts or restarts is served by
    // that start or restart, unless it was abandoned (e.g. by a shutdown) or
    // clangd.enablePerFolderServer changed meanwhile. Restarts thus run one
    // after the other and never set up sessions concurrently.
    if (previous) {
      await previous.catch(() => {});
      if (this.session && this.multiProject === multiProjectSetting())
        return;
    }
    if (this.multiProject === multiProjectSetting() &&
        this.startingInDefaultMode())
      return;
    // In multi-project mode, the contexts are recreated even if none of their
    // files is visible.
    const keys = [...this.contextMap.keys(), ...this.pending.keys()];
    this.stopSession();
    await this.start();
    if (this.multiProject)
      await Promise.all(
          this.projects.filter(p => keys.includes(projectKey(p.uri)))
              .map(p => this.ensureContext(p.uri)));
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

    if (this.multiProject) {
      this.clangdPath = clangdPath;
      session.push(...registerFeatureCommands(this),
                   vscode.window.onDidChangeVisibleTextEditors(
                       () => { this.ensureVisibleContexts(); }),
                   vscode.window.onDidChangeActiveTextEditor(
                       () => { this.updateActiveContext(); }),
                   new vscode.Disposable(() => this.projectSet.deactivate()));
      this.projectSet.activate();
      await this.ensureVisibleContexts();
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

  // Whether the default mode's session or client is starting, which blocks
  // restart() and stop() of all clients.
  //
  // clangd.restart can be called when the extension is not yet activated.
  // In such a case, vscode will activate the extension and then run this
  // handler. Detect this situation and bail out (doing an extra stop/start
  // cycle in this situation is pointless, and doesn't work anyways because
  // the client can't be stop()-ped when it's still in the Starting state).
  //
  // In multi-project mode, nothing blocks them: the session can be torn down
  // while it starts, and a context stops its client once it has started.
  private startingInDefaultMode(): boolean {
    return !this.multiProject && this.isStarting();
  }

  private projectsChanged(diff: ClangdProjectsChangeEvent) {
    // In the default mode, the projects are only cleared.
    if (!this.multiProject)
      return;
    this.outputChannel.info(
        'Projects: ' +
        (this.projects.map((p) => `${projectName(p)} (${p.uri.fsPath})`)
             .join(', ') ||
         'none'));
    for (const project of diff.removed) {
      const key = projectKey(project.uri);
      this.removeContext(key);
      this.disposeChannel(key);
      this.stopped.delete(key);
    }
    // An updated project's client restarts with the new name, in a channel
    // named after it.
    for (const project of diff.updated) {
      const key = projectKey(project.uri);
      const hadContext = this.removeContext(key);
      this.disposeChannel(key);
      if (hadContext)
        this.ensureContext(project.uri);
    }
    this.ensureVisibleContexts();
  }

  private async ensureVisibleContexts() {
    await Promise.all(vscode.window.visibleTextEditors
                          .filter(e => isClangdDocument(e.document))
                          .map(e => this.ensureContext(e.document.uri)));
  }

  private async restartProject(uri: vscode.Uri) {
    const project = this.projectFor(uri);
    if (!project)
      return;
    const key = projectKey(project.uri);
    this.stopped.delete(key);
    // A project that had a context gets a new one, even if none of its files
    // is visible.
    if (this.removeContext(key))
      await this.ensureContext(project.uri);
    else
      await this.ensureVisibleContexts();
  }

  // Creates and starts the context of `project`. isCurrent() tells whether
  // the creation is still wanted.
  private async createContext(project: ClangdProject, isCurrent: () => boolean):
      Promise<ClangdContext|undefined> {
    const key = projectKey(project.uri);
    const channel = this.channelFor(project, key);
    let context: ClangdContext;
    try {
      context =
          await ClangdContext.create(this.clangdPath!, channel, this, project);
    } catch (error) {
      if (isCurrent())
        this.pending.delete(key);
      this.outputChannel.error(
          `Failed to create the clangd client for ${project.uri}: ${error}`);
      return undefined;
    }
    // The creation was abandoned, e.g. because the project was removed or
    // stopped, or the session was torn down.
    if (!isCurrent()) {
      context.dispose();
      return undefined;
    }
    this.pending.delete(key);
    this.addContext(key, context);
    try {
      await context.startClient();
    } catch (error) {
      this.outputChannel.error(
          `Failed to start the clangd client for ${project.uri}: ${error}`);
    }
    return context;
  }

  private channelFor(project: ClangdProject,
                     key: string): vscode.LogOutputChannel {
    let entry = this.channels.get(key);
    if (!entry) {
      const channel = vscode.window.createOutputChannel(
          `clangd (${projectLabel(project, this.projects)})`, {log: true});
      entry = {
        channel,
        guarded: guardedChannel(
            channel, () => this.channels.get(key)?.channel !== channel)
      };
      this.channels.set(key, entry);
    }
    return entry.guarded;
  }

  // Disposes the channel of `key`. The clients writing to it may still be
  // stopping; their writes are dropped.
  private disposeChannel(key: string) {
    this.channels.get(key)?.channel.dispose();
    this.channels.delete(key);
  }

  private disposeAllChannels() {
    for (const key of Array.from(this.channels.keys()))
      this.disposeChannel(key);
  }

  private updateActiveContext() {
    const document = vscode.window.activeTextEditor?.document;
    const context = document && this.contextFor(document.uri);
    if (document && context?.servesDocument(document))
      this.setActive(context);
  }

  private setActive(context: ClangdContext|undefined) { this.active = context; }

  private addContext(key: string, context: ClangdContext) {
    this.contextMap.set(key, context);
    context.subscriptions.push(context.client.onDidChangeState(
        () => { this.onDidChangeClientsEmitter.fire(); }));
    if (!this.active)
      this.setActive(context);
    if (this.multiProject)
      this.updateActiveContext();
    this.onDidAddContextEmitter.fire(context);
  }

  // Disposes the context of `key`, or abandons its creation. Returns whether
  // there was either.
  private removeContext(key: string): boolean {
    const wasPending = this.pending.delete(key);
    const context = this.contextMap.get(key);
    if (!context)
      return wasPending;
    this.contextMap.delete(key);
    if (this.active === context)
      this.setActive(undefined);
    const hadClient = context.clientIsStartingOrRunning();
    this.onDidRemoveContextEmitter.fire(context);
    context.dispose();
    if (hadClient)
      this.onDidChangeClientsEmitter.fire();
    return true;
  }

  private stopSession() {
    for (const key of Array.from(this.contextMap.keys()))
      this.removeContext(key);
    this.pending.clear();
    this.stopped.clear();
    this.clangdPath = undefined;
    this.disposeSession();
  }

  private disposeSession() {
    this.session?.forEach((d) => { d.dispose(); });
    this.session = undefined;
  }
}
