import * as assert from 'assert';
import * as sinon from 'sinon';
import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {
  capabilityContextKey,
  ClangdContext,
  contextForCommand
} from '../src/clangd-context';
import {ClangdContextManager} from '../src/clangd-context-manager';
import * as config from '../src/config';
import * as configFileWatcher from '../src/config-file-watcher';
import * as inactiveRegions from '../src/inactive-regions';
import * as inlayHints from '../src/inlay-hints';

import {settle} from './mocks';

suite('ClangdContext', () => {
  let sandbox: sinon.SinonSandbox;
  let context: ClangdContext;
  let state: vscodelc.State;
  let stateEmitter: vscode.EventEmitter<vscodelc.StateChangeEvent>;
  let stop: sinon.SinonStub;

  setup(async () => {
    sandbox = sinon.createSandbox();
    const values: {[key: string]: unknown} = {arguments: [], fallbackFlags: []};
    sandbox.stub(config, 'get')
        .callsFake((async (key: string) => values[key]) as typeof config.get);
    context = await ClangdContext.create(
        'clangd', {} as vscode.LogOutputChannel, {} as ClangdContextManager);
    state = vscodelc.State.Stopped;
    stateEmitter = new vscode.EventEmitter();
    sandbox.stub(context.client, 'state').get(() => state);
    sandbox.stub(context.client, 'onDidChangeState')
        .get(() => stateEmitter.event);
    stop = sandbox.stub(context.client, 'stop').resolves();
  });

  teardown(() => { sandbox.restore(); });

  // Simulates startClient() with a client that is starting.
  function startClient(): () => void {
    let resolve!: () => void;
    context['clientStarted'] = new Promise<void>(r => resolve = r);
    state = vscodelc.State.Starting;
    return resolve;
  }

  function setState(newState: vscodelc.State) {
    const oldState = state;
    state = newState;
    stateEmitter.fire({oldState, newState});
  }

  test('stops a client that was never started', async () => {
    context.dispose();
    await settle();
    assert.ok(stop.calledOnce);
  });

  test('stops a starting client once it runs', async () => {
    const started = startClient();
    context.dispose();
    await settle();
    assert.ok(stop.notCalled);
    setState(vscodelc.State.Running);
    started();
    await settle();
    assert.ok(stop.calledOnce);
  });

  test('waits for a client restarting after a crash', async () => {
    const started = startClient();
    started();
    await settle();
    // The client crashed and is starting again.
    context.dispose();
    await settle();
    assert.ok(stop.notCalled);
    setState(vscodelc.State.Running);
    await settle();
    assert.ok(stop.calledOnce);
  });

  test(
      'features initialized after dispose() subscribe to nothing', async () => {
        const features: vscodelc.StaticFeature[] = [];
        sandbox.stub(context.client, 'registerFeature').callsFake((feature) => {
          features.push(feature as vscodelc.StaticFeature);
        });
        // The features that subscribe once the client has initialized.
        await configFileWatcher.activate(context);
        inactiveRegions.activate(context);
        inlayHints.activate(context);
        context.dispose();
        // The client finishes starting and initializes the features.
        const capabilities = {
          clangdInlayHintsProvider: true,
          inactiveRegionsProvider: true
        } as vscodelc.ServerCapabilities;
        for (const feature of features)
          await feature.initialize(capabilities, undefined);
        await settle();
        assert.strictEqual(features.length, 3);
        assert.strictEqual(context.subscriptions.length, 0);
      });

  test('ignores a failing stop()', async () => {
    stop.rejects(new Error('Stopping the server timed out'));
    context.dispose();
    await settle();
    assert.ok(stop.calledOnce);
  });
});

suite('Feature commands', () => {
  let sandbox: sinon.SinonSandbox;
  let contexts: {client: unknown, clientIsRunning: () => boolean}[];
  let clientsChanged: vscode.EventEmitter<void>;
  let manager: ClangdContextManager;

  setup(() => {
    sandbox = sinon.createSandbox();
    contexts = [];
    clientsChanged = new vscode.EventEmitter();
    manager = {
      multiProject: true,
      get contexts() { return contexts; },
      onDidChangeClients: clientsChanged.event,
      contextFor: () => undefined,
      projectFor: (uri: vscode.Uri) =>
          uri.path.startsWith('/w/') ? {uri: vscode.Uri.file('/w')} : undefined,
    } as unknown as ClangdContextManager;
  });

  teardown(() => { sandbox.restore(); });

  function runningContext(capabilities: object) {
    return {
      client: {initializeResult: {capabilities}},
      clientIsRunning: () => true
    };
  }

  test('a capability context key is set while a running client supports it',
       async () => {
         const setContext =
             sandbox.stub(vscode.commands, 'executeCommand').resolves();
         const key = capabilityContextKey(manager, 'test.supported',
                                          (c) => 'astProvider' in c);
         contexts.push(runningContext({}));
         clientsChanged.fire();
         contexts.push(runningContext({astProvider: true}));
         clientsChanged.fire();
         contexts.pop();
         clientsChanged.fire();
         key.dispose();
         assert.deepStrictEqual(setContext.args.map(a => a[2]),
                                [false, false, true, false]);
       });

  test('a command on a file outside all projects says so', () => {
    const message =
        sandbox.stub(vscode.window, 'showInformationMessage').resolves();
    assert.strictEqual(
        contextForCommand(manager, vscode.Uri.file('/elsewhere/a.cpp')),
        undefined);
    assert.strictEqual(contextForCommand(manager, vscode.Uri.file('/w/a.cpp')),
                       undefined);
    assert.deepStrictEqual(message.args.map(a => a[0]), [
      'This file is not part of any clangd project',
      'clangd is not running for this file'
    ]);
  });
});
