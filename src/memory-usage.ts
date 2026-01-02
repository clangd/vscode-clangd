// Implements the "memory usage" feature.
// When the server advertises `memoryUsageProvider`, a command
// (clangd.memoryUsage) is available (context variable:
// clangd.memoryUsage.supported). It sends the $/memoryUsage request and
// displays the result in a tree view (clangd.memoryUsage) which becomes visible
// (context: clangd.memoryUsage.hasData)
//
// The tree view shows memory usage for all active contexts, with a top-level
// node per context (workspace folder or "Global" for the global context).

import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext} from './clangd-context';
import {ClangdContextManager} from './clangd-context-manager';

export function activate(manager: ClangdContextManager) {
  const provider = new MemoryUsageProvider(manager);
  manager.subscriptions.push(provider);

  for (const context of manager.getAllContexts()) {
    registerFeatureForContext(context);
  }

  manager.subscriptions.push(manager.onDidCreateContext(
      (context) => { registerFeatureForContext(context); }));
}

function registerFeatureForContext(context: ClangdContext) {
  const feature = new MemoryUsageFeature(context);
  context.client.registerFeature(feature);
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
  id: string; // unique identifier for tree item stability
  title: string;
  total: number;
  self: number;
  isFile: boolean;    // different icon, default collapsed
  isContext: boolean; // top-level context node
  children: InternalTree[];
}
function convert(m: WireTree, title: string, parentId: string = '',
                 isContext: boolean = false): InternalTree {
  const slash = Math.max(title.lastIndexOf('/'), title.lastIndexOf('\\'));
  const id = parentId ? `${parentId}/${title}` : title;
  return {
    id,
    title: slash >= 0 ? title.substr(slash + 1)
                      : title, // display basename only for files
    isFile: slash >= 0,
    isContext,
    total: m._total,
    self: m._self,
    children: Object.keys(m)
                  .sort()
                  .filter(x => !x.startsWith('_'))
                  .map(e => convert(m[e] as WireTree, e, id))
                  .sort((x, y) => y.total - x.total),
  };
}

class MemoryUsageFeature implements vscodelc.StaticFeature {
  constructor(context: ClangdContext) {
    context.subscriptions.push(context.client.onDidChangeState(stateChange => {
      this.recomputeMemoryUsageSupported(stateChange.newState);
    }));
  }

  fillClientCapabilities(capabilities: vscodelc.ClientCapabilities) {}
  fillInitializeParams(_params: vscodelc.InitializeParams) {}

  private serverSupportsMemoryUsage = false;

  initialize(capabilities: vscodelc.ServerCapabilities,
             _documentSelector: vscodelc.DocumentSelector|undefined) {
    this.serverSupportsMemoryUsage = 'memoryUsageProvider' in capabilities;
    this.recomputeMemoryUsageSupported(vscodelc.State.Running);
  }
  getState(): vscodelc.FeatureState { return {kind: 'static'}; }
  clear() {}

  private recomputeMemoryUsageSupported(state: vscodelc.State) {
    if (state === vscodelc.State.Running) {
      vscode.commands.executeCommand('setContext',
                                     'clangd.memoryUsage.supported',
                                     this.serverSupportsMemoryUsage);
    } else if (state === vscodelc.State.Stopped) {
      vscode.commands.executeCommand('setContext',
                                     'clangd.memoryUsage.supported', false);
    }
  }
}

class MemoryUsageProvider implements vscode.TreeDataProvider<InternalTree>,
                                     vscode.Disposable {
  private manager: ClangdContextManager;

  // Array of context roots, one per active context
  private roots_: InternalTree[] = [];
  private subscriptions: vscode.Disposable[] = [];

  private _onDidChangeTreeData = new vscode.EventEmitter<InternalTree|null>();
  readonly onDidChangeTreeData = this._onDidChangeTreeData.event;

  constructor(manager: ClangdContextManager) {
    this.manager = manager;

    this.subscriptions.push(
        vscode.window.registerTreeDataProvider('clangd.memoryUsage', this));

    this.subscriptions.push(this.onDidChangeTreeData(
        (e) => vscode.commands.executeCommand('setContext',
                                              'clangd.memoryUsage.hasData',
                                              this.roots_.length > 0)));

    this.subscriptions.push(
        vscode.commands.registerCommand('clangd.memoryUsage', async () => {
          const contexts = this.manager.getAllContexts();
          if (contexts.length === 0) {
            vscode.window.showInformationMessage(
                'No clangd instance available');
            return;
          }

          const roots: InternalTree[] = [];
          for (const context of contexts) {
            const contextName = context.workspaceFolder?.name ?? 'Global';

            if (!context.clientIsRunning()) {
              continue;
            }

            try {
              const usage =
                  await context.client.sendRequest(MemoryUsageRequest, {});
              // Skip contexts with no memory usage data
              if (!usage || usage._total === 0) {
                continue;
              }
              const tree = convert(usage, contextName, '', true);
              // Override title to use folder name or "Global"
              tree.title = contextName;
              roots.push(tree);
            } catch (e) {
              // Context might not support memory usage, skip it
              console.log(`Failed to get memory usage for context: ${e}`);
            }
          }

          if (roots.length === 0) {
            vscode.window.showInformationMessage(
                'No clangd instance supports memory usage');
            return;
          }

          this.roots = roots;
        }));

    this.subscriptions.push(vscode.commands.registerCommand(
        'clangd.memoryUsage.close', () => this.roots = []));
  }

  get roots(): InternalTree[] { return this.roots_; }
  set roots(n: InternalTree[]) {
    this.roots_ = n;
    this._onDidChangeTreeData.fire(/*root changed*/ null);
  }

  public getTreeItem(node: InternalTree): vscode.TreeItem {
    const item = new vscode.TreeItem(node.title);
    item.id = node.id;
    item.description = (node.total / 1024 / 1024).toFixed(2) + ' MB';
    item.tooltip = `self=${node.self} total=${node.total}`;
    if (node.isContext)
      item.iconPath = new vscode.ThemeIcon('folder');
    else if (node.isFile)
      item.iconPath = new vscode.ThemeIcon('symbol-file');
    else if (!node.children.length)
      item.iconPath = new vscode.ThemeIcon('circle-filled');
    if (node.children.length) {
      if (node.children.length >= 6 || node.isFile || node.isContext)
        item.collapsibleState = vscode.TreeItemCollapsibleState.Collapsed;
      else
        item.collapsibleState = vscode.TreeItemCollapsibleState.Expanded;
    }
    return item;
  }

  public async getChildren(t?: InternalTree): Promise<InternalTree[]> {
    if (!t)
      return this.roots_;
    return t.children;
  }

  dispose() {
    this.subscriptions.forEach(d => d.dispose());
    this.subscriptions.length = 0;
    this._onDidChangeTreeData.dispose();
  }
}
