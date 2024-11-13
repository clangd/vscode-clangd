import {BaseLanguageClient} from 'vscode-languageclient';

import {ClangdApiV1, ClangdApiV2, ClangdExtension} from '../api/vscode-clangd';

export class ClangdExtensionImpl implements ClangdExtension {
  constructor(public client: BaseLanguageClient|undefined) {}

  public getApi(version: 1): ClangdApiV1;
  public getApi(version: 2): ClangdApiV2;
  public getApi(version: number): unknown {
    switch (version) {
    case 1:
    case 2: {
      return {languageClient: this.client};
      break;
    }
    }

    throw new Error(`No API version ${version} found`);
  }
}
