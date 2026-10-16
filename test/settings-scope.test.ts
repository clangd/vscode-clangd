import * as assert from 'assert';
import * as sinon from 'sinon';
import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext} from '../src/clangd-context';
import {ClangdContextManager} from '../src/clangd-context-manager';
import * as config from '../src/config';
import * as configFileWatcher from '../src/config-file-watcher';
import * as inactiveRegions from '../src/inactive-regions';

import * as mocks from './mocks';

suite('Resource-scoped settings', () => {
  const folder = {uri: vscode.Uri.file('/w'), name: 'w', index: 0};
  const project = {uri: vscode.Uri.file('/w/p'), name: 'p'};
  const resourceSettings = [
    'arguments', 'fallbackFlags', 'trace', 'restartAfterCrash',
    'onConfigChanged', 'onConfigChangedForceEnable', 'enableCodeCompletion',
    'enableHover', 'serverCompletionRanking',
    'inactiveRegions.useBackgroundHighlight', 'inactiveRegions.opacity'
  ];

  let sandbox: sinon.SinonSandbox;
  let get: sinon.SinonStub;

  setup(() => {
    sandbox = sinon.createSandbox();
    sandbox.stub(vscode.workspace, 'getWorkspaceFolder')
        .callsFake((uri: vscode.Uri) =>
                       uri.path.startsWith('/w/') ? folder : undefined);
    const values: {[key: string]: unknown} = {
      'arguments': [],
      'fallbackFlags': [],
      'enableCodeCompletion': true,
      'enableHover': false,
      'onConfigChanged': 'prompt',
      'inactiveRegions.opacity': 0.5
    };
    get = sandbox.stub(config, 'get')
              .callsFake((async (key: string) => values[key]) as
                         typeof config.get);
  });

  teardown(() => { sandbox.restore(); });

  // The scopes in which `key` was read.
  function scopes(key: string) {
    return get.getCalls()
        .filter(call => call.args[0] === key)
        .map(call => call.args[1]);
  }

  async function createContext(withProject: boolean) {
    const channel = {} as vscode.LogOutputChannel;
    const context = await ClangdContext.create(
        'clangd', channel, {} as ClangdContextManager,
        withProject ? project : undefined);
    const {provideHover, provideCompletionItem} =
        context.client.clientOptions.middleware ?? {};
    assert.ok(provideHover && provideCompletionItem);
    const document =
        new mocks.MockTextDocument(vscode.Uri.file('/w/p/a.cpp'), 'cpp');
    const position = new vscode.Position(0, 0);
    const token = new vscode.CancellationTokenSource().token;
    const disabled = () => { throw new Error('hovers are disabled'); };
    await provideHover(document, position, token, disabled);
    const completionContext = {
      triggerKind: vscode.CompletionTriggerKind.Invoke,
      triggerCharacter: undefined
    };
    const noItems = async () => [];
    await provideCompletionItem(document, position, completionContext, token,
                                noItems);
    const registerFeature = sandbox.stub(context.client, 'registerFeature');
    await configFileWatcher.activate(context);
    // The config file watcher reads onConfigChangedForceEnable once the
    // server's capabilities are known.
    const watcher = registerFeature.firstCall.args[0];
    await watcher.initialize({compilationDatabase: {automaticReload: true}} as
                                 vscodelc.ServerCapabilities,
                             undefined);
    const feature = new inactiveRegions.InactiveRegionsFeature(context);
    await feature.updateDecorationType();
    feature.clear();
    return context;
  }

  test('are read in the folder of the project', async () => {
    const context = await createContext(true);
    assert.strictEqual(context.workspaceFolder, folder);
    for (const key of resourceSettings) {
      assert.ok(scopes(key).length > 0, `${key} was not read`);
      assert.ok(scopes(key).every(scope => scope === folder),
                `${key} was read outside the folder scope`);
    }
    assert.deepStrictEqual(scopes('useScriptAsExecutable'), [undefined]);
  });

  test('are read without a scope in the default mode', async () => {
    const context = await createContext(false);
    assert.strictEqual(context.workspaceFolder, undefined);
    for (const key of resourceSettings)
      assert.ok(scopes(key).every(scope => scope === undefined),
                `${key} was read in a folder scope`);
  });
});
