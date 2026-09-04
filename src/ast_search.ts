// Implements the "ast search" feature: textDocument/searchAST +
// clangd/completeASTMatcher.
import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {
  ASTMatcherCompletion,
  ASTMatcherCompletionParams,
  ASTNode,
  ASTSearchParams,
  ASTSearchResult
} from '../api/vscode-clangd';

import {TreeAdapter} from './ast';
import {ClangdContext} from './clangd-context';

const ASTSearchRequestMethod = 'textDocument/searchAST';
const ASTSearchRequestType =
    new vscodelc.RequestType<ASTSearchParams, ASTSearchResult[], void>(
        ASTSearchRequestMethod);

const CompleteASTMatcherMethod = 'clangd/completeASTMatcher';
const ASTMatcherCompletionRequestType =
    new vscodelc
        .RequestType<ASTMatcherCompletionParams, ASTMatcherCompletion[], void>(
            CompleteASTMatcherMethod);

export function activate(context: ClangdContext) {
  const feature = new ASTSearchFeature(context);
  context.client.registerFeature(feature);
}

// A per-webview nonce, used to scope the Content-Security-Policy to the
// scripts/styles we actually emit.
function nonce(): string {
  const chars =
      'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
  let text = '';
  for (let i = 0; i < 32; i++)
    text += chars.charAt(Math.floor(Math.random() * chars.length));
  return text;
}

// Hosts the search input box and its AST-matcher completion dropdown.
// Talks to ASTSearchFeature over postMessage; has no LSP knowledge itself.
class ASTSearchViewProvider implements vscode.WebviewViewProvider {
  public static readonly viewType = 'clangd.astSearch';

  constructor(private readonly onSearch: (query: string) => void,
              private readonly onComplete: (query: string, offset: number) =>
                  Promise<ASTMatcherCompletion[]>) {}

  resolveWebviewView(webviewView: vscode.WebviewView) {
    webviewView.webview.options = {enableScripts: true};
    webviewView.webview.html = this.getHtml();

    webviewView.webview.onDidReceiveMessage(async message => {
      switch (message?.command) {
      case 'search':
        if (typeof message.query === 'string')
          this.onSearch(message.query);
        return;
      case 'complete':
        if (typeof message.query !== 'string' ||
            typeof message.offset !== 'number')
          return;
        const items = await this.onComplete(message.query, message.offset);
        webviewView.webview.postMessage(
            {command: 'completions', requestId: message.requestId, items});
        return;
      }
    });
  }

