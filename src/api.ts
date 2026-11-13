import * as vscode from 'vscode';
import {BaseLanguageClient} from 'vscode-languageclient';

import {
  ClangdApiV1,
  ClangdApiV2,
  ClangdExtension,
  ClangdProject,
  ClangdProjectProvider
} from '../api/vscode-clangd';

import {ClangdContext} from './clangd-context';
import {ClangdContextManager} from './clangd-context-manager';

function clientOf(context: ClangdContext|undefined): BaseLanguageClient|
    undefined {
  return context?.clientIsStartingOrRunning() ? context.client : undefined;
}

class ClangdApiV2Impl implements ClangdApiV2 {
  constructor(private readonly manager: ClangdContextManager) {}

  get multiProject() { return this.manager.multiProject; }

  getLanguageClient(uri?: vscode.Uri): BaseLanguageClient|undefined {
    return clientOf(uri ? this.manager.contextFor(uri)
                        : this.manager.currentContext);
  }

  getAllLanguageClients(): BaseLanguageClient[] {
    return this.manager.contexts.flatMap(c => clientOf(c) ?? []);
  }

  async startLanguageClient(uri: vscode.Uri):
      Promise<BaseLanguageClient|undefined> {
    await this.manager.ready();
    const context = await this.manager.ensureContext(uri);
    if (!context)
      return undefined;
    await context.clientSettled();
    return context.clientIsRunning() ? context.client : undefined;
  }

  get onDidChangeClients() { return this.manager.onDidChangeClients; }

  get projects() { return this.manager.projects; }

  getProject(uri: vscode.Uri): ClangdProject|undefined {
    return this.manager.projectFor(uri);
  }

  get activeProject() { return this.manager.activeProject; }

  get onDidChangeActiveProject() {
    return this.manager.onDidChangeActiveProject;
  }

  get onDidChangeProjects() { return this.manager.onDidChangeProjects; }

  registerProjectProvider(id: string,
                          provider: ClangdProjectProvider): vscode.Disposable {
    return this.manager.registerProjectProvider(id, provider);
  }
}

export class ClangdExtensionImpl implements ClangdExtension {
  private readonly apiV2: ClangdApiV2;

  constructor(private readonly manager: ClangdContextManager) {
    this.apiV2 = new ClangdApiV2Impl(manager);
  }

  public getApi(version: 1): ClangdApiV1;
  public getApi(version: 2): ClangdApiV2;
  public getApi(version: number): unknown {
    if (version === 1)
      return {languageClient: clientOf(this.manager.currentContext)};
    if (version === 2)
      return this.apiV2;

    throw new Error(`No API version ${version} found`);
  }
}
