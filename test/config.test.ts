import * as assert from 'assert';
import * as sinon from 'sinon';
import * as vscode from 'vscode';

import * as config from '../src/config';

suite('Config', () => {
  let sandbox: sinon.SinonSandbox;

  setup(() => { sandbox = sinon.createSandbox(); });

  teardown(() => { sandbox.restore(); });

  test('substitutes named workspace folders', async () => {
    const folder: vscode.WorkspaceFolder = {
      uri: vscode.Uri.file('/projects/application'),
      name: 'Appl',
      index: 0,
    };
    const fallbackFlags = [
      '-I${workspaceFolder:Appl}/include',
      '-I${workspaceFolder:Missing}/include',
    ];
    sandbox.stub(vscode.workspace, 'workspaceFolders').value([folder]);
    sandbox.stub(vscode.workspace, 'getConfiguration').returns({
      get: () => fallbackFlags,
    } as unknown as vscode.WorkspaceConfiguration);

    assert.deepStrictEqual(await config.get<string[]>('fallbackFlags'), [
      `-I${folder.uri.fsPath}/include`,
      '-I${workspaceFolder:Missing}/include',
    ]);
  });
});
