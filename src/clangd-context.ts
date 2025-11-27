import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import * as ast from './ast';
import * as config from './config';
import * as configFileWatcher from './config-file-watcher';
import * as inactiveRegions from './inactive-regions';
import * as inlayHints from './inlay-hints';
import * as install from './install';
import * as memoryUsage from './memory-usage';
import * as openConfig from './open-config';
import * as switchSourceHeader from './switch-source-header';

export function clangdDocumentSelector(workspaceFolder: vscode.WorkspaceFolder|
                                       null): vscodelc.DocumentSelector {
  const baseSelector = [
    {scheme: 'file', language: 'c'},
    {scheme: 'file', language: 'cpp'},
    {scheme: 'file', language: 'cuda-cpp'},
    {scheme: 'file', language: 'objective-c'},
    {scheme: 'file', language: 'objective-cpp'},
  ];
  if (workspaceFolder) {
    return baseSelector.map(
        selector =>
            ({...selector, pattern: `${workspaceFolder.uri.fsPath}/**/*`}));
  }
  return baseSelector;
}

export function isClangdDocument(document: vscode.TextDocument) {
  return vscode.languages.match(clangdDocumentSelector(null), document);
}

class ClangdLanguageClient extends vscodelc.LanguageClient {
  // Override the default implementation for failed requests. The default
  // behavior is just to log failures in the output panel, however output panel
  // is designed for extension debugging purpose, normal users will not open it,
  // thus when the failure occurs, normal users doesn't know that.
  //
  // For user-interactive operations (e.g. applyFixIt, applyTweaks), we will
  // prompt up the failure to users.

  handleFailedRequest<T>(type: vscodelc.MessageSignature, error: any,
                         token: vscode.CancellationToken|undefined,
                         defaultValue: T): T {
    if (error instanceof vscodelc.ResponseError &&
        type.method === 'workspace/executeCommand')
      vscode.window.showErrorMessage(error.message);

    return super.handleFailedRequest(type, token, error, defaultValue);
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

export class ClangdContext implements vscode.Disposable {
  subscriptions: vscode.Disposable[];
  client: ClangdLanguageClient;
  readonly workspaceFolder: vscode.WorkspaceFolder|null;
  readonly ui: install.UI;

  static async create(globalStoragePath: string,
                      workspaceFolder: vscode.WorkspaceFolder|
                      null): Promise<ClangdContext|undefined> {
    const ui = await install.UI.create(globalStoragePath, workspaceFolder);
    const clangdPath = await install.prepare(ui, workspaceFolder);
    if (!clangdPath) {
      return undefined;
    }

    const outputChannelName =
        workspaceFolder ? `clangd (${workspaceFolder.name})` : 'clangd';

    return new ClangdContext(await ClangdContext.createClient(clangdPath,
                                                              outputChannelName,
                                                              workspaceFolder),
                             workspaceFolder, ui);
  }

  private static async createClient(clangdPath: string,
                                    outputChannelName: string,
                                    workspaceFolder: vscode.WorkspaceFolder|
                                    null): Promise<ClangdLanguageClient> {
    const useScriptAsExecutable =
        await config.get<boolean>('useScriptAsExecutable', workspaceFolder);
    let clangdArguments =
        await config.get<string[]>('arguments', workspaceFolder);
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
        cwd: workspaceFolder?.uri.fsPath ??
                 vscode.workspace.workspaceFolders?.[0]?.uri.fsPath ??
                 process.cwd(),
        shell: useScriptAsExecutable
      }
    };
    const traceFile = await config.get<string>('trace', workspaceFolder);
    if (!!traceFile) {
      const trace = {CLANGD_TRACE: traceFile};
      clangd.options = {...clangd.options, env: {...process.env, ...trace}};
    }
    const serverOptions: vscodelc.ServerOptions = clangd;

    const clientOptions: vscodelc.LanguageClientOptions = {
      // Register the server for c-family and cuda files.
      documentSelector: clangdDocumentSelector(workspaceFolder),
      workspaceFolder: workspaceFolder ?? undefined,
      initializationOptions: {
        clangdFileStatus: true,
        fallbackFlags:
            await config.get<string[]>('fallbackFlags', workspaceFolder)
      },
      outputChannelName: outputChannelName,
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
          if (!await config.get<boolean>('enableCodeCompletion',
                                         workspaceFolder))
            return new vscode.CompletionList([], /*isIncomplete=*/ false);
          let list = await next(document, position, context, token);
          if (!await config.get<boolean>('serverCompletionRanking',
                                         workspaceFolder))
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
          if (!await config.get<boolean>('enableHover', workspaceFolder))
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
        await config.get<boolean>('restartAfterCrash', workspaceFolder)
            ? /*default*/ 4
            : 0);
    client.registerFeature(new EnableEditsNearCursorFeature);

    return client;
  }

  private constructor(client: ClangdLanguageClient,
                      workspaceFolder: vscode.WorkspaceFolder|null,
                      ui: install.UI) {
    this.subscriptions = [];
    this.client = client;
    this.workspaceFolder = workspaceFolder;
    this.ui = ui;

    this.startClient();
  }

  async startClient() {
    inlayHints.activate(this);
    memoryUsage.activate(this);
    ast.activate(this);
    openConfig.activate(this);
    inactiveRegions.activate(this);
    await configFileWatcher.activate(this);
    this.client.start();
    const folderSuffix = this.workspaceFolder
                             ? ` for the ${this.workspaceFolder.name} folder`
                             : '';
    console.log(`Clang Language Server is now active${folderSuffix}!`);
    switchSourceHeader.activate(this);
  }

  /**
   * Returns the document selector for this context.
   * For per-folder contexts, restricts to files in that folder.
   * For the global context, returns the base clangd document selector.
   */
  get documentSelector(): vscodelc.DocumentSelector {
    return clangdDocumentSelector(this.workspaceFolder);
  }

  get visibleClangdEditors(): vscode.TextEditor[] {
    return vscode.window.visibleTextEditors.filter((e) => {
      if (!isClangdDocument(e.document)) {
        return false;
      }
      // For per-folder contexts, only include editors for documents in this
      // folder
      if (this.workspaceFolder) {
        const docFolder = vscode.workspace.getWorkspaceFolder(e.document.uri);
        return docFolder?.uri.toString() ===
               this.workspaceFolder.uri.toString();
      }
      return true;
    });
  }

  clientIsStarting() { return this.client.state == vscodelc.State.Starting; }

  clientIsRunning() { return this.client.state == vscodelc.State.Running; }

  dispose() {
    this.subscriptions.forEach((d) => { d.dispose(); });
    this.client.stop();
    this.subscriptions = []
  }
}
