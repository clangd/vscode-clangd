import * as assert from 'assert';
import * as vscode from 'vscode';

import {ClangdApiV2} from '../api/vscode-clangd';
import {ClangdExtensionImpl} from '../src/api';
import {ClangdContextManager} from '../src/clangd-context-manager';

import {
  cppEditor as editor,
  FakeContext,
  ManagerHarness,
  MockTextDocument,
  MockTextEditor
} from './mocks';

suite('API', () => {
  const file = (p: string) => vscode.Uri.file(p);
  const fileA = file('/w/a/x.cpp');
  const fileB = file('/w/b/y.cpp');

  let harness: ManagerHarness;
  let manager: ClangdContextManager;
  let extension: ClangdExtensionImpl;
  let api: ClangdApiV2;

  setup(() => {
    harness = new ManagerHarness();
    harness.setFolders(['/w/a', '/w/b']);
    manager = harness.manager;
    extension = new ClangdExtensionImpl(manager);
    api = extension.getApi(2);
  });

  teardown(() => { harness.dispose(); });

  const clientOf = (context: FakeContext) =>
      context.client as unknown as ReturnType<ClangdApiV2['getLanguageClient']>;

  test('getApi supports versions 1 and 2', async () => {
    harness.multiProject = false;
    await manager.start();
    assert.strictEqual(extension.getApi(1).languageClient,
                       clientOf(harness.contexts[0]));
    assert.strictEqual(extension.getApi(2), api);
    assert.throws(() => extension.getApi(3 as any), /No API version 3 found/);
  });

  test('API v1 has no client while clangd is stopped', async () => {
    harness.multiProject = false;
    await manager.start();
    harness.contexts[0].started = false;
    assert.strictEqual(extension.getApi(1).languageClient, undefined);
  });

  test('default mode has one client', async () => {
    harness.multiProject = false;
    await manager.start();
    const client = clientOf(harness.contexts[0]);
    assert.strictEqual(api.multiProject, false);
    assert.strictEqual(api.getLanguageClient(), client);
    assert.strictEqual(api.getLanguageClient(file('/elsewhere/x.cpp')), client);
    assert.deepStrictEqual(api.getAllLanguageClients(), [client]);
    assert.strictEqual(await api.startLanguageClient(fileA), client);
  });

  test('multi mode routes files to the clients of their projects', async () => {
    harness.visibleEditors = [editor(fileA.path), editor(fileB.path)];
    await manager.start();
    const [a, b] = harness.contexts;
    assert.strictEqual(api.multiProject, true);
    assert.strictEqual(api.getLanguageClient(fileA), clientOf(a));
    assert.strictEqual(api.getLanguageClient(fileB), clientOf(b));
    assert.strictEqual(api.getLanguageClient(file('/w/c/z.cpp')), undefined);
    assert.deepStrictEqual(api.getAllLanguageClients(),
                           [clientOf(a), clientOf(b)]);
  });

  test('getLanguageClient() follows the focused clangd editor', async () => {
    const editorA = editor(fileA.path);
    const editorB = editor(fileB.path);
    harness.visibleEditors = [editorA, editorB];
    harness.activeEditor = editorA;
    await manager.start();
    const [a, b] = harness.contexts;
    assert.strictEqual(api.getLanguageClient(), clientOf(a));
    harness.setActiveEditor(editorB);
    assert.strictEqual(api.getLanguageClient(), clientOf(b));
    // Focusing a file that isn't a C/C++ file keeps the client.
    harness.setActiveEditor(
        new MockTextEditor(new MockTextDocument(file('/w/notes.txt'))));
    assert.strictEqual(api.getLanguageClient(), clientOf(b));
    // A C/C++ file outside all projects has no client.
    harness.setActiveEditor(editor('/w/c/z.cpp'));
    assert.strictEqual(api.getLanguageClient(), undefined);
  });

  test('stopped clients are left out', async () => {
    harness.visibleEditors = [editor(fileA.path)];
    await manager.start();
    harness.contexts[0].started = false;
    assert.strictEqual(api.getLanguageClient(fileA), undefined);
    assert.deepStrictEqual(api.getAllLanguageClients(), []);
    assert.strictEqual(await api.startLanguageClient(fileA), undefined);
  });

  test('startLanguageClient starts the client of a project', async () => {
    await manager.start();
    assert.strictEqual(harness.contexts.length, 0);
    let changes = 0;
    api.onDidChangeClients(() => changes++);

    const client = await api.startLanguageClient(fileB);
    assert.ok(client);
    assert.strictEqual(api.getLanguageClient(fileB), client);
    assert.strictEqual(harness.contexts.length, 1);
    assert.strictEqual(changes, 1);
    assert.strictEqual(await api.startLanguageClient(file('/w/c/z.cpp')),
                       undefined);

    await manager.stop(fileB);
    assert.strictEqual(changes, 2);
    assert.deepStrictEqual(api.getAllLanguageClients(), []);
    assert.strictEqual(await api.startLanguageClient(fileB), undefined);
  });

  test('startLanguageClient waits for a restart', async () => {
    await manager.start();
    const restarted = manager.restart();
    const client = await api.startLanguageClient(fileA);
    await restarted;
    assert.ok(client);
    assert.strictEqual(api.getLanguageClient(fileA), client);
  });
});
