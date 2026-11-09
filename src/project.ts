// The project model used when clangd.enablePerFolderServer is set: every
// project is a directory that is served by its own clangd instance.

import {createHash} from 'crypto';
import * as path from 'path';
import * as vscode from 'vscode';

// A directory served by its own clangd instance.
export interface ClangdProject {
  // The root directory. Files below it are served by the project's clangd.
  readonly uri: vscode.Uri;
  // Shown in the UI, e.g. in the output channel name. Defaults to the
  // basename of the root directory.
  readonly name?: string;
}

export interface ClangdProjectsChangeEvent {
  readonly added: readonly ClangdProject[];
  readonly removed: readonly ClangdProject[];
  // Projects whose name changed. Their clangd restarts.
  readonly updated: readonly ClangdProject[];
}

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
    else if (old.name !== project.name)
      updated.push(project);
  }
  const removed = oldProjects.filter(p => !newKeys.has(projectKey(p.uri)));
  return {added, removed, updated};
}

// The projects of the multi-project mode: one per workspace folder. While it
// is active, the set follows the workspace folders and reports the changes.
export class ProjectSet implements vscode.Disposable {
  private list: ClangdProject[] = [];
  private active = false;
  private readonly onDidChangeEmitter =
      new vscode.EventEmitter<ClangdProjectsChangeEvent>();
  readonly onDidChange = this.onDidChangeEmitter.event;
  private readonly subscriptions: vscode.Disposable[];

  constructor(private readonly log: vscode.LogOutputChannel) {
    this.subscriptions = [
      this.onDidChangeEmitter,
      vscode.workspace.onDidChangeWorkspaceFolders(() => { this.update(); })
    ];
  }

  get projects(): readonly ClangdProject[] { return this.list; }

  projectFor(uri: vscode.Uri): ClangdProject|undefined {
    return this.list.find((p) => uriContains(p.uri, uri));
  }

  // Updates the projects, and keeps them up to date until deactivate().
  activate() {
    this.active = true;
    this.update();
  }

  // Stops updating the projects; they are kept.
  deactivate() { this.active = false; }

  clear() {
    this.deactivate();
    this.set([]);
  }

  dispose() {
    this.subscriptions.forEach((d) => { d.dispose(); });
  }

  private update() {
    if (!this.active)
      return;
    const {kept, dropped} =
        normalizeProjects((vscode.workspace.workspaceFolders ??
                           []).map(f => ({uri: f.uri, name: f.name})));
    for (const project of dropped)
      this.log.info(
          `Ignoring project ${project.uri.toString()}: ` +
          'it is not a local directory, or it is a duplicate of or nested in ' +
          'another project');
    this.set(kept);
  }

  private set(projects: ClangdProject[]) {
    const diff = diffProjects(this.list, projects);
    this.list = projects;
    if (diff.added.length || diff.removed.length || diff.updated.length)
      this.onDidChangeEmitter.fire(diff);
  }
}
