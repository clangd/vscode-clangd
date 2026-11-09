import * as assert from 'assert';
import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {projectDocumentSelector} from '../src/clangd-context';
import {normalizeProjects, uriContains} from '../src/project';

import * as mocks from './mocks';

suite('Project routing', () => {
  const client = new vscodelc.LanguageClient('', {command: ''}, {});
  const file = (p: string) => vscode.Uri.file(p);

  // The indices of the roots whose selectors, as converted by the language
  // client, match `document`.
  function matches(roots: vscode.Uri[],
                   document: vscode.TextDocument): number[] {
    const result: number[] = [];
    roots.forEach((root, i) => {
      const selector = client.protocol2CodeConverter.asDocumentSelector(
          projectDocumentSelector(root));
      if (vscode.languages.match(selector, document) > 0)
        result.push(i);
    });
    return result;
  }

  const cpp = (p: string) => new mocks.MockTextDocument(file(p), 'cpp');

  test('sibling roots match disjoint files', () => {
    const roots = [file('/w/release_project'), file('/w/debug_project')];
    assert.deepStrictEqual(matches(roots, cpp('/w/release_project/a.cpp')),
                           [0]);
    assert.deepStrictEqual(matches(roots, cpp('/w/debug_project/x/b.cpp')),
                           [1]);
    assert.deepStrictEqual(matches(roots, cpp('/w/other/c.cpp')), []);
    assert.deepStrictEqual(matches(roots, cpp('/w/release_project2/d.cpp')),
                           []);
  });

  test('only the outermost of nested roots is served', () => {
    const {kept} = normalizeProjects(
        [{uri: file('/w/outer/inner')}, {uri: file('/w/outer')}]);
    const roots = kept.map(p => p.uri);
    assert.deepStrictEqual(roots.map(r => r.path), ['/w/outer']);
    assert.deepStrictEqual(matches(roots, cpp('/w/outer/inner/a.cpp')), [0]);
    assert.deepStrictEqual(matches(roots, cpp('/w/outer/b.cpp')), [0]);
  });

  test('glob metacharacters in roots are matched literally', () => {
    const roots = [file('/w/[x]{y},z'), file('/w/x')];
    assert.deepStrictEqual(matches(roots, cpp('/w/[x]{y},z/a.cpp')), [0]);
    assert.deepStrictEqual(matches(roots, cpp('/w/x/a.cpp')), [1]);
    assert.deepStrictEqual(matches(roots, cpp('/w/y/a.cpp')), []);
  });

  test('only clangd documents are matched', () => {
    const roots = [file('/w/p')];
    assert.deepStrictEqual(
        matches(roots, new mocks.MockTextDocument(file('/w/p/a.txt'))), []);
    assert.deepStrictEqual(
        matches(roots, new mocks.MockTextDocument(
                           vscode.Uri.parse('untitled:/w/p/a.cpp'), 'cpp')),
        []);
  });

  test('selectors and routing agree on case', () => {
    const root = file('/w/Proj');
    for (const path of ['/w/Proj/a.cpp', '/w/proj/a.cpp', '/W/PROJ/b.cpp']) {
      const document = cpp(path);
      assert.strictEqual(matches([root], document).length > 0,
                         uriContains(root, document.uri), path);
    }
  });
});
