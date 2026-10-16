import * as assert from 'assert';
import * as sinon from 'sinon';
import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {
  ServerCommandRegistry,
  SharedServerCommandsFeature
} from '../src/server-commands';

// Commands that don't collide with the ones of the extension under test.
const applyFix = 'clangd.test.applyFix';
const applyTweak = 'clangd.test.applyTweak';

function fakeClient(): vscodelc.BaseLanguageClient {
  return {
    sendRequest: sinon.stub().resolves('done'),
    handleFailedRequest: sinon.stub().returns(undefined),
  } as unknown as vscodelc.BaseLanguageClient;
}

async function isRegistered(command: string) {
  return (await vscode.commands.getCommands(true)).includes(command);
}

suite('ServerCommandRegistry', () => {
  const uriA = vscode.Uri.file('/w/a/x.cpp');
  const uriB = vscode.Uri.file('/w/b/y.cpp');
  let clientA: vscodelc.BaseLanguageClient;
  let clientB: vscodelc.BaseLanguageClient;
  let activeDocument: vscode.Uri|undefined;
  let registry: ServerCommandRegistry;
  let sandbox: sinon.SinonSandbox;

  setup(() => {
    clientA = fakeClient();
    clientB = fakeClient();
    activeDocument = undefined;
    sandbox = sinon.createSandbox();
    sandbox.stub(vscode.window, 'activeTextEditor')
        .get(() => activeDocument && {document: {uri: activeDocument}});
    registry = new ServerCommandRegistry(
        (uri) => uri.path.startsWith('/w/a/')   ? clientA
                 : uri.path.startsWith('/w/b/') ? clientB
                                                : undefined);
  });

  teardown(() => {
    registry.dispose();
    sandbox.restore();
  });

  test('registers each command once', async () => {
    const register = sinon.spy(vscode.commands, 'registerCommand');
    try {
      registry.add(applyTweak, clientA);
      registry.add(applyTweak, clientB);
    } finally {
      register.restore();
    }
    assert.strictEqual(register.withArgs(applyTweak).callCount, 1);
    assert.ok(await isRegistered(applyTweak));
  });

  test('dispatches to the client of the argument URI', async () => {
    registry.add(applyTweak, clientA);
    registry.add(applyTweak, clientB);
    registry.add(applyFix, clientA);
    registry.add(applyFix, clientB);
    activeDocument = uriA;

    const tweakArgs = {file: uriB.toString(), selection: {}, tweakID: 'x'};
    assert.strictEqual(
        await vscode.commands.executeCommand(applyTweak, tweakArgs), 'done');
    sinon.assert.calledOnceWithExactly(
        clientB.sendRequest as sinon.SinonStub,
        vscodelc.ExecuteCommandRequest.type,
        {command: applyTweak, arguments: [tweakArgs]});
    sinon.assert.notCalled(clientA.sendRequest as sinon.SinonStub);

    const documentChanges = {
      documentChanges:
          [{textDocument: {uri: uriB.toString(), version: 1}, edits: []}]
    };
    await vscode.commands.executeCommand(applyFix, documentChanges);
    sinon.assert.calledTwice(clientB.sendRequest as sinon.SinonStub);

    const changes = {changes: {[uriB.toString()]: []}};
    await vscode.commands.executeCommand(applyFix, changes);
    sinon.assert.calledThrice(clientB.sendRequest as sinon.SinonStub);
    sinon.assert.notCalled(clientA.sendRequest as sinon.SinonStub);
  });

  test('falls back to the active editor, then the single owner', async () => {
    registry.add(applyTweak, clientA);
    registry.add(applyTweak, clientB);

    activeDocument = uriB;
    await registry.execute(applyTweak, []);
    sinon.assert.calledOnce(clientB.sendRequest as sinon.SinonStub);

    // Neither the arguments nor the active editor tell the owner apart.
    activeDocument = undefined;
    await registry.execute(applyTweak, [{file: 'file:///elsewhere.cpp'}]);
    sinon.assert.calledOnce(clientB.sendRequest as sinon.SinonStub);
    sinon.assert.notCalled(clientA.sendRequest as sinon.SinonStub);

    registry.remove(clientB);
    await registry.execute(applyTweak, []);
    sinon.assert.calledOnce(clientA.sendRequest as sinon.SinonStub);
  });

  test('reports failures through the owning client', async () => {
    const error = new vscodelc.ResponseError(1, 'failed');
    (clientA.sendRequest as sinon.SinonStub).rejects(error);
    registry.add(applyTweak, clientA);
    registry.add(applyTweak, clientB);
    await registry.execute(applyTweak, [{file: uriA.toString()}]);
    sinon.assert.calledOnce(clientA.handleFailedRequest as sinon.SinonStub);
    sinon.assert.calledWith(clientA.handleFailedRequest as sinon.SinonStub,
                            vscodelc.ExecuteCommandRequest.type,
                            sinon.match.any, error);
    sinon.assert.notCalled(clientB.handleFailedRequest as sinon.SinonStub);
  });

  test('unregisters a command after its last owner leaves', async () => {
    registry.add(applyTweak, clientA);
    registry.add(applyTweak, clientB);
    registry.remove(clientA);
    assert.ok(await isRegistered(applyTweak));
    registry.remove(clientB);
    assert.ok(!await isRegistered(applyTweak));
    // It can be registered again.
    registry.add(applyTweak, clientA);
    assert.ok(await isRegistered(applyTweak));
  });

  test('SharedServerCommandsFeature takes over the server commands',
       async () => {
         const initializeResult = {
           capabilities:
               {executeCommandProvider: {commands: [applyFix, applyTweak]}}
         };
         // vscode-languageclient passes a shallow copy to the features.
         const capabilities: vscodelc.ServerCapabilities =
             Object.assign({}, initializeResult.capabilities);
         const feature = new SharedServerCommandsFeature(registry, clientA);
         feature.preInitialize(capabilities);

         assert.deepStrictEqual(capabilities.executeCommandProvider?.commands,
                                []);
         assert.deepStrictEqual(
             initializeResult.capabilities.executeCommandProvider.commands,
             [applyFix, applyTweak]);

         feature.clear();
         assert.ok(!await isRegistered(applyFix));
       });

  test('a detached SharedServerCommandsFeature adds no commands', async () => {
    const feature = new SharedServerCommandsFeature(registry, clientA);
    feature.detach();
    const capabilities: vscodelc.ServerCapabilities = {
      executeCommandProvider: {commands: [applyFix]}
    };
    feature.preInitialize(capabilities);
    assert.deepStrictEqual(capabilities.executeCommandProvider?.commands, []);
    assert.ok(!await isRegistered(applyFix));
  });
});
