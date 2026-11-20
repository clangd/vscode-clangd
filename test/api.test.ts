import * as assert from 'assert';
import {BaseLanguageClient} from 'vscode-languageclient';

import {ClangdExtensionImpl} from '../src/api';

suite('API', () => {
  test('keeps a previously returned API connected to the active client', () => {
    const first = {} as BaseLanguageClient;
    const second = {} as BaseLanguageClient;
    const extension = new ClangdExtensionImpl(first);
    const api = extension.getApi(1);

    extension.client = second;

    assert.strictEqual(api.languageClient, second);
  });
});
