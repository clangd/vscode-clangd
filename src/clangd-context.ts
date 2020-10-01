import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import * as config from './config';
import * as configFileWatcher from './config-file-watcher';
import * as fileStatus from './file-status';
import * as install from './install';
import * as semanticHighlighting from './semantic-highlighting';
import * as switchSourceHeader from './switch-source-header';
import * as typeHierarchy from './type-hierarchy';

class ClangdLanguageClient extends vscodelc.LanguageClient {
  // Override the default implementation for failed requests. The default
  // behavior is just to log failures in the output panel, however output panel
  // is designed for extension debugging purpose, normal users will not open it,
  // thus when the failure occurs, normal users doesn't know that.
  //
  // For user-interactive operations (e.g. applyFixIt, applyTweaks), we will
  // prompt up the failure to users.

  handleFailedRequest<T>(type: vscodelc.MessageSignature, error: any,
                         defaultValue: T): T {
    if (error instanceof vscodelc.ResponseError &&
        type.method === 'workspace/executeCommand')
      vscode.window.showErrorMessage(error.message);

    return super.handleFailedRequest(type, error, defaultValue);
  }

  activate() {
    this.dispose();
    this.startDisposable = this.start();
  }

  dispose() {
    if (this.startDisposable)
      this.startDisposable.dispose();
  }
  private startDisposable: vscodelc.Disposable;
}

class EnableEditsNearCursorFeature implements vscodelc.StaticFeature {
  initialize() {}
  fillClientCapabilities(capabilities: vscodelc.ClientCapabilities): void {
    const extendedCompletionCapabilities: any =
        capabilities.textDocument.completion;
    extendedCompletionCapabilities.editsNearCursor = true;
  }
}

export class ClangdContext implements vscode.Disposable {
  subscriptions: vscode.Disposable[] = [];
  client: ClangdLanguageClient;

  async activate(globalStoragePath: string,
                 outputChannel: vscode.OutputChannel) {
    const clangdPath = await install.activate(this, globalStoragePath);
    if (!clangdPath)
      return;

    const clangd: vscodelc.Executable = {
      command: clangdPath,
      args: config.get<string[]>('arguments')
    };
    const traceFile = config.get<string>('trace');
    if (!!traceFile) {
      const trace = {CLANGD_TRACE: traceFile};
      clangd.options = {env: {...process.env, ...trace}};
    }
    const serverOptions: vscodelc.ServerOptions = clangd;

    const clientOptions: vscodelc.LanguageClientOptions = {
      // Register the server for c-family and cuda files.
      documentSelector: [
        {scheme: 'file', language: 'c'},
        {scheme: 'file', language: 'cpp'},
        // CUDA is not supported by vscode, but our extension does supports it.
        {scheme: 'file', language: 'cuda'},
        {scheme: 'file', language: 'objective-c'},
        {scheme: 'file', language: 'objective-cpp'},
      ],
      initializationOptions: {
        clangdFileStatus: true,
        fallbackFlags: config.get<string[]>('fallbackFlags')
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
          let list = await next(document, position, context, token);
          let items = (Array.isArray(list) ? list : list.items).map(item => {
            // Gets the prefix used by VSCode when doing fuzzymatch.
            let prefix = document.getText(
                new vscode.Range((item.range as vscode.Range).start, position))
            if (prefix)
            item.filterText = prefix + '_' + item.filterText;
            return item;
          })
          return new vscode.CompletionList(items, /*isIncomplete=*/ true);
        },
        // VSCode applies fuzzy match only on the symbol name, thus it throws
        // away
        // all results if query token is a prefix qualified name.
        // By adding the containerName to the symbol name, it prevents VSCode
        // from
        // filtering out any results, e.g. enable workspaceSymbols for qualified
        // symbols.
        provideWorkspaceSymbols: async (query, token, next) => {
          let symbols = await next(query, token);
          return symbols.map(symbol => {
            if (symbol.containerName)
              symbol.name = `${symbol.containerName}::${symbol.name}`;
            // Always clean the containerName to avoid displaying it twice.
            symbol.containerName = '';
            return symbol;
          })
        },
      },
    };

    this.client = new ClangdLanguageClient('Clang Language Server',
                                           serverOptions, clientOptions);
    this.subscriptions.push(vscode.Disposable.from(this.client));
    if (config.get<boolean>('semanticHighlighting'))
      semanticHighlighting.activate(this);
    this.client.registerFeature(new EnableEditsNearCursorFeature);
    typeHierarchy.activate(this);
    this.client.activate();
    console.log('Clang Language Server is now active!');
    fileStatus.activate(this);
    switchSourceHeader.activate(this);
    configFileWatcher.activate(this);
  }

  dispose() {
    this.subscriptions.forEach((d) => { d.dispose(); });
    this.subscriptions = []
  }
}
