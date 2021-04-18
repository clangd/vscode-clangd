import * as path from 'path';
import * as vscode from 'vscode';

// Gets the config value `clangd.<key>`. Applies ${variable} substitutions.
export function get<T>(key: string, defaultValue: T): T {
  return substitute(
      vscode.workspace.getConfiguration('clangd').get(key, defaultValue));
}

// Like get(), but won't load settings from workspace config unless the user has
// previously explicitly allowed this.
export function getSecure<T>(key: string, workspaceState: vscode.Memento): T|
    undefined {
  const prop = new SecureProperty<T>(key, workspaceState);
  return prop.get(prop.blessed ?? false);
}

// Like get(), but won't implicitly load settings from workspace config.
// If there is workspace config, prompts the user and caches the decision.
export async function getSecureOrPrompt<T>(
    key: string, workspaceState: vscode.Memento): Promise<T|undefined> {
  const prop = new SecureProperty<T>(key, workspaceState);
  // Common case: value not overridden in workspace.
  if (!prop.mismatched)
    return prop.get(false);
  // Check whether user has blessed or blocked this value.
  const blessed = prop.blessed;
  if (blessed !== null)
    return prop.get(blessed);
  // No cached decision for this value, ask the user.
  const Yes = 'Yes, use this setting', No = 'No, use my default',
        Info = 'More Info'
  switch (await vscode.window.showWarningMessage(
      `This workspace wants to set clangd.${key} to ${prop.insecureJSON}.
    \u2029
    This will override your default of ${prop.secureJSON}.`,
      Yes, No, Info)) {
  case Info:
    vscode.env.openExternal(vscode.Uri.parse(
        'https://github.com/clangd/vscode-clangd/blob/master/docs/settings.md#security'));
    break;
  case Yes:
    await prop.bless(true);
    return prop.get(true);
  case No:
    await prop.bless(false);
  }
  return prop.get(false);
}

// Sets the config value `clangd.<key>`. Does not apply substitutions.
export function update<T>(key: string, value: T,
                          target?: vscode.ConfigurationTarget) {
  return vscode.workspace.getConfiguration('clangd').update(key, value, target);
}

// Traverse a JSON value, replacing placeholders in all strings.
function substitute<T>(val: T): T {
  if (typeof val == 'string') {
    val = val.replace(/\$\{(.*?)\}/g, (match, name) => {
      const rep = replacement(name);
      // If there's no replacement available, keep the placeholder.
      return (rep === null) ? match : rep;
    }) as unknown as T;
  } else if (Array.isArray(val))
    val = val.map((x) => substitute(x)) as unknown as T;
  else if (typeof val == 'object') {
    // Substitute values but not keys, so we don't deal with collisions.
    const result = {} as {[k: string]: any};
    for (let [k, v] of Object.entries(val))
      result[k] = substitute(v);
    val = result as T;
  }
  return val;
}

// Subset of substitution variables that are most likely to be useful.
// https://code.visualstudio.com/docs/editor/variables-reference
function replacement(name: string): string|null {
  if (name == 'workspaceRoot' || name == 'workspaceFolder' || name == 'cwd') {
    if (vscode.workspace.rootPath !== undefined)
      return vscode.workspace.rootPath;
    if (vscode.window.activeTextEditor !== undefined)
      return path.dirname(vscode.window.activeTextEditor.document.uri.fsPath);
    return process.cwd();
  }
  const envPrefix = 'env:';
  if (name.startsWith(envPrefix))
    return process.env[name.substr(envPrefix.length)] || '';
  const configPrefix = 'config:';
  if (name.startsWith(configPrefix)) {
    const config = vscode.workspace.getConfiguration().get(
        name.substr(configPrefix.length));
    return (typeof config == 'string') ? config : null;
  }

  return null;
}

// Caches a user's decision about whether to respect a workspace override of a
// sensitive setting. Valid only for a particular variable, workspace, and
// value.
interface BlessCache {
  json: string     // JSON-serialized workspace value that the decision governs.
  allowed: boolean // Whether the user chose to allow this value.
}

// Common logic between getSecure and getSecureOrPrompt.
class SecureProperty<T> {
  secure: T|undefined;
  insecure: T|undefined;
  public secureJSON: string;
  public insecureJSON: string;
  blessKey: string;

  constructor(key: string, private workspaceState: vscode.Memento) {
    const cfg = vscode.workspace.getConfiguration('clangd');
    const inspect = cfg.inspect<T>(key)!;
    this.secure = inspect.globalValue ?? inspect.defaultValue;
    this.insecure = cfg.get<T>(key);
    this.secureJSON = JSON.stringify(this.secure);
    this.insecureJSON = JSON.stringify(this.insecure);
    this.blessKey = 'bless.' + key;
  }

  get mismatched(): boolean { return this.secureJSON != this.insecureJSON; }

  get(trusted: boolean): T|undefined {
    return substitute(trusted ? this.insecure : this.secure);
  }

  get blessed(): boolean|null {
    let cache = this.workspaceState.get<BlessCache>(this.blessKey);
    if (!cache || cache.json != this.insecureJSON)
      return null;
    return cache.allowed;
  }

  async bless(b: boolean): Promise<void> {
    await this.workspaceState.update(this.blessKey,
                                     {json: this.insecureJSON, allowed: b});
  }
}