  private getHtml(): string {
    const csp = nonce();
    return `<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'nonce-${
        csp}'; script-src 'nonce-${csp}';">
<style nonce="${csp}">
  body {
    padding: 0;
    margin: 0;
    color: var(--vscode-foreground);
    font-family: var(--vscode-font-family);
    font-size: var(--vscode-font-size);
  }
  .search-box {
    position: relative;
    display: flex;
    gap: 4px;
    padding: 6px 8px;
  }
  #query {
    flex: 1 1 auto;
    min-width: 0;
    box-sizing: border-box;
    padding: 3px 6px;
    background: var(--vscode-input-background);
    color: var(--vscode-input-foreground);
    border: 1px solid var(--vscode-input-border, transparent);
    border-radius: 2px;
    font-family: var(--vscode-editor-font-family, monospace);
  }
  #query:focus {
    outline: 1px solid var(--vscode-focusBorder);
    outline-offset: -1px;
  }
  #search {
    padding: 3px 10px;
    background: var(--vscode-button-background);
    color: var(--vscode-button-foreground);
    border: none;
    border-radius: 2px;
    cursor: pointer;
  }
  #search:hover {
    background: var(--vscode-button-hoverBackground);
  }
  #completions {
    position: absolute;
    left: 8px;
    right: 8px;
    top: 100%;
    z-index: 10;
    margin: 2px 0 0;
    padding: 2px;
    max-height: 240px;
    overflow-y: auto;
    list-style: none;
    background: var(--vscode-editorSuggestWidget-background);
    border: 1px solid var(--vscode-editorSuggestWidget-border);
    border-radius: 2px;
    box-shadow: 0 2px 6px rgba(0, 0, 0, 0.25);
  }
  #completions.hidden {
    display: none;
  }
  .completion-item {
    display: flex;
    flex-direction: column;
    gap: 1px;
    padding: 4px 6px;
    border-radius: 2px;
    cursor: pointer;
  }
  .completion-item .label {
    font-family: var(--vscode-editor-font-family, monospace);
    color: var(--vscode-editorSuggestWidget-foreground);
    white-space: nowrap;
  }
  .completion-item .detail {
    font-size: 0.9em;
    color: var(--vscode-descriptionForeground);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }
  .completion-item.active,
  .completion-item:hover {
    background: var(--vscode-editorSuggestWidget-selectedBackground);
    color: var(--vscode-editorSuggestWidget-selectedForeground);
  }
  .completion-item.active .detail,
  .completion-item:hover .detail {
    color: inherit;
  }
</style>
</head>
<body>
  <div class="search-box">
    <input id="query" type="text" autocomplete="off" spellcheck="false"
           placeholder="AST matcher, e.g. functionDecl(hasName(&quot;foo&quot;))">
    <button id="search" title="Search AST (Enter)">Search</button>
    <ul id="completions" class="hidden" role="listbox"
        aria-label="AST matcher completions"></ul>
  </div>
<script nonce="${csp}">
  (function() {
    const vscode = acquireVsCodeApi();
    const query = document.getElementById('query');
    const search = document.getElementById('search');
    const completionsEl = document.getElementById('completions');

    let items = [];
    let activeIndex = -1;
    let latestRequestId = 0;
    let latestOffset = 0;
    let debounceTimer;

    function doSearch() {
      vscode.postMessage({command: 'search', query: query.value});
    }

    function requestCompletions() {
      latestRequestId++;
      latestOffset = query.selectionStart ?? query.value.length;
      vscode.postMessage({
        command: 'complete',
        query: query.value,
        offset: latestOffset,
        requestId: latestRequestId,
      });
    }

    function hideCompletions() {
      items = [];
      activeIndex = -1;
      render();
    }

    function showCompletions(newItems) {
      items = newItems;
      activeIndex = items.length ? 0 : -1;
      render();
    }

    function acceptCompletion(index) {
      const item = items[index];
      if (!item)
        return;
      const value = query.value;
      query.value =
          value.slice(0, latestOffset) + item.typedText + value.slice(latestOffset);
      const cursor = latestOffset + item.typedText.length;
      query.setSelectionRange(cursor, cursor);
      hideCompletions();
      query.focus();
      requestCompletions();
    }

    function render() {
      completionsEl.textContent = '';

      if (items.length === 0) {
        completionsEl.classList.add('hidden');
        return;
      }

      completionsEl.classList.remove('hidden');

      items.forEach((item, i) => {
        const li = document.createElement('li');
        li.className = 'completion-item' + (i === activeIndex ? ' active' : '');
        li.setAttribute('role', 'option');
        li.setAttribute('aria-selected', String(i === activeIndex));

        const label = document.createElement('div');
        label.className = 'label';
        label.textContent = item.typedText;
        li.appendChild(label);

        if (item.matcherDecl) {
          const detail = document.createElement('div');
          detail.className = 'detail';
          detail.textContent = item.matcherDecl;
          li.appendChild(detail);
        }

        li.addEventListener('click', () => acceptCompletion(i));
        completionsEl.appendChild(li);

        if (i === activeIndex)
          li.scrollIntoView({block: 'nearest'});
      });
    }

    // Keep focus on the input when clicking a completion (avoids a blur
    // closing the dropdown before the click is handled).
    completionsEl.addEventListener('mousedown', event => event.preventDefault());

    query.addEventListener('input', () => {
      clearTimeout(debounceTimer);
      if (!query.value.trim()) {
        hideCompletions();
        return;
      }
      debounceTimer = setTimeout(requestCompletions, 150);
    });

    query.addEventListener('blur', () => setTimeout(hideCompletions, 100));

    query.addEventListener('keydown', event => {
      if (event.key === 'ArrowDown' && items.length) {
        event.preventDefault();
        activeIndex = (activeIndex + 1) % items.length;
        render();
      } else if (event.key === 'ArrowUp' && items.length) {
        event.preventDefault();
        activeIndex = (activeIndex - 1 + items.length) % items.length;
        render();
      } else if (event.key === 'Escape' && items.length) {
        event.preventDefault();
        hideCompletions();
      } else if (event.key === 'Tab' && activeIndex >= 0) {
        event.preventDefault();
        acceptCompletion(activeIndex);
      } else if (event.key === 'Enter') {
        event.preventDefault();
        if (activeIndex >= 0)
          acceptCompletion(activeIndex);
        else
          doSearch();
      } else if (event.key === ' ' && event.ctrlKey) {
        event.preventDefault();
        requestCompletions();
      }
    });

    window.addEventListener('message', event => {
      const message = event.data;
      if (message.command === 'completions' &&
          message.requestId === latestRequestId) {
        showCompletions(message.items || []);
      }
    });

    search.addEventListener('click', doSearch);
  })();
</script>
</body>
</html>`;
  }
}

class ASTSearchFeature implements vscodelc.StaticFeature {
  private readonly treeAdapter = new ASTSearchTreeAdapter();
  private searchSupported = false;

