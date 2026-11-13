import * as assert from 'assert';
import * as vscode from 'vscode';

import {ClangdProject, ClangdProjectsChangeEvent} from '../api/vscode-clangd';
import * as project from '../src/project';

import {settle} from './mocks';

const win32Test = process.platform === 'win32' ? test : test.skip;
const ignoresCase =
    process.platform === 'win32' || process.platform === 'darwin';

suite('Project', () => {
  const file = (p: string) => vscode.Uri.file(p);

  test('uriContains compares whole path segments', () => {
    assert.ok(project.uriContains(file('/a/p'), file('/a/p')));
    assert.ok(project.uriContains(file('/a/p'), file('/a/p/x.cpp')));
    assert.ok(project.uriContains(file('/a/p'), file('/a/p/x/y.cpp')));
    assert.ok(!project.uriContains(file('/a/p'), file('/a/p2')));
    assert.ok(!project.uriContains(file('/a/p'), file('/a/p2/x.cpp')));
    assert.ok(!project.uriContains(file('/a/p'), file('/a')));
    assert.ok(!project.uriContains(file('/a/p/x'), file('/a/p/y')));
  });

  test('uriContains handles ..foo segments and trailing slashes', () => {
    assert.ok(project.uriContains(file('/a/p'), file('/a/p/..foo/x.cpp')));
    assert.ok(!project.uriContains(file('/a/p'), file('/a/..foo/x.cpp')));
    assert.ok(
        project.uriContains(vscode.Uri.parse('file:///a/p/'), file('/a/p/x')));
    assert.ok(
        project.uriContains(file('/a/p'), vscode.Uri.parse('file:///a/p/')));
  });

  test('uriContains requires the same scheme and authority', () => {
    assert.ok(!project.uriContains(file('/a/p'),
                                   vscode.Uri.parse('untitled:/a/p/x.cpp')));
    assert.ok(!project.uriContains(vscode.Uri.parse('file://host1/a/p'),
                                   vscode.Uri.parse('file://host2/a/p/x.cpp')));
    assert.ok(project.uriContains(vscode.Uri.parse('file://host/a/p'),
                                  vscode.Uri.parse('file://host/a/p/x.cpp')));
  });

  win32Test('uriContains handles drive letters and case on win32', () => {
    assert.ok(project.uriContains(file('C:\\a\\p'), file('c:\\A\\P\\x.cpp')));
    assert.ok(!project.uriContains(file('C:\\a\\p'), file('D:\\a\\p\\x.cpp')));
    assert.ok(!project.uriContains(file('C:\\a\\p'), file('C:\\a\\p2')));
  });

  test('projectKey identifies a root', () => {
    assert.strictEqual(project.projectKey(file('/a/p')),
                       project.projectKey(vscode.Uri.parse('file:///a/p')));
    assert.notStrictEqual(project.projectKey(file('/a/p')),
                          project.projectKey(file('/a/p2')));
  });

  win32Test('projectKey ignores case on win32', () => {
    assert.strictEqual(project.projectKey(file('C:\\a\\P')),
                       project.projectKey(file('c:\\A\\p')));
  });

  test('projectKey and uriContains agree on case', () => {
    const lower = file('/w/proj');
    const mixed = file('/w/Proj');
    const sameKey = project.projectKey(lower) === project.projectKey(mixed);
    assert.strictEqual(sameKey, ignoresCase);
    assert.strictEqual(project.uriContains(lower, mixed), sameKey);
    assert.strictEqual(project.uriContains(mixed, file('/w/proj/a.cpp')),
                       sameKey);
  });

  test('normalizeProjects keeps the outermost project', () => {
    const outer = {uri: file('/w/outer')};
    const inner = {uri: file('/w/outer/inner')};
    const sibling = {uri: file('/w/outer2')};
    const {kept, dropped} = project.normalizeProjects([inner, sibling, outer]);
    assert.deepStrictEqual(kept, [sibling, outer]);
    assert.deepStrictEqual(dropped, [inner]);
  });

  test('normalizeProjects drops duplicates and non-file projects', () => {
    const first = {uri: file('/w/p'), name: 'first'};
    const second = {uri: file('/w/p'), name: 'second'};
    const remote = {uri: vscode.Uri.parse('vscode-remote://host/w/q')};
    const {kept, dropped} = project.normalizeProjects([first, second, remote]);
    assert.deepStrictEqual(kept, [first]);
    assert.deepStrictEqual(dropped, [second, remote]);
  });

  test('normalizeProjects keeps a project provided twice once', () => {
    const p = {uri: file('/w/p')};
    const {kept, dropped} = project.normalizeProjects([p, p]);
    assert.deepStrictEqual(kept, [p]);
    assert.deepStrictEqual(dropped, [p]);
  });

  test('diffProjects reports added, removed and updated projects', () => {
    const a = {uri: file('/w/a')};
    const b = {uri: file('/w/b'), name: 'x'};
    const c = {uri: file('/w/c'), name: 'c'};
    const b2 = {uri: file('/w/b'), name: 'y'};
    const c2 = {uri: file('/w/c'), name: 'c'};
    const d = {uri: file('/w/d')};
    const diff = project.diffProjects([a, b, c], [b2, c2, d]);
    assert.deepStrictEqual(diff.added, [d]);
    assert.deepStrictEqual(diff.removed, [a]);
    assert.deepStrictEqual(diff.updated, [b2]);
  });

  test('diffProjects reports projects with new arguments as updated', () => {
    const diff = project.diffProjects([{uri: file('/w/a'), arguments: ['-x']}],
                                      [{uri: file('/w/a'), arguments: ['-y']}]);
    assert.strictEqual(diff.updated.length, 1);
    assert.strictEqual(diff.added.length + diff.removed.length, 0);
  });

  test('diffProjects reports renamed projects as updated', () => {
    const diff = project.diffProjects([{uri: file('/w/a'), name: 'x'}],
                                      [{uri: file('/w/a'), name: 'y'}]);
    assert.strictEqual(diff.updated.length, 1);
    assert.strictEqual(diff.added.length + diff.removed.length, 0);
  });

  test('projectTraceFile adds the project name and a hash of its root', () => {
    const p = {uri: file('/w/a'), name: 'my app'};
    assert.match(project.projectTraceFile('/t/trace.json', p),
                 /^\/t\/trace\.my_app-[0-9a-f]{8}\.json$/);
    assert.match(project.projectTraceFile('/t.d/trace', p),
                 /^\/t\.d\/trace\.my_app-[0-9a-f]{8}$/);
    // Projects with the same name in different places.
    assert.notStrictEqual(
        project.projectTraceFile('/t/trace.json', {uri: file('/r1/src')}),
        project.projectTraceFile('/t/trace.json', {uri: file('/r2/src')}));
  });

  test('projectName defaults to the basename', () => {
    assert.strictEqual(project.projectName({uri: file('/w/a')}), 'a');
    assert.strictEqual(project.projectName({uri: file('/w/a'), name: 'n'}),
                       'n');
  });

  test('projectLabel tells apart projects with the same name', () => {
    const a = {uri: file('/r1/app')};
    const b = {uri: file('/r2/app')};
    const c = {uri: file('/r2/lib')};
    assert.strictEqual(project.projectLabel(a, [a, c]), 'app');
    assert.strictEqual(project.projectLabel(a, [a, b, c]),
                       file('/r1/app').fsPath);
    assert.strictEqual(project.projectLabel(c, [a, b, c]), 'lib');
  });

  suite('ProjectSet', () => {
    const log = {info() {}, error() {}} as unknown as vscode.LogOutputChannel;
    let projects: project.ProjectSet;

    setup(() => { projects = new project.ProjectSet(log, 20); });
    teardown(() => { projects.dispose(); });

    test('ignores providers that fail or do not respond', async () => {
      const p = {uri: file('/w/p')};
      projects.register('hanging',
                        {provideProjects: () => new Promise(() => {})});
      projects.register(
          'failing', {provideProjects: () => { throw new Error('failed'); }});
      projects.register('working', {provideProjects: async () => [p]});
      projects.activate();
      await projects.settled();
      assert.deepStrictEqual(projects.projects, [p]);
    });

    test('keeps the projects of a provider that fails or does not respond',
         async () => {
           const p = {uri: file('/w/p')};
           const q = {uri: file('/w/q')};
           let answer: () => unknown = () => [p, q];
           const changed = new vscode.EventEmitter<void>();
           projects.register('flaky', {
             provideProjects: () => answer() as ClangdProject[],
             onDidChangeProjects: changed.event
           });
           projects.activate();
           await projects.settled();
           const changes: ClangdProjectsChangeEvent[] = [];
           projects.onDidChange(diff => changes.push(diff));
           for (const failure of [() => new Promise(() => {}),
                                 () => { throw new Error('failed'); },
                                 () => Promise.reject(new Error('failed')),
                                 () => [p, {}],
                                 () => 'no projects',
           ]) {
             answer = failure;
             changed.fire();
             await projects.settled();
             assert.deepStrictEqual(projects.projects, [p, q]);
           }
           assert.deepStrictEqual(changes, []);
           answer = () => [q];
           changed.fire();
           await projects.settled();
           assert.deepStrictEqual(projects.projects, [q]);
         });

    test('applies an answer that arrives after the timeout', async () => {
      const p = {uri: file('/w/p')};
      let resolve!: (projects: ClangdProject[]) => void;
      projects.register('slow', {
        provideProjects: () => new Promise<ClangdProject[]>(r => resolve = r)
      });
      projects.activate();
      await projects.settled();
      assert.deepStrictEqual(projects.projects, []);
      resolve([p]);
      await settle();
      assert.deepStrictEqual(projects.projects, [p]);
    });

    test('drops a late answer once the provider was queried again',
         async () => {
           const p = {uri: file('/w/p')};
           const q = {uri: file('/w/q')};
           const resolvers: ((projects: ClangdProject[]) => void)[] = [];
           const changed = new vscode.EventEmitter<void>();
           projects.register('slow', {
             provideProjects: () =>
                 new Promise<ClangdProject[]>(r => resolvers.push(r)),
             onDidChangeProjects: changed.event
           });
           projects.activate();
           await projects.settled();
           changed.fire();
           resolvers[1]([q]);
           await projects.settled();
           resolvers[0]([p]);
           await settle();
           assert.deepStrictEqual(projects.projects, [q]);
         });

    test('cancels a superseded query', async () => {
      const tokens: vscode.CancellationToken[] = [];
      const changed = new vscode.EventEmitter<void>();
      projects.register('hanging', {
        provideProjects: (token) => {
          tokens.push(token);
          return new Promise(() => {});
        },
        onDidChangeProjects: changed.event
      });
      projects.activate();
      changed.fire();
      assert.strictEqual(tokens.length, 2);
      assert.ok(tokens[0].isCancellationRequested);
      assert.ok(!tokens[1].isCancellationRequested);
      projects.deactivate();
      assert.ok(tokens[1].isCancellationRequested);
      // The cancelled queries don't wait for the timeout.
      await projects.settled();
    });

    test('queries no provider until activated', async () => {
      let queries = 0;
      projects.register('test', {
        provideProjects: () => {
          queries++;
          return [];
        }
      });
      await projects.settled();
      assert.strictEqual(queries, 0);
      projects.activate();
      assert.strictEqual(queries, 1);
    });
  });
});
