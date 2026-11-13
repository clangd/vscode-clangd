// The project model used when clangd.enablePerFolderServer is set: every
// project is a directory that is served by its own clangd instance.

import {createHash} from 'crypto';
import * as path from 'path';
import * as vscode from 'vscode';

import {
  ClangdProject,
  ClangdProjectProvider,
  ClangdProjectsChangeEvent
} from '../api/vscode-clangd';

export function projectName(project: ClangdProject): string {
  return project.name ?? path.posix.basename(project.uri.path);
}

// The label of `project` in the UI: its name, or its path if another of
// `projects` has the same name.
export function projectLabel(project: ClangdProject,
                             projects: readonly ClangdProject[]): string {
  const name = projectName(project);
  const key = projectKey(project.uri);
  return projects.some(p =>
                           projectKey(p.uri) !== key && projectName(p) === name)
             ? project.uri.fsPath
             : name;
}

// The trace file of the clangd of `project`: `file` with the project's name
// and a hash of its root before the extension, e.g. trace.json becomes
// trace.<name>-<hash>.json. Projects with the same name get different files.
export function projectTraceFile(file: string, project: ClangdProject): string {
  const extension = path.extname(file);
  const name = projectName(project).replace(/[^\w.-]/g, '_');
  const hash = createHash('sha256')
                   .update(projectKey(project.uri))
                   .digest('hex')
                   .slice(0, 8);
  return `${file.slice(0, file.length - extension.length)}.${name}-${hash}${
      extension}`;
}

// Paths are compared case-insensitively where file systems usually are case
// insensitive. VS Code matches the RelativePatterns of the document selectors
// the same way.
const ignoreCase =
    process.platform === 'win32' || process.platform === 'darwin';
const normalize = (s: string) => ignoreCase ? s.toLowerCase() : s;

// Identifies a project by its root.
export function projectKey(uri: vscode.Uri): string {
  return normalize(uri.toString());
}

// Whether `child` is `parent` or below it. Paths are compared segment-wise, so
// /a/p does not contain /a/p2.
export function uriContains(parent: vscode.Uri, child: vscode.Uri): boolean {
  if (parent.scheme !== child.scheme ||
      parent.authority.toLowerCase() !== child.authority.toLowerCase())
    return false;
  const rel =
      path.posix.relative(normalize(parent.path), normalize(child.path));
  return rel === '' || (rel !== '..' && !rel.startsWith('../'));
}

// Drops the projects that can't get a clangd instance of their own: projects
// that aren't on disk, duplicates, and projects nested in another project.
// Only the outermost of nested projects is kept, because a client receives
// every document below its root. Of duplicates, the first one is kept.
export function normalizeProjects(projects: readonly ClangdProject[]):
    {kept: ClangdProject[]; dropped: ClangdProject[]} {
  // Stable, so duplicates stay in the order in which they were provided.
  const outermostFirst = projects.map((project, index) => ({project, index}))
                             .sort((a, b) => a.project.uri.path.length -
                                             b.project.uri.path.length);
  const kept: {project: ClangdProject, index: number}[] = [];
  const dropped: ClangdProject[] = [];
  for (const entry of outermostFirst) {
    const uri = entry.project.uri;
    if (uri.scheme !== 'file' ||
        kept.some(k => uriContains(k.project.uri, uri)))
      dropped.push(entry.project);
    else
      kept.push(entry);
  }
  kept.sort((a, b) => a.index - b.index);
  return {kept: kept.map(k => k.project), dropped};
}

export function diffProjects(oldProjects: readonly ClangdProject[],
                             newProjects: readonly ClangdProject[]):
    ClangdProjectsChangeEvent {
  const oldByKey = new Map(oldProjects.map(p => [projectKey(p.uri), p]));
  const newKeys = new Set(newProjects.map(p => projectKey(p.uri)));
  const added: ClangdProject[] = [];
  const updated: ClangdProject[] = [];
  for (const project of newProjects) {
    const old = oldByKey.get(projectKey(project.uri));
    if (!old)
      added.push(project);
    else if (old.name !== project.name ||
             JSON.stringify(old.arguments ?? []) !==
                 JSON.stringify(project.arguments ?? []))
      updated.push(project);
  }
  const removed = oldProjects.filter(p => !newKeys.has(projectKey(p.uri)));
  return {added, removed, updated};
}

// Checks the result of a provider. Throws if it is not a list of projects.
function validProjects(result: unknown): ClangdProject[] {
  if (!Array.isArray(result) ||
      !result.every(project => project?.uri instanceof vscode.Uri))
    throw new Error('the result is not a list of projects');
  return [...result];
}

interface ProviderEntry {
  provider: ClangdProjectProvider;
  listener?: vscode.Disposable;
  projects: ClangdProject[];
  queries: number;
}

// The projects of the multi-project mode: the union of the projects of the
// registered providers, or without providers, one project per workspace
// folder. While the set is active, it follows its sources and reports the
// changes.
export class ProjectSet implements vscode.Disposable {
  private list: ClangdProject[] = [];
  private active = false;
  // The registered providers, with the last projects each one provided and
  // the number of times it was queried.
  private readonly providers = new Map<string, ProviderEntry>();
  // Cancels the update in progress.
  private tokenSource: vscode.CancellationTokenSource|undefined;
  // The update in progress, or the last one. Never rejects.
  private updating: Promise<void> = Promise.resolve();
  private readonly onDidChangeEmitter =
      new vscode.EventEmitter<ClangdProjectsChangeEvent>();
  readonly onDidChange = this.onDidChangeEmitter.event;
  private readonly subscriptions: vscode.Disposable[];

