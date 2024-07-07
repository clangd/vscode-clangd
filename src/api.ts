import {BaseLanguageClient} from 'vscode-languageclient';

import {ClangdApiV1, ClangdExtension} from '../api/vscode-clangd';

export class ClangdExtensionImpl implements ClangdExtension {
  constructor(public client: BaseLanguageClient) {}

  public getApi(version: 1): ClangdApiV1;
  public getApi(version: number): unknown {
    if (version === 1) {
      return {languageClient: this.client};
    }

    throw new Error(`No API version ${version} found`);
  }
}
