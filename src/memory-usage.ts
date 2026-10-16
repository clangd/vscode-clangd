// Implements the "memory usage" feature.
// When the server advertises `memoryUsageProvider`, a command
// (clangd.memoryUsage) is available (context variable:
// clangd.memoryUsage.supported). It sends the $/memoryUsage request and
// displays the result in a tree view (clangd.memoryUsage) which becomes visible
// (context: clangd.memoryUsage.hasData)

import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext} from './clangd-context';
import type {ClangdContextManager} from './clangd-context-manager';

export function activate(context: ClangdContext) {
  const feature = new MemoryUsageFeature();
  context.client.registerFeature(feature);
}

// Creates the memory usage view and its commands, shared by all contexts.
export function registerCommands(manager: ClangdContextManager):
    vscode.Disposable[] {
  const adapter = new TreeAdapter();
  return [
    adapter.onDidChangeTreeData((e) => vscode.commands.executeCommand(
                                    'setContext', 'clangd.memoryUsage.hasData',
                                    adapter.roots !== undefined)),
    vscode.window.registerTreeDataProvider('clangd.memoryUsage', adapter),
    vscode.commands.registerCommand(
        'clangd.memoryUsage',
        async () => {
          const contexts =
              manager.contexts.filter(c => c.clientIsStartingOrRunning());
          if (!contexts.length) {
            vscode.window.showInformationMessage('clangd is not running');
            return;
          }
          // One top-level node per client. A client whose request fails,
          // e.g. because it is stopping, is left out, unless all requests
          // fail.
          const results = await Promise.allSettled(contexts.map(
              async (c) =>
                  convert(await c.client.sendRequest(MemoryUsageRequest, {}),
                          '<root>')));
          const roots =
              results.flatMap(r => r.status === 'fulfilled' ? [r.value] : []);
          if (!roots.length)
            throw (results[0] as PromiseRejectedResult).reason;
          adapter.roots = roots;
        }),
    vscode.commands.registerCommand('clangd.memoryUsage.close',
                                    () => adapter.roots = undefined),
  ];
}

// LSP wire format for this clangd feature.
interface NoParams {}
interface WireTree {
  _self: number;
  _total: number;
  [child: string]: WireTree|number;
}
export const MemoryUsageRequest =
    new vscodelc.RequestType<NoParams, WireTree, void>('$/memoryUsage');

// Internal representation that's a bit easier to work with.
interface InternalTree {
  title: string;
  total: number;
  self: number;
  isFile: boolean; // different icon, default collapsed
  children: InternalTree[];
}
function convert(m: WireTree, title: string): InternalTree {
  const slash = Math.max(title.lastIndexOf('/'), title.lastIndexOf('\\'));
  return {
    title: title.substr(slash + 1), // display basename only
    isFile: slash >= 0,
    total: m._total,
    self: m._self,
    children: Object.keys(m)
                  .sort()
                  .filter(x => !x.startsWith('_'))
                  .map(e => convert(m[e] as WireTree, e))
                  .sort((x, y) => y.total - x.total),
  };
}

class MemoryUsageFeature implements vscodelc.StaticFeature {
  fillClientCapabilities(capabilities: vscodelc.ClientCapabilities) {}
  fillInitializeParams(_params: vscodelc.InitializeParams) {}

  initialize(capabilities: vscodelc.ServerCapabilities,
             _documentSelector: vscodelc.DocumentSelector|undefined) {
    vscode.commands.executeCommand('setContext', 'clangd.memoryUsage.supported',
                                   'memoryUsageProvider' in capabilities);
  }
  getState(): vscodelc.FeatureState { return {kind: 'static'}; }
  clear() {}
}

class TreeAdapter implements vscode.TreeDataProvider<InternalTree> {
  private roots_?: InternalTree[];

  get roots(): InternalTree[]|undefined { return this.roots_; }
  set roots(n: InternalTree[]|undefined) {
    this.roots_ = n;
    this._onDidChangeTreeData.fire(/*root changed*/ null);
  }

  private _onDidChangeTreeData = new vscode.EventEmitter<InternalTree|null>();
  readonly onDidChangeTreeData = this._onDidChangeTreeData.event;

  public getTreeItem(node: InternalTree): vscode.TreeItem {
    const item = new vscode.TreeItem(node.title);
    item.description = (node.total / 1024 / 1024).toFixed(2) + ' MB';
    item.tooltip = `self=${node.self} total=${node.total}`;
    if (node.isFile)
      item.iconPath = new vscode.ThemeIcon('symbol-file');
    else if (!node.children.length)
      item.iconPath = new vscode.ThemeIcon('circle-filled');
    if (node.children.length) {
      if (node.children.length >= 6 || node.isFile)
        item.collapsibleState = vscode.TreeItemCollapsibleState.Collapsed;
      else
        item.collapsibleState = vscode.TreeItemCollapsibleState.Expanded;
    }
    return item;
  }

  public async getChildren(t?: InternalTree): Promise<InternalTree[]> {
    if (!t)
      return this.roots ?? [];
    return t.children;
  }
}
