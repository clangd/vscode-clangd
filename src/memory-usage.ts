// Implements the "memory usage" feature.
// When the server advertises `memoryUsageProvider`, a command
// (clangd.memoryUsage) is available (context variable:
// clangd.memoryUsage.supported). It sends the $/memoryUsage request and
// displays the result in a tree view (clangd.memoryUsage) which becomes visible
// (context: clangd.memoryUsage.hasData)

import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext, ClangdLanguageClient} from './clangd-context';

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

export class MemoryUsageFeature implements vscodelc.StaticFeature {
  constructor(client: ClangdLanguageClient) {
    client.registerFeature(this);
  }

  fillClientCapabilities(capabilities: vscodelc.ClientCapabilities) {}
  fillInitializeParams(_params: vscodelc.InitializeParams) {}

  initialize(capabilities: vscodelc.ServerCapabilities,
             _documentSelector: vscodelc.DocumentSelector|undefined) {
    if ('memoryUsageProvider' in capabilities)
      vscode.commands.executeCommand('setContext', 'clangd.memoryUsage.supported', true);
  }
  getState(): vscodelc.FeatureState { return {kind: 'static'}; }
  clear() {}
}

export class MemoryUsageProvider implements vscode.TreeDataProvider<InternalTree> {
  private root_?: InternalTree;

  constructor(context: ClangdContext) {
    context.subscriptions.push(this.onDidChangeTreeData((e) => vscode.commands.executeCommand(
      'setContext', 'clangd.memoryUsage.hasData',
      this.root !== undefined)));
    context.subscriptions.push(
        vscode.window.registerTreeDataProvider('clangd.memoryUsage', this));
    context.subscriptions.push(
        vscode.commands.registerCommand('clangd.memoryUsage', async () => {
          const usage =
              await context.getActiveClient()?.sendRequest(MemoryUsageRequest, {});
          if (usage !== undefined)
            this.root = convert(usage, '<root>');
        }));
    context.subscriptions.push(vscode.commands.registerCommand(
        'clangd.memoryUsage.close', () => this.root = undefined));
  }

  get root(): InternalTree|undefined { return this.root_; }
  set root(n: InternalTree|undefined) {
    this.root_ = n;
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
      return this.root ? [this.root] : [];
    return t.children;
  }
}
