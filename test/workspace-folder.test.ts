import * as assert from 'assert';
import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';
import * as sinon from 'sinon';
import * as vscode from 'vscode';

import {ownsProtocolMessage} from '../src/clangd-context';
import {workspaceFolderForUri} from '../src/workspace-folder';

suite('Workspace folder ownership', () => {
  let root: string;
  let parent: vscode.WorkspaceFolder;
  let target: vscode.WorkspaceFolder;
  let aliasFile: vscode.Uri;
  let sandbox: sinon.SinonSandbox;

  setup(() => {
    sandbox = sinon.createSandbox();
    root = fs.mkdtempSync(path.join(os.tmpdir(), 'clangd-workspaces-'));
    const parentPath = path.join(root, 'ml-ext');
    const targetPath = path.join(root, 'hld');
    const aliasPath = path.join(parentPath, 'products', 'regtest', 'hld');
    fs.mkdirSync(path.dirname(aliasPath), {recursive: true});
    fs.mkdirSync(path.join(targetPath, 'dsdk'), {recursive: true});
    fs.symlinkSync(targetPath, aliasPath, 'dir');
    fs.writeFileSync(path.join(targetPath, 'dsdk', 'main.cpp'), '');

    parent = workspaceFolder(parentPath, 'ml-ext', 0);
    target = workspaceFolder(targetPath, 'hld', 1);
    aliasFile = vscode.Uri.file(path.join(aliasPath, 'dsdk', 'main.cpp'));
    sandbox.stub(vscode.workspace, 'workspaceFolders')
        .get(() => [parent, target]);
  });

  teardown(() => {
    sandbox.restore();
    fs.rmSync(root, {recursive: true, force: true});
  });

  test('assigns a symlinked file to its canonical workspace folder',
       () => { assert.strictEqual(workspaceFolderForUri(aliasFile), target); });

  test('resolves a symlinked ancestor when the file does not exist', () => {
    const missing = vscode.Uri.file(
        path.join(path.dirname(aliasFile.fsPath), 'missing.cpp'));
    assert.strictEqual(workspaceFolderForUri(missing), target);
  });

  test('routes protocol messages only to the canonical owner', () => {
    const params = {textDocument: {uri: aliasFile.toString()}};
    assert.strictEqual(ownsProtocolMessage(parent, params), false);
    assert.strictEqual(ownsProtocolMessage(target, params), true);
  });
});

function workspaceFolder(folderPath: string, name: string,
                         index: number): vscode.WorkspaceFolder {
  return {uri: vscode.Uri.file(folderPath), name, index};
}
