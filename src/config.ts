import {homedir} from 'os';
import * as path from 'path';
import * as vscode from 'vscode';

// Gets the config value `clangd.<key>`. Applies ${variable} substitutions.
export async function get<T>(key: string): Promise<T> {
  return await substitute(vscode.workspace.getConfiguration('clangd').get<T>(key)!);
}

// Sets the config value `clangd.<key>`. Does not apply substitutions.
export function update<T>(key: string, value: T,
                          target?: vscode.ConfigurationTarget) {
  return vscode.workspace.getConfiguration('clangd').update(key, value, target);
}

// Traverse a JSON value, replacing placeholders in all strings.
async function substitute<T>(val: T): Promise<T> {
  if (typeof val === 'string') {
    const replacementPattern = /\$\{(.*?)\}/g;
    const replacementPromises: Promise<string | undefined>[] = [];
    const matches = val.matchAll(replacementPattern);
    for (const match of matches) {
      // match[1] is the first captured group
      replacementPromises.push(replacement(match[1]));
    }
    const replacements = await Promise.all(replacementPromises);
    val = val.replace(replacementPattern,
      // If there's no replacement available, keep the placeholder.
      match => replacements.shift() ?? match) as unknown as T;
  } else if (Array.isArray(val)) {
    val = await Promise.all(val.map(substitute)) as T;
  } else if (typeof val === 'object') {
    // Substitute values but not keys, so we don't deal with collisions.
    const result = {} as {[k: string]: any};
    for (const key in val) {
      result[key] = await substitute(val[key]);
    }
    val = result as T;
  }
  return val;
}

// Subset of substitution variables that are most likely to be useful.
// https://code.visualstudio.com/docs/editor/variables-reference
async function replacement(name: string): Promise<string|undefined> {
  if (name === 'userHome') {
    return homedir();
  }
  if (name === 'workspaceRoot' || name === 'workspaceFolder' ||
      name === 'cwd') {
    if (vscode.workspace.rootPath !== undefined)
      return vscode.workspace.rootPath;
    if (vscode.window.activeTextEditor !== undefined)
      return path.dirname(vscode.window.activeTextEditor.document.uri.fsPath);
    return process.cwd();
  }
  if (name === 'workspaceFolderBasename' &&
      vscode.workspace.rootPath !== undefined) {
    return path.basename(vscode.workspace.rootPath);
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
    try {
      return await vscode.commands.executeCommand(name.substr(commandPrefix.length));
    } catch {
      return undefined;
    }
  }

  return undefined;
}
