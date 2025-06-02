import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import { ASTFeature, ASTProvider } from './ast';
import * as config from './config';
import { ConfigFileWatcher, ConfigFileWatcherFeature } from './config-file-watcher';
import { FileStatus } from './file-status'
import { InactiveRegionsFeature } from './inactive-regions';
import { InlayHintsFeature } from './inlay-hints';
import { MemoryUsageProvider, MemoryUsageFeature } from './memory-usage';
import * as openConfig from './open-config';
import * as switchSourceHeader from './switch-source-header';
import { TypeHierarchyProvider, TypeHierarchyFeature } from './type-hierarchy';

export function clangdDocumentSelector(folder: vscode.WorkspaceFolder|undefined): vscodelc.DocumentSelector {
  return [{scheme: 'file', language: 'c', pattern: `${folder?.uri.fsPath}/**/*`},
          {scheme: 'file', language: 'cpp', pattern: `${folder?.uri.fsPath}/**/*`},
          {scheme: 'file', language: 'cuda-cpp', pattern: `${folder?.uri.fsPath}/**/*`},
          {scheme: 'file', language: 'objective-c', pattern: `${folder?.uri.fsPath}/**/*`},
          {scheme: 'file', language: 'objective-cpp', pattern: `${folder?.uri.fsPath}/**/*`}];
}

export function isClangdDocument(document: vscode.TextDocument) {
  return vscode.languages.match(clangdDocumentSelector(vscode.workspace.getWorkspaceFolder(document.uri)), document);
}

export class ClangdLanguageClient extends vscodelc.LanguageClient {
  subscriptions: vscode.Disposable[] = [];

  // Override the default implementation for failed requests. The default
  // behavior is just to log failures in the output panel, however output panel
  // is designed for extension debugging purpose, normal users will not open it,
  // thus when the failure occurs, normal users doesn't know that.
  //
  // For user-interactive operations (e.g. applyFixIt, applyTweaks), we will
  // prompt up the failure to users.

  constructor(
    public readonly context: ClangdContext,
    serverOptions: vscodelc.ServerOptions,
    clientOptions: vscodelc.LanguageClientOptions,
  ) {
    super('clangd', serverOptions, clientOptions)
  }

  handleFailedRequest<T>(type: vscodelc.MessageSignature, error: any,
                         token: vscode.CancellationToken|undefined,
                         defaultValue: T): T {
    if (error instanceof vscodelc.ResponseError &&
        type.method === 'workspace/executeCommand')
      vscode.window.showErrorMessage(error.message);

    return super.handleFailedRequest(type, token, error, defaultValue);
  }

  isStarting() {
    return this.state == vscodelc.State.Starting;
  }

  visibleEditors() {
    return vscode.window.visibleTextEditors.filter((e) => isClangdDocument(e.document) &&
      vscode.workspace.getWorkspaceFolder(e.document.uri) == this.clientOptions.workspaceFolder);
  }

  dispose(timeout?: number): Promise<void> {
    this.subscriptions.forEach((d) => { d.dispose(); });
    return super.dispose(timeout);
  }
}

class EnableEditsNearCursorFeature implements vscodelc.StaticFeature {
  constructor(client: ClangdLanguageClient) {
    client.registerFeature(this);
  }

  initialize() {}
  fillClientCapabilities(capabilities: vscodelc.ClientCapabilities): void {
    const extendedCompletionCapabilities: any =
        capabilities.textDocument?.completion;
    extendedCompletionCapabilities.editsNearCursor = true;
  }
  getState(): vscodelc.FeatureState { return {kind: 'static'}; }
  clear() {}
}

export class ClangdContext implements vscode.Disposable {
  subscriptions: vscode.Disposable[];
  clients: Map<string, ClangdLanguageClient>;

  configFileWatcher: ConfigFileWatcher|undefined;
  fileStatus: FileStatus;

