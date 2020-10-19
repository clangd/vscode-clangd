// Implements the "ast dump" feature: textDocument/ast.

import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext} from './clangd-context';

export function activate(context: ClangdContext) {
  const feature = new ASTFeature(context);
  context.client.registerFeature(feature);
}

// The wire format: we send a position, and get back a tree of ASTNode.
namespace ASTRequest {
export const type =
    new vscodelc.RequestType<ASTParams, ASTNode|null, void, void>(
        'textDocument/ast');
}

interface ASTParams {
  textDocument: vscodelc.TextDocumentIdentifier;
  range: vscodelc.Range;
}

interface ASTNode {
  role: string;    // e.g. expression
  kind: string;    // e.g. BinaryOperator
  detail?: string; // e.g. ||
  arcana?: string; // e.g. BinaryOperator <0x12345> <col:12, col:1> 'bool' '||'
  children?: Array<ASTNode>;
  range?: vscodelc.Range;
}

class ASTFeature implements vscodelc.StaticFeature {
  constructor(private context: ClangdContext) {
    // The adapter holds the currently inspected node.
    const adapter = new TreeAdapter();
    // Ensure the AST view is visible exactly when the adapter has a node.
    // clangd.ast.hasData controls the view visibility (package.json).
    adapter
        .onDidChangeTreeData(
            (e) => vscode.commands.executeCommand(
                'setContext', 'clangd.ast.hasData', adapter.hasRoot()))
        // Create the AST view, showing data from the adapter.
        this.context.subscriptions.push(
            vscode.window.registerTreeDataProvider('clangd.ast', adapter));
    // Create the "Show AST" command for the context menu.
    // It's only shown if the feature is dynamicaly available (package.json)
    vscode.commands.registerTextEditorCommand(
        'clangd.ast', async (editor, _edit) => {
          const converter = this.context.client.code2ProtocolConverter;
          const item = await this.context.client.sendRequest(ASTRequest.type, {
            textDocument: converter.asTextDocumentIdentifier(editor.document),
            range: converter.asRange(editor.selection),
          });
          if (!item)
            vscode.window.showInformationMessage('No AST node at selection');
          adapter.setRoot(item, editor.document.uri);
        });
    // Clicking "close" will empty the adapter, which in turn hides the view.
    vscode.commands.registerCommand('clangd.ast.close',
                                    () => adapter.setRoot(null, null));
  }

  fillClientCapabilities(capabilities: vscodelc.ClientCapabilities) {}

  // The "Show AST" command is enabled if the server advertises the capability.
  initialize(capabilities: vscodelc.ServerCapabilities,
             _documentSelector: vscodelc.DocumentSelector|undefined) {
    vscode.commands.executeCommand('setContext', 'clangd.ast.supported',
                                   'astProvider' in capabilities);
  }
}

// Icons used for nodes of particular roles and kinds. (Kind takes precedence).
// IDs from https://code.visualstudio.com/api/references/icons-in-labels
const RoleIcons: {[role: string]: string} = {
  'type': 'symbol-misc',
  'declaration': 'symbol-function',
  'expression': 'primitive-dot',
  'specifier': 'list-tree',
  'statement': 'symbol-event',
  'template argument': 'symbol-type-parameter',
};
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
  if (role == 'expression' || role == 'statement' || role == 'declaration' ||
      role == 'template name')
    return kind;
  return kind + ' ' + role;
}

// Map a root ASTNode onto a VSCode tree.
class TreeAdapter implements vscode.TreeDataProvider<ASTNode> {
  private root?: ASTNode;
  private doc?: vscode.Uri;

  hasRoot(): boolean { return this.root != null; }

  setRoot(newRoot: ASTNode|null, newDoc: vscode.Uri|null) {
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
    if (node.range && this.doc)
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
    return item;
  }

  public async getChildren(element?: ASTNode): Promise<ASTNode[]> {
    if (!element)
      return this.root ? [this.root] : [];
    return element.children || [];
  }
}
