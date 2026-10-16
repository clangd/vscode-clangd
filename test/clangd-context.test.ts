import * as assert from 'assert';
import * as sinon from 'sinon';
import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext} from '../src/clangd-context';
import {ClangdContextManager} from '../src/clangd-context-manager';
import * as config from '../src/config';
import * as configFileWatcher from '../src/config-file-watcher';
import * as inactiveRegions from '../src/inactive-regions';
import * as inlayHints from '../src/inlay-hints';

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

  // Resolves once pending promise callbacks have run.
  const settle = () => new Promise(resolve => setTimeout(resolve, 0));

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
