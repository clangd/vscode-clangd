// Implements the "ast dump" feature: textDocument/ast.

import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import type {ASTNode, ASTParams} from '../api/vscode-clangd';

import {ClangdContext} from './clangd-context';
import {ClangdContextManager} from './clangd-context-manager';

const ASTRequestMethod = 'textDocument/ast';

export function activate(manager: ClangdContextManager) {
  const provider = new ASTProvider(manager);
  manager.subscriptions.push(provider);

  for (const context of manager.getAllContexts()) {
    registerFeatureForContext(context);
  }

  manager.subscriptions.push(manager.onDidCreateContext(
      (context) => { registerFeatureForContext(context); }));
}

function registerFeatureForContext(context: ClangdContext) {
  const feature = new ASTFeature(context);
  context.client.registerFeature(feature);
}

const ASTRequestType =
    new vscodelc.RequestType<ASTParams, ASTNode|null, void>(ASTRequestMethod);

class ASTFeature implements vscodelc.StaticFeature {
  constructor(context: ClangdContext) {
    context.subscriptions.push(context.client.onDidChangeState(
        stateChange => { this.recomputeASTSupported(stateChange.newState); }));
  }

  fillClientCapabilities(capabilities: vscodelc.ClientCapabilities) {}

  private serverSupportsAST = false;

  // The "Show AST" command is enabled if the server advertises the capability.
  initialize(capabilities: vscodelc.ServerCapabilities,
             _documentSelector: vscodelc.DocumentSelector|undefined) {
    this.serverSupportsAST = 'astProvider' in capabilities;
    this.recomputeASTSupported(vscodelc.State.Running);
  }
  getState(): vscodelc.FeatureState { return {kind: 'static'}; }
  clear() {}

  private recomputeASTSupported(state: vscodelc.State) {
    if (state === vscodelc.State.Running) {
      vscode.commands.executeCommand('setContext', 'clangd.ast.supported',
                                     this.serverSupportsAST);
    } else if (state === vscodelc.State.Stopped) {
      vscode.commands.executeCommand('setContext', 'clangd.ast.supported',
                                     false);
    }
  }
}

// Icons used for nodes of particular roles and kinds. (Kind takes precedence).
// IDs from https://code.visualstudio.com/api/references/icons-in-labels
// We're uncomfortably coupled to the concrete roles and kinds from clangd:
// https://github.com/llvm/llvm-project/blob/main/clang-tools-extra/clangd/DumpAST.cpp

// There are only a few roles, corresponding to base AST node types.
const RoleIcons: {[role: string]: string} = {
  'type': 'symbol-misc',
  'declaration': 'symbol-function',
  'expression': 'primitive-dot',
  'specifier': 'list-tree',
  'statement': 'symbol-event',
  'template argument': 'symbol-type-parameter',
};
// Kinds match Stmt::StmtClass etc, corresponding to AST node subtypes.
// In principle these could overlap, but in practice they don't.
const KindIcons: {[type: string]: string} = {
  'Compound': 'json',
  'Recovery': 'error',
  'TranslationUnit': 'file-code',
  'PackExpansion': 'ellipsis',
  'TemplateTypeParm': 'symbol-type-parameter',
  'TemplateTemplateParm': 'symbol-type-parameter',
  'TemplateParamObject': 'symbol-type-parameter',
};
// Primary text shown for this node.
function describe(role: string, kind: string): string {
  // For common roles where the kind is fairly self-explanatory, we don't
  // include it. e.g. "Call" rather than "Call expression".
  if (role === 'expression' || role === 'statement' || role === 'declaration' ||
      role === 'template name')
    return kind;
  return kind + ' ' + role;
}

// Map a root ASTNode onto a VSCode tree.
class TreeAdapter implements vscode.TreeDataProvider<ASTNode> {
  private root?: ASTNode;
  private doc?: vscode.Uri;

  hasRoot(): boolean { return this.root !== undefined; }

