// This file implements the client side of the proposed type hierarchy
// extension to LSP. The proposal can be found at
// https://github.com/microsoft/vscode-languageserver-node/pull/426.
// Clangd supports the server side of this protocol.
// The feature allows querying the base and derived classes of the
// symbol under the cursor, which are visualized in a tree view.

import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

export function activate(client: vscodelc.LanguageClient,
                         context: vscode.ExtensionContext) {
  new TypeHierarchyProvider(context, client);
}

export namespace TypeHierarchyDirection {
export const Children = 0;
export const Parents = 1;
export const Both = 2;
}

type TypeHierarchyDirection = 0|1|2;

interface TypeHierarchyParams extends vscodelc.TextDocumentPositionParams {
  resolve?: number;
  direction: TypeHierarchyDirection;
}

interface TypeHierarchyItem {
  name: string;
  detail?: string;
  kind: vscodelc.SymbolKind;
  deprecated?: boolean;
  uri: string;
  range: vscodelc.Range;
  selectionRange: vscodelc.Range;
  parents?: TypeHierarchyItem[];
  children?: TypeHierarchyItem[];
  data?: any;
}

namespace TypeHierarchyRequest {
export const type =
    new vscodelc
        .RequestType<TypeHierarchyParams, TypeHierarchyItem|null, void, void>(
            'textDocument/typeHierarchy');
}

interface ResolveTypeHierarchyItemParams {
  item: TypeHierarchyItem;
  resolve: number;
  direction: TypeHierarchyDirection;
}

export namespace ResolveTypeHierarchyRequest {
export const type =
    new vscodelc.RequestType<ResolveTypeHierarchyItemParams,
                             TypeHierarchyItem|null, void, void>(
        'typeHierarchy/resolve');
}

class TypeHierarchyTreeItem extends vscode.TreeItem {
  constructor(item: TypeHierarchyItem) {
    super(item.name);
    if (item.children) {
      if (item.children.length == 0) {
        this.collapsibleState = vscode.TreeItemCollapsibleState.None;
      } else {
        this.collapsibleState = vscode.TreeItemCollapsibleState.Expanded;
      }
    } else {
      this.collapsibleState = vscode.TreeItemCollapsibleState.Collapsed;
    }

    // Make the item respond to a single-click by navigating to the
    // definition of the class.
    this.command = {
      arguments: [item],
      command: 'clangd.typeHierarchy.gotoItem',
      title: 'Go to'
    };
  }
}

