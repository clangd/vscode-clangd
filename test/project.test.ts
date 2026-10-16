import * as assert from 'assert';
import * as vscode from 'vscode';

import * as project from '../src/project';

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

  test('diffProjects reports renamed projects as updated', () => {
    const diff = project.diffProjects([{uri: file('/w/a'), name: 'x'}],
                                      [{uri: file('/w/a'), name: 'y'}]);
    assert.strictEqual(diff.updated.length, 1);
    assert.strictEqual(diff.added.length + diff.removed.length, 0);
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
});
