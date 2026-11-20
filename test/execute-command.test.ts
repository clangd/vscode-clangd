import * as assert from 'assert';
import * as sinon from 'sinon';
import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import type {ClangdContext} from '../src/clangd-context';
import type {ClangdClientProvider} from '../src/client-provider';
import {ExecuteCommandRouter} from '../src/execute-command';

type ExecuteCommandClient =
    vscodelc.FeatureClient<vscodelc.ExecuteCommandMiddleware>;

suite('ExecuteCommandRouter', () => {
  test('registers shared commands once and routes them by argument URI',
       async () => {
         const command = 'clangd.test.multiRootExecuteCommand';
         const one = fakeClient();
         const two = fakeClient();
         const twoUri = vscode.Uri.file('/workspace/two/b.cc');
         const provider = {
           contextForUri: (uri: vscode.Uri) => {
             if (uri.fsPath.startsWith('/workspace/one'))
               return {client: one} as unknown as ClangdContext;
             if (uri.fsPath.startsWith('/workspace/two'))
               return {client: two} as unknown as ClangdContext;
             return undefined;
           },
           activeContext: () => undefined,
         } as unknown as ClangdClientProvider;
         const router = new ExecuteCommandRouter(provider);
         try {
           router.register(one, 'one', [command]);
           router.register(two, 'two', [command]);

           await vscode.commands.executeCommand(command,
                                                {file: twoUri.toString()});

           assert.strictEqual(one.sendRequest.callCount, 0);
           assert.strictEqual(two.sendRequest.callCount, 1);
           assert.deepStrictEqual(two.sendRequest.firstCall.args[1], {
             command,
             arguments: [{file: twoUri.toString()}],
           });
         } finally {
           router.dispose();
         }
       });

  test('preserves execute-command middleware replacements', async () => {
    const command = 'clangd.test.executeCommandMiddleware';
    const client = fakeClient();
    client.middleware.executeCommand = (_command, _args, next) =>
        next('clangd.replaced', ['replacement']);
    const provider = {
      contextForUri: () => undefined,
      activeContext: () => undefined,
    } as unknown as ClangdClientProvider;
    const router = new ExecuteCommandRouter(provider);
    try {
      router.register(client, 'client', [command]);

      await vscode.commands.executeCommand(command);

      assert.deepStrictEqual(client.sendRequest.firstCall.args[1], {
        command: 'clangd.replaced',
        arguments: ['replacement'],
      });
    } finally {
      router.dispose();
    }
  });
});

function fakeClient(): ExecuteCommandClient&{sendRequest: sinon.SinonStub} {
  return {
    middleware: {},
    sendRequest: sinon.stub().resolves(undefined),
    handleFailedRequest: sinon.stub(),
  } as unknown as ExecuteCommandClient &
         {sendRequest: sinon.SinonStub};
}