  // A provider that fails or doesn't respond within `timeout` milliseconds is
  // considered to provide the projects it provided last.
  constructor(private readonly log: vscode.LogOutputChannel,
              private readonly timeout = 10000) {
    this.subscriptions = [
      this.onDidChangeEmitter,
      vscode.workspace.onDidChangeWorkspaceFolders(() => {
        if (this.providers.size === 0)
          this.update();
      })
    ];
  }

  get projects(): readonly ClangdProject[] { return this.list; }

  projectFor(uri: vscode.Uri): ClangdProject|undefined {
    return this.list.find((p) => uriContains(p.uri, uri));
  }

  // Adds a source of projects. Throws if `id` is already registered.
  register(id: string, provider: ClangdProjectProvider): vscode.Disposable {
    if (this.providers.has(id))
      throw new Error(`A project provider with id ${id} is already registered`);
    // Nothing is registered if this throws.
    const listener = provider.onDidChangeProjects?.(() => { this.update(); });
    this.providers.set(id, {provider, listener, projects: [], queries: 0});
    this.log.info(`Registered the project provider ${id}`);
    this.update();
    return new vscode.Disposable(() => {
      if (this.providers.get(id)?.provider !== provider)
        return;
      listener?.dispose();
      this.providers.delete(id);
      this.log.info(`Unregistered the project provider ${id}`);
      this.update();
    });
  }

  // Updates the projects, and keeps them up to date until deactivate().
  activate() {
    this.active = true;
    this.update();
  }

  // Stops updating the projects, and cancels an update in progress; the
  // projects are kept.
  deactivate() {
    this.active = false;
    this.cancel();
  }

  clear() {
    this.deactivate();
    this.set([]);
  }

  // Resolves once no update is in progress.
  async settled(): Promise<void> {
    let updating: Promise<void>;
    do {
      updating = this.updating;
      await updating;
    } while (updating !== this.updating);
  }

  dispose() {
    this.deactivate();
    this.providers.forEach(({listener}) => { listener?.dispose(); });
    this.providers.clear();
    this.subscriptions.forEach((d) => { d.dispose(); });
  }

  // Queries the sources and applies the result, unless the update is
  // superseded by a newer one or the set is deactivated meanwhile.
  private update() {
    if (!this.active)
      return;
    this.cancel();
    const tokenSource = new vscode.CancellationTokenSource();
    this.tokenSource = tokenSource;
    this.updating =
        this.query(tokenSource.token)
            .then((projects) => {
              if (tokenSource.token.isCancellationRequested)
                return;
              this.tokenSource = undefined;
              tokenSource.dispose();
              this.set(projects);
            })
            .catch((error) => {
              this.log.error(`Failed to update the projects: ${error}`);
            });
  }

  private cancel() {
    this.tokenSource?.cancel();
    this.tokenSource?.dispose();
    this.tokenSource = undefined;
  }

  private async query(token: vscode.CancellationToken):
      Promise<ClangdProject[]> {
    if (this.providers.size === 0)
      return (vscode.workspace.workspaceFolders ?? [])
          .map(f => ({uri: f.uri, name: f.name}));
    await Promise.all(Array.from(
        this.providers, ([id, entry]) => this.queryProvider(id, entry, token)));
    return this.providedProjects();
  }

  private providedProjects(): ClangdProject[] {
    return Array.from(this.providers.values()).flatMap(e => e.projects);
  }

  // Updates the projects of a provider, unless it fails or doesn't respond in
  // time. Stops waiting once `token` is cancelled.
  //
  // An answer after the timeout is applied right away, unless the provider was
  // queried again or the set was deactivated meanwhile: the provider has no
  // reason to fire onDidChangeProjects again.
  private async queryProvider(id: string, entry: ProviderEntry,
                              token: vscode.CancellationToken) {
    const query = ++entry.queries;
    let timer: NodeJS.Timeout|undefined;
    let cancellation: vscode.Disposable|undefined;
    const noAnswer = Symbol();
    const timeout = new Promise<typeof noAnswer>((resolve) => {
      timer = setTimeout(() => resolve(noAnswer), this.timeout);
      cancellation = token.onCancellationRequested(() => resolve(noAnswer));
    });
    try {
      const answer = Promise.resolve(entry.provider.provideProjects(token));
      const result = await Promise.race([answer, timeout]);
      if (result === noAnswer) {
        if (!token.isCancellationRequested) {
          this.log.error(`The project provider ${id} did not respond within ${
              this.timeout} ms; keeping its previous projects until it does`);
          answer.then((late) => this.applyLateAnswer(id, entry, query, late),
                      () => {});
        }
        return;
      }
      entry.projects = validProjects(result ?? []);
    } catch (error) {
      this.log.error(`The project provider ${id} failed: ${
          error}; keeping its previous projects`);
    } finally {
      clearTimeout(timer);
      cancellation?.dispose();
    }
  }

  private applyLateAnswer(id: string, entry: ProviderEntry, query: number,
                          result: readonly ClangdProject[]|null|undefined) {
    if (!this.active || this.providers.get(id) !== entry ||
        entry.queries !== query)
      return;
    try {
      entry.projects = validProjects(result ?? []);
    } catch (error) {
      this.log.error(`The project provider ${id} failed: ${error}`);
      return;
    }
    this.log.info(`The project provider ${id} responded`);
    this.set(this.providedProjects());
  }

  private set(projects: ClangdProject[]) {
    const {kept, dropped} = normalizeProjects(projects);
    for (const project of dropped)
      this.log.info(`Ignoring project ${project.uri.toString()}: ` +
                    'it is not a local directory, or it is a duplicate of or ' +
                    'nested in another project');
    const diff = diffProjects(this.list, kept);
    this.list = kept;
    if (diff.added.length || diff.removed.length || diff.updated.length)
      this.onDidChangeEmitter.fire(diff);
  }
}
