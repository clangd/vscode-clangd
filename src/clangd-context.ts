import * as fs from 'fs';
import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import * as ast from './ast';
import * as config from './config';
import * as configFileWatcher from './config-file-watcher';
import * as fileStatus from './file-status';
import * as inactiveRegions from './inactive-regions';
import * as inlayHints from './inlay-hints';
import * as install from './install';
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

function createFollowSymlinksMiddleware(): vscodelc.Middleware {
  const isLocationLink = (value: unknown): value is vscode.LocationLink => {
    return !!value && typeof value === 'object' && 'targetUri' in value;
  };

  const resolveRealPath = async(uri: vscode.Uri): Promise<vscode.Uri> => {
    try {
      const resolvedPath = await fs.promises.realpath(uri.fsPath);
      return vscode.Uri.file(resolvedPath);
    } catch {
      return uri;
    }
  };

  const canonicalizeUri =
      async(uri: vscode.Uri, token: vscode.CancellationToken,
            cache: Map<string, vscode.Uri>): Promise<vscode.Uri> => {
    if (token.isCancellationRequested || uri.scheme !== 'file')
      return uri;
    const key = uri.toString();
    const cached = cache.get(key);
    if (cached)
      return cached;
    const resolved = await resolveRealPath(uri);
    cache.set(key, resolved);
    return resolved;
  };

  const canonicalizeLocation =
      async(location: vscode.Location, token: vscode.CancellationToken,
            cache: Map<string, vscode.Uri>): Promise<vscode.Location> => {
    const newUri = await canonicalizeUri(location.uri, token, cache);
    if (newUri.toString() === location.uri.toString())
      return location;
    return new vscode.Location(newUri, location.range);
  };

  const canonicalizeLocationLink =
      async(link: vscode.LocationLink, token: vscode.CancellationToken,
            cache: Map<string, vscode.Uri>): Promise<vscode.LocationLink> => {
    const newTargetUri = await canonicalizeUri(link.targetUri, token, cache);
    if (newTargetUri.toString() === link.targetUri.toString())
      return link;
    return {
      originSelectionRange: link.originSelectionRange,
      targetUri: newTargetUri,
      targetRange: link.targetRange,
      targetSelectionRange: link.targetSelectionRange,
    };
  };

  const canonicalizeNavigationResult =
      async<T extends vscode.Location|vscode.Location[]|vscode.LocationLink[]>(
          result: T|null|undefined, token: vscode.CancellationToken,
          cache: Map<string, vscode.Uri>): Promise<T|null|undefined> => {
    if (!result || token.isCancellationRequested)
      return result;
    if (Array.isArray(result)) {
      if (result.length === 0)
        return result;
      if (isLocationLink(result[0])) {
        const canonicalLinks: vscode.LocationLink[] = [];
        for (const link of result as vscode.LocationLink[]) {
          canonicalLinks.push(
              await canonicalizeLocationLink(link, token, cache));
        }
        return canonicalLinks as T;
      }
      const canonicalLocations: vscode.Location[] = [];
      for (const location of result as vscode.Location[]) {
        canonicalLocations.push(
            await canonicalizeLocation(location, token, cache));
      }
      return canonicalLocations as T;
    }
    const canonicalLocation =
        await canonicalizeLocation(result as vscode.Location, token, cache);
    return canonicalLocation as T;
  };

  const canonicalizeReferences =
      async(result: vscode.Location[]|null|undefined,
            token: vscode.CancellationToken, cache: Map<string, vscode.Uri>):
          Promise<vscode.Location[]|null|undefined> => {
            if (!result || token.isCancellationRequested)
              return result;
            return Promise.all(
                result.map(loc => canonicalizeLocation(loc, token, cache)));
          };

  const canonicalizeDocumentLinks =
      async(links: vscode.DocumentLink[]|null|undefined,
            token: vscode.CancellationToken, cache: Map<string, vscode.Uri>):
          Promise<vscode.DocumentLink[]|null|undefined> => {
            if (!links || token.isCancellationRequested)
              return links;
            const updated: vscode.DocumentLink[] = [];
            for (const link of links) {
              if (!link.target) {
                updated.push(link);
                continue;
              }
              const newTarget =
                  await canonicalizeUri(link.target, token, cache);
              if (newTarget.toString() !== link.target.toString())
                link.target = newTarget;
              updated.push(link);
            }
            return updated;
          };

  return {
    provideDefinition: async (document, position, token, next) => {
      if (!next)
        return null;
      const cache = new Map<string, vscode.Uri>();
      const result = await next(document, position, token);
      return await canonicalizeNavigationResult(result, token, cache);
    },
    provideTypeDefinition: async (document, position, token, next) => {
      if (!next)
        return null;
      const cache = new Map<string, vscode.Uri>();
      const result = await next(document, position, token);
      return await canonicalizeNavigationResult(result, token, cache);
    },
    provideImplementation: async (document, position, token, next) => {
      if (!next)
        return null;
      const cache = new Map<string, vscode.Uri>();
      const result = await next(document, position, token);
      return await canonicalizeNavigationResult(result, token, cache);
    },
    provideDeclaration: async (document, position, token, next) => {
      if (!next)
        return null;
      const cache = new Map<string, vscode.Uri>();
      const result = await next(document, position, token);
      return await canonicalizeNavigationResult(result, token, cache);
    },
    provideReferences: async (document, position, context, token, next) => {
      if (!next)
        return null;
      const cache = new Map<string, vscode.Uri>();
      const result = await next(document, position, context, token);
      return await canonicalizeReferences(result, token, cache);
    },
    provideDocumentLinks: async (document, token, next) => {
      if (!next)
        return null;
      const cache = new Map<string, vscode.Uri>();
      const links = await next(document, token);
      return await canonicalizeDocumentLinks(links, token, cache);
    }
  };
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

  static async create(globalStoragePath: string,
                      outputChannel: vscode.OutputChannel):
      Promise<ClangdContext|null> {
    const subscriptions: vscode.Disposable[] = [];
    const clangdPath = await install.activate(subscriptions, globalStoragePath);
    if (!clangdPath) {
      subscriptions.forEach((d) => { d.dispose(); });
      return null;
    }

    return new ClangdContext(subscriptions, await ClangdContext.createClient(
                                                clangdPath, outputChannel));
  }

  private static async createClient(clangdPath: string,
                                    outputChannel: vscode.OutputChannel):
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
    const middleware: vscodelc.Middleware = {
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
    };

    if (await config.get<boolean>('followSymlinks')) {
      Object.assign(middleware, createFollowSymlinksMiddleware());
    }

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
      middleware,
    };

    const client = new ClangdLanguageClient('Clang Language Server',
                                            serverOptions, clientOptions);
    client.clientOptions.errorHandler = client.createDefaultErrorHandler(
        // max restart count
        await config.get<boolean>('restartAfterCrash') ? /*default*/ 4 : 0);
    client.registerFeature(new EnableEditsNearCursorFeature);

    return client;
  }

  private constructor(subscriptions: vscode.Disposable[],
                      client: ClangdLanguageClient) {
    this.subscriptions = subscriptions;
    this.client = client;

    this.startClient();
  }

  async startClient() {
    typeHierarchy.activate(this);
    inlayHints.activate(this);
    memoryUsage.activate(this);
    ast.activate(this);
    openConfig.activate(this);
    inactiveRegions.activate(this);
    await configFileWatcher.activate(this);
    this.client.start();
    console.log('Clang Language Server is now active!');
    fileStatus.activate(this);
    switchSourceHeader.activate(this);
  }

  get visibleClangdEditors(): vscode.TextEditor[] {
    return vscode.window.visibleTextEditors.filter(
        (e) => isClangdDocument(e.document));
  }

  clientIsStarting() {
    return this.client && this.client.state == vscodelc.State.Starting;
  }

  clientIsRunning() {
    return this.client && this.client.state == vscodelc.State.Running;
  }

  dispose() {
    this.subscriptions.forEach((d) => { d.dispose(); });
    if (this.client)
      this.client.stop();
    this.subscriptions = []
  }
}
