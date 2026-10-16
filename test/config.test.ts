import * as assert from 'assert';
import {homedir} from 'os';
import * as sinon from 'sinon';
import * as vscode from 'vscode';

import * as config from '../src/config';

suite('config', () => {
  const folder: vscode.WorkspaceFolder = {
    uri: vscode.Uri.file('/path/to/project'),
    name: 'Project Name',
    index: 0
  };

  let sandbox: sinon.SinonSandbox;
  let getConfiguration: sinon.SinonStub;
  let value: unknown;

  setup(() => {
    sandbox = sinon.createSandbox();
    getConfiguration =
        sandbox.stub(vscode.workspace, 'getConfiguration').returns({
          get: () => value
        } as unknown as vscode.WorkspaceConfiguration);
  });

  teardown(() => { sandbox.restore(); });

  test('substitutes workspace folder variables with a folder', async () => {
    value = [
      '${workspaceFolder}/build', '--name=${workspaceFolderBasename}',
      '${workspaceRoot}', '${cwd}'
    ];
    assert.deepStrictEqual(await config.get('arguments', folder), [
      `${folder.uri.fsPath}/build`, '--name=project', folder.uri.fsPath,
      folder.uri.fsPath
    ]);
    assert.ok(getConfiguration.calledOnceWithExactly('clangd', folder));
  });

  test('substitutes nested values with a folder', async () => {
    value = {a: ['${userHome}'], b: '${workspaceFolderBasename}'};
    assert.deepStrictEqual(await config.get('x', folder),
                           {a: [homedir()], b: 'project'});
  });

  test('reads the value for no specific resource without a folder',
       async () => {
         value = ['--log=verbose', '${userHome}'];
         assert.deepStrictEqual(await config.get('arguments'),
                                ['--log=verbose', homedir()]);
         assert.ok(getConfiguration.calledOnceWithExactly('clangd', null));
       });
});
