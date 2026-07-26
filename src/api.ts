import * as vscode from 'vscode';
import {BaseLanguageClient} from 'vscode-languageclient';

import {ClangdApiV1, ClangdApiV2, ClangdExtension} from '../api/vscode-clangd';

import {ClangdContextManager} from './clangd-context-manager';

export class ClangdExtensionImpl implements ClangdExtension {
  private readonly _onDidChangeClients = new vscode.EventEmitter<void>();

  constructor(private readonly manager: ClangdContextManager) {
    // Fire the event when contexts are created or disposed
    manager.onDidCreateContext(() => this._onDidChangeClients.fire());
    manager.onDidDisposeContext(() => this._onDidChangeClients.fire());
  }

  public getApi(version: 1): ClangdApiV1;
  public getApi(version: 2): ClangdApiV2;
  public getApi(version: number): unknown {
    if (version === 1) {
      // V1 backward compatibility: return the active context's client
      const manager = this.manager;
      return {
        get languageClient() { return manager.getActiveContext()?.client; },
      };
    }

    if (version === 2) {
      return {
        getLanguageClient: (uri?: vscode.Uri):
                                   BaseLanguageClient | undefined => {
          if (uri) {
            return this.manager.getContextForUri(uri)?.client;
          }
          return this.manager.getActiveContext()?.client;
        },
        getAllLanguageClients: (): BaseLanguageClient[] => {
          const clients: BaseLanguageClient[] = [];
          for (const ctx of this.manager.getAllContexts()) {
            if (ctx.client) {
              clients.push(ctx.client);
            }
          }
          return clients;
        },
        onDidChangeClients: this._onDidChangeClients.event,
      };
    }

    throw new Error(`No API version ${version} found`);
  }

  dispose(): void { this._onDidChangeClients.dispose(); }
}
