import {ClangdApiV1, ClangdExtension} from '../api/vscode-clangd';

import {ClangdContextManager} from './clangd-context-manager';

export class ClangdExtensionImpl implements ClangdExtension {
  constructor(private readonly manager: ClangdContextManager) {}

  public getApi(version: 1): ClangdApiV1;
  public getApi(version: number): unknown {
    if (version === 1) {
      const context = this.manager.currentContext;
      return {
        languageClient: context?.clientIsStartingOrRunning() ? context.client
                                                             : undefined
      };
    }

    throw new Error(`No API version ${version} found`);
  }
}
