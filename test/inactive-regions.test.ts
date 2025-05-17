import * as sinon from 'sinon';
import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext} from '../src/clangd-context';
import * as config from '../src/config';
import * as inactiveRegions from '../src/inactive-regions';

import * as mocks from './mocks';

class MockClangdContext implements ClangdContext {
  subscriptions: vscode.Disposable[] = [];
  client = new vscodelc.LanguageClient('', {command: ''}, {});

  visibleClangdEditors: vscode.TextEditor[] = [];

  async activate() { throw new Error('Method not implemented.'); }

  clientIsStarting() { return false; }

  dispose() { throw new Error('Method not implemented.'); }
}

suite('InactiveRegionsFeature', () => {
  let sandbox: sinon.SinonSandbox;

  setup(() => {
    sandbox = sinon.createSandbox();
    sandbox.stub(config, 'get')
        .withArgs('inactiveRegions.opacity')
        .returns(Promise.resolve(0.55));
  });

  teardown(() => { sandbox.restore(); });

  test('highlights correctly', async () => {
    const context = new MockClangdContext();
    const feature = new inactiveRegions.InactiveRegionsFeature(context);
    const serverCapabilities: vscodelc.ServerCapabilities&
        {inactiveRegionsProvider?: any} = {inactiveRegionsProvider: true};
    feature.initialize(serverCapabilities, undefined);

    const document = new mocks.MockTextDocument(vscode.Uri.file('/foo.c'), 'c');
    const editor = new mocks.MockTextEditor(document);
    const stub = sandbox.stub(editor, 'setDecorations').returns();
    context.visibleClangdEditors = [editor];

    feature.handleNotification({
      textDocument: {uri: 'file:///foo.c', version: 0},
      regions: [{start: {line: 0, character: 1}, end: {line: 2, character: 3}}]
    });

    sandbox.assert.calledOnceWithExactly(stub, sinon.match.truthy,
                                         [new vscode.Range(0, 1, 2, 3)]);

    feature.handleNotification({
      textDocument: {uri: 'file:///foo.c', version: 0},
      regions: [
        {start: {line: 10, character: 1}, end: {line: 20, character: 2}},
        {start: {line: 30, character: 3}, end: {line: 40, character: 4}}
      ]
    });

    sandbox.assert.calledTwice(stub);
    sandbox.assert.calledWithExactly(
        stub.getCall(1), sinon.match.truthy,
        [new vscode.Range(10, 1, 20, 2), new vscode.Range(30, 3, 40, 4)]);
  });

  test('handles URIs sent from clangd server', async () => {
    const context = new MockClangdContext();
    const feature = new inactiveRegions.InactiveRegionsFeature(context);
    const serverCapabilities: vscodelc.ServerCapabilities&
        {inactiveRegionsProvider?: any} = {inactiveRegionsProvider: true};
    feature.initialize(serverCapabilities, undefined);

    const uris = [
      {
        vscodeUri: vscode.Uri.file('/path/to/source.c'),
        fromServer: 'file:///path/to/source.c'
      },
      {
        // Clangd server would encode colons but `vscode.Uri.toString()`
        // doesn't. See #515 for details.
        vscodeUri: vscode.Uri.file('C:/path/to/source.c'),
        fromServer: 'file:///C:/path/to/source.c'
      },
      {
        vscodeUri: vscode.Uri.file('/föö/bör/#[0].c'),
        fromServer: 'file:///f%C3%B6%C3%B6/b%C3%B6r/%23%5B0%5D.c'
      }
    ];

    for (const {vscodeUri, fromServer} of uris) {
      const document = new mocks.MockTextDocument(vscodeUri, 'c');
      const editor = new mocks.MockTextEditor(document);
      const stub = sandbox.stub(editor, 'setDecorations').returns();
      context.visibleClangdEditors = [editor];

      feature.handleNotification({
        textDocument: {uri: fromServer, version: 0},
        regions:
            [{start: {line: 0, character: 1}, end: {line: 2, character: 3}}]
      });

      sandbox.assert.calledOnceWithExactly(stub, sinon.match.truthy,
                                           [new vscode.Range(0, 1, 2, 3)]);
    }
  });
});
