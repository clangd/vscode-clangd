import * as assert from 'assert';
import * as sinon from 'sinon';
import * as vscode from 'vscode';

import {ClangdContext, documentSelector} from '../src/clangd-context';
import {ClangdInstaller} from '../src/install';
import {
  ClangdContextFactory,
  WorkspaceManager,
} from '../src/workspace-manager';

suite('WorkspaceManager', () => {
  let sandbox: sinon.SinonSandbox;

  setup(() => { sandbox = sinon.createSandbox(); });
  teardown(() => { sandbox.restore(); });

  test('uses a folder-restricted document selector', () => {
    const folder = workspaceFolder('/workspace/one', 'one', 0);
    const selector = documentSelector(folder) as vscode.DocumentFilter[];
    assert.ok(selector.length > 0);
    for (const filter of selector) {
      const pattern = filter.pattern as vscode.RelativePattern;
      assert.strictEqual(pattern.baseUri.toString(), folder.uri.toString());
      assert.strictEqual(pattern.pattern, '**/*');
    }
  });

  test('owns an independent context for each workspace folder', async () => {
    const one = workspaceFolder('/workspace/one', 'one', 0);
    const two = workspaceFolder('/workspace/two', 'two', 1);
    sandbox.stub(vscode.workspace, 'getWorkspaceFolder').callsFake(uri => {
      if (uri.fsPath.startsWith(one.uri.fsPath))
        return one;
      if (uri.fsPath.startsWith(two.uri.fsPath))
        return two;
      return undefined;
    });

    const created: vscode.WorkspaceFolder[] = [];
    const disposed: sinon.SinonSpy[] = [];
    const factory: ClangdContextFactory =
        async (_installer, _storage, _output, _provider, folder) => {
      assert.ok(folder);
      created.push(folder);
      const dispose = sandbox.spy();
      disposed.push(dispose);
      return {
        folder,
        client: {},
        subscriptions: [],
        dispose,
        stop: async () => { dispose(); },
        clientIsRunning: () => true,
        clientIsStarting: () => false,
      } as unknown as ClangdContext;
    };

    const manager =
        new WorkspaceManager({} as ClangdInstaller, '/storage',
                             {} as vscode.LogOutputChannel, () => {}, factory);
    try {
      const contextOne = await manager.startForFolder(one);
      const contextTwo = await manager.startForFolder(two);

      assert.notStrictEqual(contextOne, contextTwo);
      assert.strictEqual(
          manager.contextForUri(vscode.Uri.file('/workspace/one/a.cc')),
          contextOne);
      assert.strictEqual(
          manager.contextForUri(vscode.Uri.file('/workspace/two/b.cc')),
          contextTwo);
      assert.deepStrictEqual(created, [one, two]);

      await manager.restart(one);
      assert.strictEqual(disposed[0].callCount, 1);
      assert.strictEqual(disposed[1].callCount, 0);
      assert.deepStrictEqual(created, [one, two, one]);

      await manager.shutdown();
      assert.strictEqual(disposed[1].callCount, 1);
      assert.strictEqual(disposed[2].callCount, 1);
    } finally {
      manager.dispose();
    }
  });
});

function workspaceFolder(path: string, name: string,
                         index: number): vscode.WorkspaceFolder {
  return {uri: vscode.Uri.file(path), name, index};
}
