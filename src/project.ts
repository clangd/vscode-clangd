// The project model used when clangd.enablePerFolderServer is set: every
// project is a directory that is served by its own clangd instance.

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
