import {BaseLanguageClient} from 'vscode-languageclient';

import {ClangdApiV1, ClangdExtension} from '../api/vscode-clangd';

export class ClangdExtensionImpl implements ClangdExtension {
  private readonly apiV1: ClangdApiV1;

  constructor(public client: BaseLanguageClient|undefined) {
    const extension = this;
    this.apiV1 = {
      get languageClient() { return extension.client; },
    };
  }

  public getApi(version: 1): ClangdApiV1;
  public getApi(version: number): unknown {
    if (version === 1) {
      return this.apiV1;
    }

    throw new Error(`No API version ${version} found`);
  }
}