  constructor(private readonly context: ClangdContext) {
    const searchView = new ASTSearchViewProvider(
        query => this.search(query),
        (query, offset) => this.complete(query, offset));

    const resultsTree =
        vscode.window.createTreeView('clangd.astSearchResults', {
          treeDataProvider: this.treeAdapter,
          showCollapseAll: true,
        });

    context.subscriptions.push(
        vscode.window.registerWebviewViewProvider(
            ASTSearchViewProvider.viewType, searchView,
            {webviewOptions: {retainContextWhenHidden: true}}),
        resultsTree,
        vscode.commands.registerCommand(
            'clangd.astSearch',
            () => vscode.commands.executeCommand('clangd.astSearch.focus')),
        vscode.commands.registerCommand('clangd.astSearch.close',
                                        () => this.treeAdapter.clear()));

    this.treeAdapter.onDidChangeTreeData(() => {
      vscode.commands.executeCommand('setContext', 'clangd.astSearch.hasData',
                                     !this.treeAdapter.empty());
    });
  }

  private async search(query: string): Promise<void> {
    if (!query.trim())
      return;

    const editor = vscode.window.activeTextEditor;
    if (!editor) {
      vscode.window.showInformationMessage('No active editor.');
      return;
    }

    try {
      const converter = this.context.client.code2ProtocolConverter;
      const result =
          await this.context.client.sendRequest(ASTSearchRequestType, {
            textDocument: converter.asTextDocumentIdentifier(editor.document),
            query,
          });

      if (!result || result.length === 0) {
        this.treeAdapter.clear();
        vscode.window.showInformationMessage('No matching AST nodes found.');
        return;
      }

      this.treeAdapter.setResults(result, editor.document.uri);
    } catch (error) {
      vscode.window.showErrorMessage(`AST search failed: ${error}`);
    }
  }

  private async complete(query: string,
                         offset: number): Promise<ASTMatcherCompletion[]> {
    if (!this.searchSupported)
      return [];

    try {
      return await this.context.client.sendRequest(
          ASTMatcherCompletionRequestType, {searchQuery: query, offset});
    } catch {
      return [];
    }
  }

  fillClientCapabilities(_capabilities: vscodelc.ClientCapabilities) {}

  initialize(capabilities: vscodelc.ServerCapabilities,
             _documentSelector: vscodelc.DocumentSelector|undefined) {
    this.searchSupported = 'astSearchProvider' in capabilities;

    vscode.commands.executeCommand('setContext', 'clangd.astSearch.supported',
                                   this.searchSupported);
  }

  getState(): vscodelc.FeatureState { return {kind: 'static'}; }

  clear() {}
}

class ASTSearchTreeNode {
  constructor(public readonly adapter: TreeAdapter,
              public readonly node: ASTNode, public readonly binding?: string) {
  }
}

class ASTSearchMatch {
  readonly nodes: ASTSearchTreeNode[];

  constructor(result: ASTSearchResult, document: vscode.Uri) {
    this.nodes = Object.entries(result).map(([binding, node]) => {
      const adapter = new TreeAdapter();
      adapter.setRoot(node, document);

      return new ASTSearchTreeNode(adapter, node, binding);
    });
  }
}

class ASTSearchTreeAdapter implements
    vscode.TreeDataProvider<ASTSearchMatch|ASTSearchTreeNode> {

  private matches: ASTSearchMatch[] = [];

  private readonly _onDidChangeTreeData =
      new vscode.EventEmitter<ASTSearchMatch|ASTSearchTreeNode|undefined>();

  readonly onDidChangeTreeData = this._onDidChangeTreeData.event;

  setResults(results: ASTSearchResult[], document: vscode.Uri) {

    this.matches = results.map(result => new ASTSearchMatch(result, document));

    this._onDidChangeTreeData.fire(undefined);
  }

  clear() {
    this.matches = [];

    this._onDidChangeTreeData.fire(undefined);
  }

  getTreeItem(element: ASTSearchMatch|ASTSearchTreeNode): vscode.TreeItem {

    if (element instanceof ASTSearchMatch) {
      return new vscode.TreeItem('Match',
                                 vscode.TreeItemCollapsibleState.Expanded);
    }

    const item = element.adapter.getTreeItem(element.node);

    if (element.binding) {
      item.label = `${element.binding}: ${item.label}`;
    }

    return item;
  }

  getChildren(element?: ASTSearchMatch|
              ASTSearchTreeNode): (ASTSearchMatch|ASTSearchTreeNode)[] {

    if (!element) {
      return this.matches;
    }

    if (element instanceof ASTSearchMatch) {
      return element.nodes;
    }

    return element.adapter.getChildren(element.node)
        .map(node => new ASTSearchTreeNode(element.adapter, node, ''));
  }

  empty(): boolean { return this.matches.length === 0; }
}
