import * as assert from 'assert';
import * as vscode from 'vscode';

import {ClangdContextManager} from '../src/clangd-context-manager';
import {ClangdProject} from '../src/project';

import {
  cppEditor as editor,
  FakeContext,
  ManagerHarness,
  settle
} from './mocks';

suite('ClangdContextManager', () => {
  const file = (p: string) => vscode.Uri.file(p);

  let harness: ManagerHarness;
  let manager: ClangdContextManager;
  let contexts: FakeContext[];

  setup(() => {
    harness = new ManagerHarness();
    harness.setFolders(['/w/a', '/w/b']);
    manager = harness.manager;
    contexts = harness.contexts;
  });

  teardown(() => { harness.dispose(); });

  test('default mode serves every file with one context', async () => {
    harness.multiProject = false;
    harness.visibleEditors = [editor('/w/a/x.cpp')];
    await manager.start();
    assert.strictEqual(harness.create.callCount, 1);
    assert.strictEqual(harness.create.firstCall.args[3], undefined);
    const context = manager.contextFor(file('/w/a/x.cpp'));
    assert.ok(context);
    assert.strictEqual(manager.contextFor(file('/elsewhere/y.cpp')), context);
    assert.strictEqual(manager.contextFor(undefined), context);
    assert.deepStrictEqual(manager.projects, []);
  });

  test('multi mode starts a context per visible project', async () => {
    harness.visibleEditors = [editor('/w/a/x.cpp'), editor('/w/b/y.cpp')];
    await manager.start();
    assert.strictEqual(harness.activate.callCount, 1);
    assert.strictEqual(harness.create.callCount, 2);
    assert.strictEqual(manager.contextFor(file('/w/a/z.cpp'))?.project?.name,
                       'a');
    assert.strictEqual(manager.contextFor(file('/w/b/z.cpp'))?.project?.name,
                       'b');
    assert.strictEqual(manager.contextFor(file('/w/c/z.cpp')), undefined);
    assert.deepStrictEqual(harness.channels.map(c => c.name),
                           ['clangd (a)', 'clangd (b)']);
  });

  test('a client that fails to start does not affect other projects',
       async () => {
         harness.create.callsFake(async (_path: string, _channel: unknown,
                                         _manager: unknown,
                                         project: ClangdProject) => {
           const context = harness.newContext(project);
           if (project.name === 'a')
             context.startClient = () => Promise.reject(new Error('failed'));
           return context;
         });
         harness.visibleEditors = [editor('/w/a/x.cpp'), editor('/w/b/y.cpp')];
         await manager.start();
         assert.strictEqual(await manager.ensureContext(file('/w/a/x.cpp')),
                            contexts[0]);
         assert.ok(contexts[1].started);
       });

  test('projects with the same name log to channels named after their roots',
       async () => {
         harness.setFolders(['/r1/app', '/r2/app']);
         harness.visibleEditors =
             [editor('/r1/app/x.cpp'), editor('/r2/app/y.cpp')];
         await manager.start();
         assert.deepStrictEqual(harness.channels.map(c => c.name), [
           `clangd (${file('/r1/app').fsPath})`,
           `clangd (${file('/r2/app').fsPath})`
         ]);
       });

  test('a project starts when one of its files becomes visible', async () => {
    harness.visibleEditors = [editor('/w/a/x.cpp')];
    await manager.start();
    assert.strictEqual(harness.create.callCount, 1);
    assert.strictEqual(manager.contextFor(file('/w/b/y.cpp')), undefined);
    harness.setVisibleEditors([editor('/w/b/y.cpp')]);
    await settle();
    assert.strictEqual(harness.create.callCount, 2);
    assert.ok(manager.contextFor(file('/w/b/y.cpp')));
  });

  test('concurrent ensureContext calls create one context', async () => {
    await manager.start();
    const [first, second] = await Promise.all([
      manager.ensureContext(file('/w/a/x.cpp')),
      manager.ensureContext(file('/w/a/y.cpp'))
    ]);
    assert.strictEqual(harness.create.callCount, 1);
    assert.ok(first);
    assert.strictEqual(first, second);
  });

  test('a context created after stop() is disposed', async () => {
    await manager.start();
    const unblock = harness.blockCreation();
    const ensured = manager.ensureContext(file('/w/a/x.cpp'));
    await manager.stop();
    unblock();
    assert.strictEqual(await ensured, undefined);
    assert.strictEqual(contexts.length, 1);
    assert.ok(contexts[0].disposed);
    assert.ok(!contexts[0].started);
    assert.deepStrictEqual(manager.contexts, []);
  });

  test('ensureContext resolves to the context of a restarted creation',
       async () => {
         await manager.start();
         const unblock = harness.blockCreation();
         const ensured = manager.ensureContext(file('/w/a/x.cpp'));
         const restarted = manager.restart(file('/w/a/x.cpp'));
         unblock();
         await restarted;
         const context = await ensured;
         assert.ok(context);
         assert.strictEqual(context, manager.contextFor(file('/w/a/x.cpp')));
         assert.strictEqual(contexts.length, 2);
         assert.ok(contexts[0].disposed);
       });

  test('a starting client in the default mode is not stopped', async () => {
    harness.multiProject = false;
    await manager.start();
    contexts[0].starting = true;
    await manager.stop();
    await manager.restart();
    assert.ok(!contexts[0].disposed);
    assert.strictEqual(harness.create.callCount, 1);
  });

  test('starting clients do not block restart and shutdown', async () => {
    harness.visibleEditors = [editor('/w/a/x.cpp'), editor('/w/b/y.cpp')];
    await manager.start();
    contexts[0].starting = true;
    await manager.restart(file('/w/a/x.cpp'));
    assert.ok(contexts[0].disposed);
    contexts[2].starting = true;
    await manager.restart();
    assert.ok(contexts[1].disposed && contexts[2].disposed);
    assert.strictEqual(manager.contexts.length, 2);
    await manager.stop();
    assert.deepStrictEqual(manager.contexts, []);
  });

  test('a restart while the session starts is dropped', async () => {
    const unblock = harness.blockActivation();
    const started = manager.start();
    const restarted = manager.restart();
    unblock();
    await started;
    await restarted;
    assert.strictEqual(harness.activate.callCount, 1);
  });

  test('a shutdown while the session starts takes effect', async () => {
    const unblock = harness.blockActivation();
    harness.visibleEditors = [editor('/w/a/x.cpp')];
    const started = manager.start();
    await manager.stop();
    unblock();
    await started;
    assert.deepStrictEqual(manager.contexts, []);
    assert.ok(!manager.isStarting());
    await manager.restart();
    assert.strictEqual(manager.contexts.length, 1);
  });

  test('a restart after a shutdown while the session starts takes effect',
       async () => {
         const unblock = harness.blockActivation();
         harness.visibleEditors = [editor('/w/a/x.cpp')];
         const started = manager.start();
         await manager.stop();
         const restarted = manager.restart();
         unblock();
         await started;
         await restarted;
         assert.strictEqual(harness.activate.callCount, 2);
         assert.strictEqual(manager.contexts.length, 1);
       });

  test('a mode change while the session starts takes effect', async () => {
    const unblock = harness.blockActivation();
    harness.visibleEditors = [editor('/w/a/x.cpp')];
    const started = manager.start();
    harness.multiProject = false;
    const restarted = manager.restart();
    unblock();
    await started;
    await restarted;
    assert.strictEqual(harness.activate.callCount, 2);
    assert.strictEqual(manager.multiProject, false);
    assert.strictEqual(manager.contextFor(file('/w/a/x.cpp'))?.project,
                       undefined);
  });

  test('overlapping restarts run one after the other', async () => {
    harness.visibleEditors = [editor('/w/a/x.cpp')];
    await manager.start();
    let activating = 0;
    let overlapped = false;
    harness.activate.callsFake(async () => {
      overlapped ||= activating > 0;
      activating++;
      await settle();
      activating--;
      return '/usr/bin/clangd';
    });
    harness.activate.resetHistory();
    // A mode change makes the restarts wait before they set up a session.
    harness.multiProject = false;
    await Promise.all([manager.restart(), manager.restart()]);
    assert.ok(!overlapped);
    assert.strictEqual(harness.activate.callCount, 1);
  });

  test('restart with a uri restarts only that project', async () => {
    harness.visibleEditors = [editor('/w/a/x.cpp'), editor('/w/b/y.cpp')];
    await manager.start();
    const [a, b] = contexts;
    await manager.restart(file('/w/a/x.cpp'));
    assert.ok(a.disposed);
    assert.ok(!b.disposed);
    assert.strictEqual(harness.create.callCount, 3);
    assert.strictEqual(manager.contextFor(file('/w/a/x.cpp')), contexts[2]);
    assert.strictEqual(harness.activate.callCount, 1);
  });

  test('restarts recreate contexts without a visible file', async () => {
    await manager.start();
    await manager.ensureContext(file('/w/a/x.cpp'));
    await manager.restart(file('/w/a/x.cpp'));
    assert.strictEqual(contexts.length, 2);
    assert.strictEqual(manager.contextFor(file('/w/a/x.cpp')), contexts[1]);
    await manager.restart();
    assert.strictEqual(contexts.length, 3);
    assert.strictEqual(manager.contextFor(file('/w/a/x.cpp')), contexts[2]);
    assert.strictEqual(manager.contextFor(file('/w/b/x.cpp')), undefined);
  });

  test('stop with a uri stays in effect until restart', async () => {
    harness.visibleEditors = [editor('/w/a/x.cpp')];
    await manager.start();
    await manager.stop(file('/w/a/x.cpp'));
    assert.ok(contexts[0].disposed);
    assert.strictEqual(await manager.ensureContext(file('/w/a/x.cpp')),
                       undefined);
    harness.setVisibleEditors([editor('/w/a/y.cpp')]);
    await settle();
    assert.strictEqual(harness.create.callCount, 1);

    await manager.restart(file('/w/a/x.cpp'));
    assert.strictEqual(harness.create.callCount, 2);
    assert.ok(manager.contextFor(file('/w/a/x.cpp')));
  });

  test('restart without a uri resumes stopped projects', async () => {
    harness.visibleEditors = [editor('/w/a/x.cpp')];
    await manager.start();
    await manager.stop(file('/w/a/x.cpp'));
    await manager.restart();
    assert.strictEqual(harness.create.callCount, 2);
    assert.ok(manager.contextFor(file('/w/a/x.cpp')));
  });

  test('removing a folder disposes its context and channel', async () => {
    harness.visibleEditors = [editor('/w/a/x.cpp'), editor('/w/b/y.cpp')];
    await manager.start();
    harness.setFolders(['/w/b']);
    await settle();
    assert.ok(contexts[0].disposed);
    assert.ok(!contexts[1].disposed);
    assert.strictEqual(manager.contextFor(file('/w/a/x.cpp')), undefined);
    assert.ok(harness.channels[0].dispose.calledOnce);
    assert.ok(harness.channels[1].dispose.notCalled);
  });

  test('a stopping client does not write to its disposed channel', async () => {
    harness.visibleEditors = [editor('/w/a/x.cpp')];
    await manager.start();
    const channel: vscode.LogOutputChannel = harness.create.firstCall.args[1];
    channel.info('started');
    assert.ok(harness.channels[0].info.calledOnceWith('started'));
    harness.setFolders(['/w/b']);
    await settle();
    assert.ok(harness.channels[0].dispose.calledOnce);
    channel.info('Server process exited successfully');
    assert.ok(harness.channels[0].info.calledOnce);
  });

  test('an added folder starts for an already visible editor', async () => {
    harness.visibleEditors = [editor('/w/c/x.cpp')];
    await manager.start();
    assert.strictEqual(harness.create.callCount, 0);
    harness.setFolders(['/w/a', '/w/b', '/w/c']);
    await settle();
    assert.strictEqual(harness.create.callCount, 1);
    assert.strictEqual(manager.contextFor(file('/w/c/x.cpp'))?.project?.name,
                       'c');
  });

  test('a renamed folder restarts its client', async () => {
    await manager.start();
    await manager.ensureContext(file('/w/a/x.cpp'));
    await manager.ensureContext(file('/w/b/y.cpp'));
    harness.setFolders(['/w/a', '/w/b'], ['renamed', 'b']);
    await settle();
    assert.ok(contexts[0].disposed);
    assert.ok(!contexts[1].disposed);
    assert.strictEqual(manager.contextFor(file('/w/a/x.cpp'))?.project?.name,
                       'renamed');
    assert.ok(harness.channels[0].dispose.calledOnce);
    assert.strictEqual(harness.channels[2].name, 'clangd (renamed)');
  });

  test('nested folders are served by the outermost one', async () => {
    harness.setFolders(['/w', '/w/a']);
    harness.visibleEditors = [editor('/w/a/x.cpp')];
    await manager.start();
    assert.deepStrictEqual(manager.projects.map(p => p.uri.path), ['/w']);
    assert.strictEqual(manager.contextFor(file('/w/a/x.cpp'))?.project?.name,
                       'w');
  });
});
