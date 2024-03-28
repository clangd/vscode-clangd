import {BaseLanguageClient} from 'vscode-languageclient';

export interface ClangdApiV1 {
  languageClient: BaseLanguageClient
}

export interface ClangdExtension {
  getApi(version: 1): ClangdApiV1;
}

export class ClangdExtensionImpl implements ClangdExtension {
  constructor(
    private readonly client: BaseLanguageClient
  ) { }

  public getApi(version: 1): ClangdApiV1;
  public getApi(version: number): unknown {
    if (version === 1) {
      return {
        languageClient: this.client
      };
    }

    throw new Error(`No API version ${version} found`);
  }
}
