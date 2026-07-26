import {homedir} from 'os';
import * as path from 'path';
import * as vscode from 'vscode';

// Gets the config value `clangd.<key>`. Applies ${variable} substitutions.
// If folder is provided, gets the configuration scoped to that workspace
// folder.
export async function get<T>(key: string,
                             folder?: vscode.WorkspaceFolder|null): Promise<T> {
  return await substitute(
      vscode.workspace.getConfiguration('clangd', folder).get<T>(key)!, folder);
}

// Sets the config value `clangd.<key>`. Does not apply substitutions.
// If folder is provided, updates the configuration scoped to that workspace
// folder.
export function update<T>(key: string, value: T,
                          target?: vscode.ConfigurationTarget,
                          folder?: vscode.WorkspaceFolder|null) {
  return vscode.workspace.getConfiguration('clangd', folder)
      .update(key, value, target);
}

// Traverse a JSON value, replacing placeholders in all strings.
async function substitute<T>(val: T,
                             folder?: vscode.WorkspaceFolder|null): Promise<T> {
  if (typeof val === 'string') {
    const replacementPattern = /\$\{(.*?)\}/g;
    const replacementPromises: Promise<string|undefined>[] = [];
    const matches = val.matchAll(replacementPattern);
    for (const match of matches) {
      // match[1] is the first captured group
      replacementPromises.push(replacement(match[1], folder));
    }
    const replacements = await Promise.all(replacementPromises);
    val = val.replace(
              replacementPattern,
              // If there's no replacement available, keep the placeholder.
              match => replacements.shift() ?? match) as unknown as T;
  } else if (Array.isArray(val)) {
    val = await Promise.all(val.map(v => substitute(v, folder))) as T;
  } else if (typeof val === 'object') {
    // Substitute values but not keys, so we don't deal with collisions.
    const result = {} as {[k: string]: any};
    for (const key in val) {
      result[key] = await substitute(val[key], folder);
    }
    val = result as T;
  }
  return val;
}

// Subset of substitution variables that are most likely to be useful.
// https://code.visualstudio.com/docs/editor/variables-reference
async function replacement(name: string, folder?: vscode.WorkspaceFolder|
                                         null): Promise<string|undefined> {
  if (name === 'userHome') {
    return homedir();
  }
  if (name === 'workspaceRoot' || name === 'workspaceFolder' ||
      name === 'cwd') {
    // If a folder is provided, use it. Otherwise, fall back to legacy
    // behavior.
    if (folder != null) {
      return folder.uri.fsPath;
    }
    if (vscode.workspace.workspaceFolders !== undefined &&
        vscode.workspace.workspaceFolders.length > 0) {
      return vscode.workspace.workspaceFolders[0].uri.fsPath;
    }
    if (vscode.window.activeTextEditor !== undefined)
      return path.dirname(vscode.window.activeTextEditor.document.uri.fsPath);
    return process.cwd();
  }
  if (name === 'workspaceFolderBasename') {
    if (folder != null) {
      return folder.name;
    }
    if (vscode.workspace.workspaceFolders !== undefined &&
        vscode.workspace.workspaceFolders.length > 0) {
      return vscode.workspace.workspaceFolders[0].name;
    }
    return undefined;
  }
  const envPrefix = 'env:';
  if (name.startsWith(envPrefix))
    return process.env[name.substr(envPrefix.length)] ?? '';
  const configPrefix = 'config:';
  if (name.startsWith(configPrefix)) {
    const config = vscode.workspace.getConfiguration().get(
        name.substr(configPrefix.length));
    return (typeof config === 'string') ? config : undefined;
  }
  const commandPrefix = 'command:';
  if (name.startsWith(commandPrefix)) {
    const commandId = name.substr(commandPrefix.length);
    try {
      return await vscode.commands.executeCommand(commandId);
    } catch (error) {
      console.warn(`Clangd: Error resolving command '${commandId}':`, error);
      vscode.window.showWarningMessage(
          `Clangd: Failed to resolve ${commandId}`);

      return undefined;
    }
  }

  return undefined;
}