  constructor(private outputChannel: vscode.OutputChannel) {
    this.subscriptions = [];
    this.clients = new Map();

    new ASTProvider(this);
    new MemoryUsageProvider(this);
    new TypeHierarchyProvider(this);

    if (config.get<string>('onConfigChanged') !== 'ignore')
      this.configFileWatcher = new ConfigFileWatcher(this);

    this.fileStatus = new FileStatus(this);

    openConfig.activate(this);
    switchSourceHeader.activate(this);
    InlayHintsFeature.activate(this);
  };

  public addClient(folder: vscode.WorkspaceFolder) {
    const useScriptAsExecutable = config.get<boolean>('useScriptAsExecutable', folder);
    let clangdPath = config.get<string>('path', folder);
    let clangdArguments = config.get<string[]>('arguments', folder);
    if (useScriptAsExecutable) {
      let quote = (str: string) => { return `"${str}"`; };
      clangdPath = quote(clangdPath);
      for (var i = 0; i < clangdArguments.length; i++) {
        clangdArguments[i] = quote(clangdArguments[i]);
      }

    }
    const clangd: vscodelc.Executable = {
      command: clangdPath,
      args: clangdArguments,
      options: {
        cwd: folder?.uri.fsPath || process.cwd(),
        shell: useScriptAsExecutable
      }
    };
    const traceFile = config.get<string>('trace', folder);
    if (!!traceFile) {
      const trace = {CLANGD_TRACE: traceFile};
      clangd.options = {env: {...process.env, ...trace}};
    }
    const serverOptions: vscodelc.ServerOptions = clangd;

    const clientOptions: vscodelc.LanguageClientOptions = {
      // Register the server for c-family and cuda files.
      documentSelector: clangdDocumentSelector(folder),
      workspaceFolder: folder,
      initializationOptions: {
        clangdFileStatus: true,
        fallbackFlags: config.get<string[]>('fallbackFlags', folder)
      },
      outputChannel: this.outputChannel,
      // Do not switch to output window when clangd returns output.
      revealOutputChannelOn: vscodelc.RevealOutputChannelOn.Never,

      // We hack up the completion items a bit to prevent VSCode from re-ranking
      // and throwing away all our delicious signals like type information.
      //
      // VSCode sorts by (fuzzymatch(prefix, item.filterText), item.sortText)
      // By adding the prefix to the beginning of the filterText, we get a
      // perfect
      // fuzzymatch score for every item.
      // The sortText (which reflects clangd ranking) breaks the tie.
      // This also prevents VSCode from filtering out any results due to the
      // differences in how fuzzy filtering is applies, e.g. enable dot-to-arrow
      // fixes in completion.
      //
      // We also mark the list as incomplete to force retrieving new rankings.
      // See https://github.com/microsoft/language-server-protocol/issues/898
      middleware: {
        provideCompletionItem: async (document, position, context, token,
                                      next) => {
          if (!config.get<boolean>('enableCodeCompletion', folder))
            return new vscode.CompletionList([], /*isIncomplete=*/ false);
          let list = await next(document, position, context, token);
          if (!config.get<boolean>('serverCompletionRanking', folder))
            return list;
          let items = (!list ? [] : Array.isArray(list) ? list : list.items);
          items = items.map(item => {
            // Gets the prefix used by VSCode when doing fuzzymatch.
            let prefix = document.getText(
                new vscode.Range((item.range as vscode.Range).start, position))
            if (prefix)
            item.filterText = prefix + '_' + item.filterText;
            // Workaround for https://github.com/clangd/vscode-clangd/issues/357
            // clangd's used of commit-characters was well-intentioned, but
            // overall UX is poor. Due to vscode-languageclient bugs, we didn't
            // notice until the behavior was in several releases, so we need
            // to override it on the client.
            item.commitCharacters = [];
            // VSCode won't automatically trigger signature help when entering
            // a placeholder, e.g. if the completion inserted brackets and
            // placed the cursor inside them.
            // https://github.com/microsoft/vscode/issues/164310
            // They say a plugin should trigger this, but LSP has no mechanism.
            // https://github.com/microsoft/language-server-protocol/issues/274
            // (This workaround is incomplete, and only helps the first param).
            if (item.insertText instanceof vscode.SnippetString &&
                !item.command &&
                item.insertText.value.match(/[([{<,] ?\$\{?[01]\D/))
              item.command = {
                title: 'Signature help',
                command: 'editor.action.triggerParameterHints'
              };
            return item;
          })
          return new vscode.CompletionList(items, /*isIncomplete=*/ true);
        },
        provideHover: async (document, position, token, next) => {
          if (!config.get<boolean>('enableHover', folder))
            return null;
          return next(document, position, token);
        },
        // VSCode applies fuzzy match only on the symbol name, thus it throws
        // away all results if query token is a prefix qualified name.
        // By adding the containerName to the symbol name, it prevents VSCode
        // from filtering out any results, e.g. enable workspaceSymbols for
        // qualified symbols.
        provideWorkspaceSymbols: async (query, token, next) => {
          let symbols = await next(query, token);
          return symbols?.map(symbol => {
            // Only make this adjustment if the query is in fact qualified.
            // Otherwise, we get a suboptimal ordering of results because
            // including the name's qualifier (if it has one) in symbol.name
            // means vscode can no longer tell apart exact matches from
            // partial matches.
            if (query.includes('::')) {
              if (symbol.containerName)
                symbol.name = `${symbol.containerName}::${symbol.name}`;
              // results from clangd strip the leading '::', so vscode fuzzy
              // match will filter out all results unless we add prefix back in
              if (query.startsWith('::')) {
                symbol.name = `::${symbol.name}`;
              }
              // Clean the containerName to avoid displaying it twice.
              symbol.containerName = '';
            }
            return symbol;
          })
        },
      },
    };

    const client = new ClangdLanguageClient(this, serverOptions, clientOptions);
    this.subscriptions.push(client);

    client.clientOptions.errorHandler =
        client.createDefaultErrorHandler(
            // max restart count
            config.get<boolean>('restartAfterCrash', folder) ? /*default*/ 4 : 0);
    new EnableEditsNearCursorFeature(client);
    new TypeHierarchyFeature(client);
    new InlayHintsFeature(client);
    new MemoryUsageFeature(client);
    new ASTFeature(client);
    new InactiveRegionsFeature(client);
    new ConfigFileWatcherFeature(client);

    client.onDidChangeState(({newState}) => {
      if (newState === vscodelc.State.Running) {
        // clangd starts or restarts after crash.
        client.onNotification(
            'textDocument/clangd.fileStatus',
            (fileStatus) => { this.fileStatus.onFileUpdated(fileStatus); });
      } else if (newState === vscodelc.State.Stopped) {
        // Clear all cached statuses when clangd crashes.
        this.fileStatus.clear();
      }
    });

    client.start();
    console.log('Clang Language Server is now active!');

    this.clients.set(folder.name, client);
  }

  public removeClient(folder: vscode.WorkspaceFolder) {
    const client = this.clients.get(folder.name);
    if (client) {
      this.clients.delete(folder.name);
      client.dispose();
    }
  }

  public hasClient(folder: vscode.WorkspaceFolder): boolean {
    return this.clients.has(folder.name);
  }

  get visibleClangdEditors(): vscode.TextEditor[] {
    return vscode.window.visibleTextEditors.filter(
        (e) => isClangdDocument(e.document));
  }

  public getActiveFolder(): vscode.WorkspaceFolder|undefined {
    let folder = undefined;

    if (vscode.window.activeTextEditor !== undefined) {
      folder = vscode.workspace.getWorkspaceFolder(vscode.window.activeTextEditor.document.uri)
    } else if (vscode.workspace.workspaceFolders !== undefined) {
      folder = vscode.workspace.workspaceFolders[0];
    }

    return folder;
  }


  public getActiveClient(): ClangdLanguageClient|undefined {
    const folder = this.getActiveFolder();
    if (folder === undefined) {
      return undefined;
    }

    return this.clients.get(folder.name);
  }

  dispose() {
    this.subscriptions.forEach((d) => { d.dispose(); });
    this.subscriptions = []
    this.clients.forEach((client) => { client.stop(); });
    this.clients.clear();
  }
}