class TypeHierarchyProvider implements
    vscode.TreeDataProvider<TypeHierarchyItem> {

  private _onDidChangeTreeData =
      new vscode.EventEmitter<TypeHierarchyItem | null>();
  readonly onDidChangeTreeData = this._onDidChangeTreeData.event;

  private root?: TypeHierarchyItem;
  private direction: TypeHierarchyDirection;
  private treeView: vscode.TreeView<TypeHierarchyItem>;

  constructor(context: vscode.ExtensionContext,
              private client: vscodelc.LanguageClient) {
    context.subscriptions.push(vscode.window.registerTreeDataProvider(
        'clangd.typeHierarchyView', this));

    context.subscriptions.push(vscode.commands.registerTextEditorCommand(
        'clangd.typeHierarchy', this.reveal, this));
    context.subscriptions.push(vscode.commands.registerCommand(
        'clangd.typeHierarchy.close', this.close, this));
    context.subscriptions.push(vscode.commands.registerCommand(
        'clangd.typeHierarchy.gotoItem', this.gotoItem, this));
    context.subscriptions.push(vscode.commands.registerCommand(
        'clangd.typeHierarchy.viewParents',
        () => this.setDirection(TypeHierarchyDirection.Parents)));
    context.subscriptions.push(vscode.commands.registerCommand(
        'clangd.typeHierarchy.viewChildren',
        () => this.setDirection(TypeHierarchyDirection.Children)));

    this.treeView = vscode.window.createTreeView('clangd.typeHierarchyView',
                                                 {treeDataProvider: this});
    context.subscriptions.push(this.treeView);
    // Show children by default.
    this.direction = TypeHierarchyDirection.Children;
  }

  public async gotoItem(item: TypeHierarchyItem) {
    const uri = vscode.Uri.parse(item.uri);
    const range =
        this.client.protocol2CodeConverter.asRange(item.selectionRange);
    const doc = await vscode.workspace.openTextDocument(uri);
    let editor: vscode.TextEditor;
    if (doc) {
      editor = await vscode.window.showTextDocument(doc, undefined);
    } else {
      editor = vscode.window.activeTextEditor;
    }
    if (!editor) {
      return;
    }
    editor.revealRange(range, vscode.TextEditorRevealType.InCenter);
    editor.selection = new vscode.Selection(range.start, range.end);
  }

  public async setDirection(direction: TypeHierarchyDirection) {
    this.direction = direction;
    this._onDidChangeTreeData.fire(null);
  }

  public getTreeItem(element: TypeHierarchyItem): vscode.TreeItem {
    return new TypeHierarchyTreeItem(element);
  }

  public getParent(element: TypeHierarchyItem): TypeHierarchyItem {
    // This function is only implemented so that VSCode lets us call
    // this.treeView.reveal(). Since we only ever call reveal() on the root,
    // which has no parent, it's fine to always return null.
    return null;
  }

  public async getChildren(element
                           ?: TypeHierarchyItem): Promise<TypeHierarchyItem[]> {
    if (!this.root)
      return [];
    if (!element)
      return [this.root];
    if (this.direction == TypeHierarchyDirection.Parents) {
      // Clangd always resolves parents eagerly, so just return them.
      return element.parents;
    }
    // Otherwise, this.direction == Children.
    if (!element.children) {
      // Children are not resolved yet, resolve them now.
      const resolved =
          await this.client.sendRequest(ResolveTypeHierarchyRequest.type, {
            item: element,
            direction: TypeHierarchyDirection.Children,
            resolve: 1
          });
      element.children = resolved.children;
    }
    return element.children;
  }

  private async reveal(editor: vscode.TextEditor) {
    // This makes the type hierarchy view visible by causing the condition
    // "when": "extension.vscode-clangd.typeHierarchyVisible" from
    // package.json to evaluate to true.
    vscode.commands.executeCommand(
        'setContext', 'extension.vscode-clangd.typeHierarchyVisible', true);

    const item = await this.client.sendRequest(TypeHierarchyRequest.type, {
      ...this.client.code2ProtocolConverter.asTextDocumentPositionParams(
          editor.document, editor.selection.active),
      // Resolve up to 5 initial levels. Any additional levels will be resolved
      // on the fly if the user expands the tree item.
      resolve: 5,
      // Resolve both directions initially. That way, if the user switches
      // to the Parents view, we have the data already. Note that clangd
      // does not support resolving parents via typeHierarchy/resolve,
      // so otherwise we'd have to remember the TextDocumentPositionParams
      // to make another textDocument/typeHierarchy request when switching
      // to Parents view.
      direction: TypeHierarchyDirection.Both
    });
    if (item) {
      this.root = item;
      this._onDidChangeTreeData.fire(null);

      // This focuses the "explorer" view container which contains the
      // type hierarchy view.
      vscode.commands.executeCommand('workbench.view.explorer');

      // This expands and focuses the type hierarchy view.
      this.treeView.reveal(this.root, {focus: true});
    } else {
      vscode.window.showInformationMessage(
          'No type hierarchy available for selection');
    }
  }

  private close() {
    // Hide the type hierarchy view.
    vscode.commands.executeCommand(
        'setContext', 'extension.vscode-clangd.typeHierarchyVisible', false);

    this.root = undefined;
    this._onDidChangeTreeData.fire(null);
  }
}