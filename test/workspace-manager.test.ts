import * as assert from 'assert';
import * as sinon from 'sinon';
import * as vscode from 'vscode';

import {ClangdContext, documentSelector} from '../src/clangd-context';
import {ClangdInstaller} from '../src/install';
import {
  ClangdContextFactory,
  WorkspaceManager,
} from '../src/workspace-manager';

import * as mocks from './mocks';

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

  test('retries a folder after client startup fails', async () => {
    const folder = workspaceFolder('/workspace/one', 'one', 0);
    const factory = sandbox.stub();
    factory.rejects(new Error('clangd failed to start'));
    const manager = new WorkspaceManager(
        {} as ClangdInstaller, '/storage', {} as vscode.LogOutputChannel,
        () => {}, factory as ClangdContextFactory);
    try {
      await assert.rejects(manager.startForFolder(folder));
      await assert.rejects(manager.startForFolder(folder));
      assert.strictEqual(factory.callCount, 2);
    } finally {
      manager.dispose();
    }
  });

  test('does not start clients from document events after shutdown',
       async () => {
         let onOpen!: (document: vscode.TextDocument) => unknown;
         sandbox.stub(vscode.workspace, 'onDidOpenTextDocument')
             .callsFake(listener => {
               onOpen = listener;
               return {dispose() {}} as vscode.Disposable;
             });
         sandbox.stub(vscode.workspace, 'textDocuments').value([]);
         const folder = workspaceFolder('/workspace/one', 'one', 0);
         sandbox.stub(vscode.workspace, 'getWorkspaceFolder').returns(folder);
         const factory = sandbox.stub().resolves({} as ClangdContext);
         const manager = new WorkspaceManager(
             {} as ClangdInstaller, '/storage', {} as vscode.LogOutputChannel,
             () => {}, factory as ClangdContextFactory);
         try {
           await manager.start();
           await manager.shutdown();
           onOpen(new mocks.MockTextDocument(
               vscode.Uri.file('/workspace/one/a.cc'), 'cpp'));
           await new Promise(resolve => setImmediate(resolve));
           assert.strictEqual(factory.callCount, 0);
         } finally {
           manager.dispose();
         }
       });

  test(
      'stops the default client when a workspace folder is added', async () => {
        let onFoldersChanged!: (event: vscode.WorkspaceFoldersChangeEvent) =>
            unknown;
        sandbox.stub(vscode.workspace, 'onDidChangeWorkspaceFolders')
            .callsFake(listener => {
              onFoldersChanged = listener;
              return {dispose() {}} as vscode.Disposable;
            });
        sandbox.stub(vscode.workspace, 'textDocuments').value([]);
        const folder = workspaceFolder('/workspace/one', 'one', 0);
        let folders: readonly vscode.WorkspaceFolder[]|undefined = [];
        sandbox.stub(vscode.workspace, 'workspaceFolders').get(() => folders);
        const stop = sandbox.spy();
        const factory: ClangdContextFactory =
            async (_installer, _storage, _output, _provider, contextFolder) => {
          return {
            folder: contextFolder,
            client: {},
            subscriptions: [],
            stop: async () => { stop(); },
          } as unknown as ClangdContext;
        };
        const manager = new WorkspaceManager({} as ClangdInstaller, '/storage',
                                             {} as vscode.LogOutputChannel,
                                             () => {}, factory);
        try {
          await manager.startForFolder(undefined);
          folders = [folder];
          onFoldersChanged({added: [folder], removed: []});
          await new Promise(resolve => setImmediate(resolve));
          assert.strictEqual(stop.callCount, 1);
        } finally {
          manager.dispose();
        }
      });
});

function workspaceFolder(path: string, name: string,
                         index: number): vscode.WorkspaceFolder {
  return {uri: vscode.Uri.file(path), name, index};
}
