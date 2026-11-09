import * as assert from 'assert';
import * as vscode from 'vscode';

import {
  ClangdApiV2,
  ClangdProject,
  ClangdProjectsChangeEvent
} from '../api/vscode-clangd';
import {ClangdExtensionImpl} from '../src/api';
import {ClangdContextManager} from '../src/clangd-context-manager';

import {
  cppEditor as editor,
  FakeContext,
  FakeProvider,
  ManagerHarness,
  MockTextDocument,
  MockTextEditor,
  settle
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

  suite('projects', () => {
    const release = {uri: file('/w/release_project'), name: 'release'};
    const debug = {uri: file('/w/debug_project'), name: 'debug'};
    const releaseFile = file('/w/release_project/a.cpp');
    const debugFile = file('/w/debug_project/b.cpp');

    setup(() => { harness.setFolders(['/w']); });

    test('default mode has no projects', async () => {
      harness.multiProject = false;
      const provider = new FakeProvider();
      provider.projects = [release];
      api.registerProjectProvider('test', provider);
      await manager.start();
      assert.deepStrictEqual(api.projects, []);
      assert.strictEqual(api.getProject(releaseFile), undefined);
      assert.strictEqual(api.activeProject, undefined);
      assert.strictEqual(api.getLanguageClient(releaseFile),
                         clientOf(harness.contexts[0]));
    });

    test('a provider routes sibling sub-folders to their own clients',
         async () => {
           harness.visibleEditors =
               [editor(releaseFile.path), editor(debugFile.path)];
           await manager.start();
           // Until a provider registers, the workspace folder is the project.
           assert.deepStrictEqual(api.projects.map(p => p.uri.path), ['/w']);
           assert.strictEqual(harness.contexts.length, 1);

           const events: ClangdProjectsChangeEvent[] = [];
           api.onDidChangeProjects(e => events.push(e));
           const provider = new FakeProvider();
           provider.projects = [release, debug];
           api.registerProjectProvider('test', provider);
           await manager.ready();

           assert.ok(harness.contexts[0].disposed);
           assert.deepStrictEqual(api.projects, [release, debug]);
           assert.strictEqual(events.length, 1);
           assert.deepStrictEqual(events[0].added, [release, debug]);
           assert.deepStrictEqual(events[0].removed.map(p => p.uri.path),
                                  ['/w']);
           const releaseClient = api.getLanguageClient(releaseFile);
           const debugClient = api.getLanguageClient(debugFile);
           assert.ok(releaseClient && debugClient);
           assert.notStrictEqual(releaseClient, debugClient);
           assert.strictEqual(api.getProject(releaseFile), release);
           assert.strictEqual(api.getProject(debugFile), debug);
           assert.strictEqual(api.getLanguageClient(file('/w/other/c.cpp')),
                              undefined);
           assert.strictEqual(
               api.getLanguageClient(file('/w/release_project2/d.cpp')),
               undefined);
           assert.strictEqual(api.getAllLanguageClients().length, 2);
         });

    test('an updated project restarts only its client', async () => {
      harness.visibleEditors =
          [editor(releaseFile.path), editor(debugFile.path)];
      const provider = new FakeProvider();
      provider.projects = [release, debug];
      api.registerProjectProvider('test', provider);
      await manager.start();
      await manager.ready();
      const debugClient = api.getLanguageClient(debugFile);
      const releaseClient = api.getLanguageClient(releaseFile);
      assert.strictEqual(harness.contexts.length, 2);

      const events: ClangdProjectsChangeEvent[] = [];
      api.onDidChangeProjects(e => events.push(e));
      const updated = {...release, arguments: ['--log=verbose']};
      provider.projects = [updated, debug];
      provider.change();
      await manager.ready();

      assert.deepStrictEqual(events,
                             [{added: [], removed: [], updated: [updated]}]);
      assert.strictEqual(harness.contexts.length, 3);
      assert.strictEqual(api.getLanguageClient(debugFile), debugClient);
      const restarted = api.getLanguageClient(releaseFile);
      assert.ok(restarted);
      assert.notStrictEqual(restarted, releaseClient);
      assert.strictEqual(harness.contexts[2].project, updated);
    });

    test('the results of the providers are united', async () => {
      const first = new FakeProvider();
      first.projects = [release];
      const second = new FakeProvider();
      second.projects = [debug, {uri: file('/w/debug_project/nested')}];
      api.registerProjectProvider('first', first);
      api.registerProjectProvider('second', second);
      await manager.start();
      await manager.ready();
      assert.deepStrictEqual(api.projects, [release, debug]);
    });

    test('unregistering the last provider restores the workspace folders',
         async () => {
           const provider = new FakeProvider();
           provider.projects = [release];
           const registration = api.registerProjectProvider('test', provider);
           assert.throws(() => api.registerProjectProvider('test', provider));
           await manager.start();
           await manager.ready();
           assert.deepStrictEqual(api.projects, [release]);
           registration.dispose();
           await manager.ready();
           assert.deepStrictEqual(api.projects.map(p => p.uri.path), ['/w']);
         });

    test('a provider whose subscription throws is not registered', () => {
      const provider = new FakeProvider();
      Object.defineProperty(provider, 'onDidChangeProjects',
                            {value: () => { throw new Error('broken'); }});
      assert.throws(() => api.registerProjectProvider('test', provider),
                    /broken/);
      api.registerProjectProvider('test', new FakeProvider()).dispose();
    });

    test('a provider failing once keeps its clients running', async () => {
      harness.visibleEditors = [editor(releaseFile.path)];
      const provider = new FakeProvider();
      provider.projects = [release];
      api.registerProjectProvider('test', provider);
      await manager.start();
      await manager.ready();
      const context = harness.contexts[0];
      assert.ok(context?.clientIsRunning());
      provider.provideProjects = () => { throw new Error('reconfiguring'); };
      provider.change();
      await manager.ready();
      await settle();
      assert.ok(!context.disposed);
      assert.deepStrictEqual(api.projects, [release]);
    });

    test('a hanging provider blocks neither restart nor shutdown', async () => {
      harness.visibleEditors = [editor(releaseFile.path)];
      api.registerProjectProvider(
          'hanging',
          {provideProjects: () => new Promise<ClangdProject[]>(() => {})});
      const provider = new FakeProvider();
      provider.projects = [release];
      api.registerProjectProvider('test', provider);
      await manager.start();
      assert.ok(!manager.isStarting());
      await manager.restart();
      assert.strictEqual(harness.activate.callCount, 2);
      await manager.stop();
      assert.ok(!manager.isStarting());
      assert.ok(!manager.isRunning());
    });

    test('a global restart reports no project changes', async () => {
      const provider = new FakeProvider();
      provider.projects = [release, debug];
      api.registerProjectProvider('test', provider);
      await manager.start();
      await manager.ready();
      const events: ClangdProjectsChangeEvent[] = [];
      api.onDidChangeProjects(e => events.push(e));
      await manager.restart();
      await manager.ready();
      assert.deepStrictEqual(api.projects, [release, debug]);
      assert.deepStrictEqual(events, []);
    });

    test('the active project follows the focused clangd editor', async () => {
      const provider = new FakeProvider();
      provider.projects = [release, debug];
      api.registerProjectProvider('test', provider);
      const releaseEditor = editor(releaseFile.path);
      const debugEditor = editor(debugFile.path);
      harness.visibleEditors = [releaseEditor, debugEditor];
      harness.activeEditor = releaseEditor;
      const active: (ClangdProject|undefined)[] = [];
      api.onDidChangeActiveProject(p => active.push(p));
      await manager.start();
      await manager.ready();
      await settle();
      assert.strictEqual(api.activeProject, release);

      harness.setActiveEditor(debugEditor);
      assert.strictEqual(api.activeProject, debug);
      assert.strictEqual(api.getLanguageClient(),
                         api.getLanguageClient(debugFile));
      // Focusing a file outside all projects keeps the active project.
      harness.setActiveEditor(editor('/w/other/c.cpp'));
      assert.strictEqual(api.activeProject, debug);
      assert.deepStrictEqual(active, [release, debug]);
    });
  });
});
