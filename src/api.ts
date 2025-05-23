import {ClangdApiV1, ClangdApiV2, ClangdExtension} from '../api/vscode-clangd';
import {ClangdContext} from './clangd-context';

export class ClangdExtensionImpl implements ClangdExtension {
  constructor(public context: ClangdContext) {}

  public getApi(version: 1): ClangdApiV1;
  public getApi(version: 2): ClangdApiV2;
  public getApi(version: number): unknown {
    if (version === 1) {
      return {languageClient: this.context.clients.entries().next().value?.[1]};
    }

    if (version === 2) {
      return {languageClients: [...this.context.clients.values()]};
    }

    throw new Error(`No API version ${version} found`);
  }
}
