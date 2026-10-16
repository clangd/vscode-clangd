import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import * as ast from './ast';
import type {ClangdContextManager} from './clangd-context-manager';
import * as config from './config';
import * as configFileWatcher from './config-file-watcher';
import * as fileStatus from './file-status';
import * as inactiveRegions from './inactive-regions';
import * as inlayHints from './inlay-hints';
import * as memoryUsage from './memory-usage';
import * as openConfig from './open-config';
import * as switchSourceHeader from './switch-source-header';
import * as typeHierarchy from './type-hierarchy';

export const clangdDocumentSelector = [
  {scheme: 'file', language: 'c'},
  {scheme: 'file', language: 'cpp'},
  {scheme: 'file', language: 'cuda-cpp'},
  {scheme: 'file', language: 'objective-c'},
  {scheme: 'file', language: 'objective-cpp'},
];

export function isClangdDocument(document: vscode.TextDocument) {
  return vscode.languages.match(clangdDocumentSelector, document);
}

class ClangdLanguageClient extends vscodelc.LanguageClient {
  // Override the default implementation for failed requests. The default
  // behavior is just to log failures in the output panel, however output panel
  // is designed for extension debugging purpose, normal users will not open it,
  // thus when the failure occurs, normal users doesn't know that.
  //
  // For user-interactive operations (e.g. applyFixIt, applyTweaks), we will
  // prompt up the failure to users.

  handleFailedRequest<T>(type: vscodelc.MessageSignature,
                         token: vscode.CancellationToken|undefined, error: any,
                         defaultValue: T, showNotification?: boolean,
                         throwOnCancel?: boolean): T {
    if (error instanceof vscodelc.ResponseError &&
        type.method === 'workspace/executeCommand')
      vscode.window.showErrorMessage(error.message);

    return super.handleFailedRequest(type, token, error, defaultValue,
                                     showNotification, throwOnCancel);
  }
}

class EnableEditsNearCursorFeature implements vscodelc.StaticFeature {
  initialize() {}
  fillClientCapabilities(capabilities: vscodelc.ClientCapabilities): void {
    const extendedCompletionCapabilities: any =
        capabilities.textDocument?.completion;
    extendedCompletionCapabilities.editsNearCursor = true;
  }
  getState(): vscodelc.FeatureState { return {kind: 'static'}; }
  clear() {}
}

// Registers the commands, views and the status bar item of the features, which
// are shared by all contexts. The per-client parts of the features are
// activated by ClangdContext.startClient().
export function registerFeatureCommands(manager: ClangdContextManager):
    vscode.Disposable[] {
  return [
    ...ast.registerCommands(manager),
    ...fileStatus.registerCommands(manager),
    ...inlayHints.registerCommands(),
    ...memoryUsage.registerCommands(manager),
    ...openConfig.registerCommands(manager),
    ...switchSourceHeader.registerCommands(manager),
    ...typeHierarchy.registerCommands(manager),
  ];
}

// Returns the context serving `uri` for a command, if its client is starting
// or running. Otherwise tells the user and returns undefined.
export function contextForCommand(manager: ClangdContextManager,
                                  uri: vscode.Uri): ClangdContext|undefined {
  const context = manager.contextFor(uri);
  if (!context?.clientIsStartingOrRunning()) {
    vscode.window.showInformationMessage('clangd is not running for this file');
    return undefined;
  }
  return context;
}

// Disposes what is pushed to it.
class DisposingArray extends Array<vscode.Disposable> {
  push(...items: vscode.Disposable[]): number {
    items.forEach((d) => d.dispose());
    return this.length;
  }
}

export class ClangdContext implements vscode.Disposable {
  // Disposed with the context. A client disposed while it starts still
  // initializes its features; what they subscribe to is disposed right away.
  subscriptions: vscode.Disposable[] = [];
  client: ClangdLanguageClient;
  readonly manager: ClangdContextManager;
  readonly documentSelector: vscode.DocumentSelector = clangdDocumentSelector;
  // Resolves once the client has started.
  private clientStarted: Promise<void>|undefined;
  private disposed = false;

  static async create(clangdPath: string,
                      outputChannel: vscode.LogOutputChannel,
                      manager: ClangdContextManager): Promise<ClangdContext> {
    return new ClangdContext(
        manager, await ClangdContext.createClient(clangdPath, outputChannel));
  }