  setRoot(newRoot: ASTNode|undefined, newDoc: vscode.Uri|undefined) {
    this.root = newRoot;
    this.doc = newDoc;
    this._onDidChangeTreeData.fire(/*root changed*/ null);
  }

  private _onDidChangeTreeData = new vscode.EventEmitter<ASTNode|null>();
  readonly onDidChangeTreeData = this._onDidChangeTreeData.event;

  public getTreeItem(node: ASTNode): vscode.TreeItem {
    const item = new vscode.TreeItem(describe(node.role, node.kind));
    if (node.children && node.children.length > 0)
      item.collapsibleState = vscode.TreeItemCollapsibleState.Expanded;
    item.description = node.detail;
    item.tooltip = node.arcana;
    const icon = KindIcons[node.kind] || RoleIcons[node.role];
    if (icon)
      item.iconPath = new vscode.ThemeIcon(icon);

    // Clicking on the node should highlight it in the source.
    if (node.range && this.doc) {
      item.command = {
        title: 'Jump to',
        command: 'vscode.open',
        arguments: [
          this.doc, {
            preserveFocus: true,
            selection: node.range,
          } as vscode.TextDocumentShowOptions
        ],
      };
    }
    return item;
  }

  public getChildren(element?: ASTNode): ASTNode[] {
    if (!element)
      return this.root ? [this.root] : [];
    return element.children || [];
  }

  public getParent(node: ASTNode): ASTNode|undefined {
    if (node === this.root)
      return undefined;
    function findUnder(parent: ASTNode|undefined): ASTNode|undefined {
      for (const child of parent?.children ?? []) {
        const result = (node === child) ? parent : findUnder(child);
        if (result)
          return result;
      }
      return undefined;
    }
    return findUnder(this.root);
  }
}

class ASTProvider implements vscode.Disposable {
  private manager: ClangdContextManager;
  private adapter: TreeAdapter;
  private subscriptions: vscode.Disposable[] = [];

  constructor(manager: ClangdContextManager) {
    this.manager = manager;

    // The adapter holds the currently inspected node.
    this.adapter = new TreeAdapter();

    // Create the AST view, showing data from the adapter.
    const tree = vscode.window.createTreeView('clangd.ast',
                                              {treeDataProvider: this.adapter});
    this.subscriptions.push(
        tree,
        // Ensure the AST view is visible exactly when the adapter has a node.
        // clangd.ast.hasData controls the view visibility (package.json).
        this.adapter.onDidChangeTreeData((_) => {
          vscode.commands.executeCommand('setContext', 'clangd.ast.hasData',
                                         this.adapter.hasRoot());
          // Work around https://github.com/microsoft/vscode/issues/90005
          // Show the AST tree even if it's been collapsed or closed.
          // reveal(root) fails here: "Data tree node not found".
          if (this.adapter.hasRoot())
            // @ts-ignore
            tree.reveal(null);
        }),
        // Create the "Show AST" command for the context menu.
        // It's only shown if the feature is dynamically available
        // (package.json)
        vscode.commands.registerTextEditorCommand(
            'clangd.ast',
            async (editor, _edit) => {
              // Get the context for the document being edited
              const context =
                  this.manager.getContextForDocument(editor.document);
              if (!context) {
                vscode.window.showInformationMessage(
                    'No clangd instance available for this document');
                return;
              }

              const converter = context.client.code2ProtocolConverter;
              const item = await context.client.sendRequest(ASTRequestType, {
                textDocument:
                    converter.asTextDocumentIdentifier(editor.document),
                range: converter.asRange(editor.selection),
              });
              if (!item)
                vscode.window.showInformationMessage(
                    'No AST node at selection');
              this.adapter.setRoot(item ?? undefined, editor.document.uri);
            }),
        // Clicking "close" will empty the adapter, which in turn hides the
        // view.
        vscode.commands.registerCommand(
            'clangd.ast.close',
            () => this.adapter.setRoot(undefined, undefined)));
  }

  dispose() {
    this.subscriptions.forEach(d => d.dispose());
    this.subscriptions.length = 0;
  }
}
