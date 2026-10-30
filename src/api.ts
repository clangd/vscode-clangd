import {commands} from 'vscode';
import {BaseLanguageClient} from 'vscode-languageclient';

import {ClangdApiV1, ClangdExtension} from '../api/vscode-clangd';

// Module-level override for --compile-commands-dir. Set via the public
// ClangdApiV1.setCompilationDatabasePath; read by ClangdContext.createClient
// when launching clangd. Lives here so the API object returned by getApi()
// can update it without holding a reference to the ClangdContext instance.
let compilationDatabasePathOverride: string|undefined;

export function setCompilationDatabasePathOverride(path: string|undefined):
    void {
  compilationDatabasePathOverride = path;
}

export function getCompilationDatabasePathOverride(): string|undefined {
  return compilationDatabasePathOverride;
}

export class ClangdExtensionImpl implements ClangdExtension {
  constructor(public client: BaseLanguageClient|undefined) {}

  public getApi(version: 1): ClangdApiV1;
  public getApi(version: number): unknown {
    if (version === 1) {
      return {
        languageClient: this.client,
        setCompilationDatabasePath: async (path: string): Promise<void> => {
          setCompilationDatabasePathOverride(path);
          await commands.executeCommand('clangd.restart');
        },
      };
    }

    throw new Error(`No API version ${version} found`);
  }
}