  private static async createClient(clangdPath: string,
                                    outputChannel: vscode.LogOutputChannel):
      Promise<ClangdLanguageClient> {
    const useScriptAsExecutable =
        await config.get<boolean>('useScriptAsExecutable');
    let clangdArguments = await config.get<string[]>('arguments');
    if (useScriptAsExecutable) {
      let quote = (str: string) => { return `"${str}"`; };
      clangdPath = quote(clangdPath)
      for (var i = 0; i < clangdArguments.length; i++) {
        clangdArguments[i] = quote(clangdArguments[i]);
      }
    }
    const clangd: vscodelc.Executable = {
      command: clangdPath,
      args: clangdArguments,
      options: {
        cwd: vscode.workspace.rootPath || process.cwd(),
        shell: useScriptAsExecutable
      }
    };
    const traceFile = await config.get<string>('trace');
    if (!!traceFile) {
      const trace = {CLANGD_TRACE: traceFile};
      clangd.options = {...clangd.options, env: {...process.env, ...trace}};
    }
    const serverOptions: vscodelc.ServerOptions = clangd;

    const clientOptions: vscodelc.LanguageClientOptions = {
      // Register the server for c-family and cuda files.
      documentSelector: clangdDocumentSelector,
      initializationOptions: {
        clangdFileStatus: true,
        fallbackFlags: await config.get<string[]>('fallbackFlags')
      },
      outputChannel: outputChannel,
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
          if (!await config.get<boolean>('enableCodeCompletion'))
            return new vscode.CompletionList([], /*isIncomplete=*/ false);
          let list = await next(document, position, context, token);
          if (!await config.get<boolean>('serverCompletionRanking'))
            return list;
          let items = (!list ? [] : Array.isArray(list) ? list : list.items);
          items = items.map(item => {
            // Gets the prefix used by VSCode when doing fuzzymatch.
            // item.range is either a Range or {inserting, replacing} (see
            // CompletionItem in the VS Code API); narrow before using.
            let prefix = '';
            if (item.range) {
              const start = item.range instanceof vscode.Range
                                ? item.range.start
                                : item.range.inserting.start;
              prefix = document.getText(new vscode.Range(start, position));
            }
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
          if (!await config.get<boolean>('enableHover'))
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

    const client = new ClangdLanguageClient('Clang Language Server',
                                            serverOptions, clientOptions);
    client.clientOptions.errorHandler = client.createDefaultErrorHandler(
        // max restart count
        await config.get<boolean>('restartAfterCrash') ? /*default*/ 4 : 0);
    client.registerFeature(new EnableEditsNearCursorFeature);

    return client;
  }

  private constructor(manager: ClangdContextManager,
                      client: ClangdLanguageClient) {
    this.manager = manager;
    this.client = client;
  }

  async startClient() {
    typeHierarchy.activate(this);
    inlayHints.activate(this);
    memoryUsage.activate(this);
    ast.activate(this);
    inactiveRegions.activate(this);
    const watcherActivated = configFileWatcher.activate(this);
    // Assigned synchronously, so that dispose() can wait for the client to
    // finish starting: a client can't be stopped while it is starting.
    this.clientStarted = watcherActivated.then(async () => {
      if (!this.disposed)
        await this.client.start();
    });
    // The client reports a failed start itself.
    this.clientStarted.catch(() => {});
    await watcherActivated;
    console.log('Clang Language Server is now active!');
  }

  servesDocument(document: vscode.TextDocument) {
    return vscode.languages.match(this.documentSelector, document) > 0;
  }

  get visibleClangdEditors(): vscode.TextEditor[] {
    return vscode.window.visibleTextEditors.filter(
        (e) => this.servesDocument(e.document));
  }

  clientIsStarting() {
    return this.client && this.client.state == vscodelc.State.Starting;
  }

  clientIsRunning() {
    return this.client && this.client.state == vscodelc.State.Running;
  }

  // Whether the client takes requests: a starting client sends them once it
  // runs.
  clientIsStartingOrRunning() {
    return this.clientIsStarting() || this.clientIsRunning();
  }

  // Resolves once the client is no longer starting: it is running, or it
  // failed to start. A client that restarts after a crash is starting again.
  async clientSettled(): Promise<void> {
    try {
      await this.clientStarted;
    } catch {
      return;
    }
    while (this.clientIsStarting())
      await new Promise<void>((resolve) => {
        const listener = this.client.onDidChangeState(() => {
          listener.dispose();
          resolve();
        });
      });
  }

  dispose() {
    if (this.disposed)
      return;
    this.disposed = true;
    this.subscriptions.forEach((d) => { d.dispose(); });
    this.subscriptions = new DisposingArray();
    // A client can't be stopped while it is starting. stop() failures, e.g.
    // a timeout, are logged by the client.
    this.clientSettled().then(() => this.client.stop()).catch(() => {});
  }
}
