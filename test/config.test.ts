import * as assert from 'assert';
import * as fs from 'fs';
import * as path from 'path';
import * as sinon from 'sinon';
import * as vscode from 'vscode';

import * as config from '../src/config';

suite('Config', () => {
  let sandbox: sinon.SinonSandbox;

  setup(() => { sandbox = sinon.createSandbox(); });
  teardown(() => { sandbox.restore(); });

  test('reads and substitutes settings for a workspace folder', async () => {
    const folder: vscode.WorkspaceFolder = {
      uri: vscode.Uri.file('/workspace/one'),
      name: 'one',
      index: 0,
    };
    const getConfiguration = sandbox.stub(vscode.workspace, 'getConfiguration');
    getConfiguration.callsFake((section?: string,
                                scope?: vscode.ConfigurationScope|null) => {
      assert.strictEqual(scope?.toString(), folder.uri.toString());
      return {
        get: (key: string) => {
          if (section === 'clangd' && key === 'path')
            return '${workspaceFolder}/bin/${workspaceFolderBasename}/clangd';
          if (section === undefined && key === 'tool.path')
            return '/toolchain';
          return undefined;
        },
      } as vscode.WorkspaceConfiguration;
    });

    assert.strictEqual(await config.get<string>('path', folder),
                       '/workspace/one/bin/one/clangd');
    sinon.assert.calledWith(getConfiguration, 'clangd',
                            sinon.match(folder.uri));
  });

  test('uses folder scope for config variable substitution', async () => {
    const folder: vscode.WorkspaceFolder = {
      uri: vscode.Uri.file('/workspace/two'),
      name: 'two',
      index: 1,
    };
    sandbox.stub(vscode.workspace, 'getConfiguration')
        .callsFake(
            (section?: string, scope?: vscode.ConfigurationScope|null) => {
              assert.strictEqual(scope?.toString(), folder.uri.toString());
              return {
                get: (key: string) => {
                  if (section === 'clangd' && key === 'arguments')
                    return ['--query-driver=${config:tool.driver}'];
                  if (section === undefined && key === 'tool.driver')
                    return '/workspace/two/bin/clang++';
                  return undefined;
                },
              } as vscode.WorkspaceConfiguration;
            });

    assert.deepStrictEqual(await config.get<string[]>('arguments', folder),
                           ['--query-driver=/workspace/two/bin/clang++']);
  });

  test('declares folder-specific settings with resource scope', () => {
    const packageJson = JSON.parse(
        fs.readFileSync(path.resolve(__dirname, '../../package.json'), 'utf8'));
    const properties = packageJson.contributes.configuration.properties;
    const folderSettings = [
      'clangd.arguments',
      'clangd.trace',
      'clangd.fallbackFlags',
      'clangd.serverCompletionRanking',
      'clangd.restartAfterCrash',
      'clangd.checkUpdates',
      'clangd.onConfigChanged',
      'clangd.onConfigChangedForceEnable',
      'clangd.inactiveRegions.useBackgroundHighlight',
      'clangd.inactiveRegions.opacity',
      'clangd.enableCodeCompletion',
      'clangd.enableHover',
    ];
    for (const setting of folderSettings)
      assert.strictEqual(properties[setting].scope, 'resource', setting);
  });
});
