import * as fs from 'fs';
import * as path from 'path';
import * as vscode from 'vscode';

// Returns the most specific workspace folder that owns a URI. VS Code's
// built-in lookup is lexical, so a file opened through a symlink may otherwise
// be assigned to the folder containing the symlink instead of the workspace
// folder containing the symlink target.
export function workspaceFolderForUri(
    uri: vscode.Uri,
    folders: readonly vscode.WorkspaceFolder[]|undefined =
        vscode.workspace.workspaceFolders): vscode.WorkspaceFolder|undefined {
  if (uri.scheme !== 'file' || !folders?.length)
    return vscode.workspace.getWorkspaceFolder(uri);

  const filePath = canonicalPath(uri.fsPath);
  let owner: vscode.WorkspaceFolder|undefined;
  let ownerLength = -1;
  for (const folder of folders) {
    if (folder.uri.scheme !== 'file')
      continue;
    const folderPath = canonicalPath(folder.uri.fsPath);
    if (!contains(folderPath, filePath) || folderPath.length <= ownerLength)
      continue;
    owner = folder;
    ownerLength = folderPath.length;
  }
  return owner ?? vscode.workspace.getWorkspaceFolder(uri);
}

export function folderOwnsUri(folder: vscode.WorkspaceFolder|undefined,
                              uri: vscode.Uri): boolean {
  return workspaceFolderForUri(uri)?.uri.toString() === folder?.uri.toString();
}

function canonicalPath(filePath: string): string {
  let current = path.resolve(filePath);
  const suffix: string[] = [];
  while (true) {
    try {
      return path.join(fs.realpathSync.native(current), ...suffix);
    } catch {
      const parent = path.dirname(current);
      if (parent === current)
        return path.resolve(filePath);
      suffix.unshift(path.basename(current));
      current = parent;
    }
  }
}

function contains(folderPath: string, filePath: string): boolean {
  const relative = path.relative(folderPath, filePath);
  return relative === '' || (!relative.startsWith(`..${path.sep}`) &&
                             relative !== '..' && !path.isAbsolute(relative));
}
