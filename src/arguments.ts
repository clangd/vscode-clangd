import * as cp from 'child_process';
import * as util from 'util';
import * as vscode from 'vscode';

import * as config from './config';

const exec = util.promisify(cp.exec);

function argsContainFlag(args: string[], flag: string) {
  for (let arg of args) {
    if (arg.startsWith(flag))
      return true;
  }

  return false;
}

async function getClangdMajorVersion(clangdPath: string) {
  const output = await exec(`${clangdPath} --version`);
  const regexv = /clangd version ([0-9]+)\..*/;
  const match = output.stdout.match(regexv);

  if (output.stderr || !match) {
    throw new Error('Could not determine clangd version')
  }

  return Number.parseInt(match[1]);
}

export async function getClangdArguments(clangdPath: string) {
  let args = config.get<string[]>('arguments');
  // Versions before clangd 7 are not checked
  let version = await getClangdMajorVersion(clangdPath);
  let overridenOptionWarning = false;

  let compileCommandsDirFlag = '--compile-commands-dir';

  if (!argsContainFlag(args, compileCommandsDirFlag)) {
    let compileCommandsDir = config.get<string>('compileCommandsDir');
    if (compileCommandsDir.length > 0)
      args.push(`${compileCommandsDirFlag}=${compileCommandsDir}`);
  } else {
    overridenOptionWarning = true;
  }

  if (version >= 9) {
    let queryDriversFlag = '--query-driver';

    if (!argsContainFlag(args, queryDriversFlag)) {
      let queryDrivers = config.get<string[]>('queryDrivers');
      if (queryDrivers.length > 0) {
        let drivers = queryDrivers.join(',');
        args.push(`${queryDriversFlag}=${drivers}`);
      }
    } else {
      overridenOptionWarning = true;
    }
  }

  if (version >= 8) {
    let backgroundIndexFlag = '--background-index';

    if (!argsContainFlag(args, backgroundIndexFlag)) {
      let backgroundIndex = config.get<boolean>('backgroundIndex');
      args.push(`${backgroundIndexFlag}=${backgroundIndex}`);
    } else {
      overridenOptionWarning = true;
    }
  }

  if (version >= 9) {
    let clangTidyFlag = '--clang-tidy';

    if (!argsContainFlag(args, clangTidyFlag)) {
      let clangTidy = config.get<boolean>('clangTidy');
      args.push(`${clangTidyFlag}=${clangTidy}`)
    } else {
      overridenOptionWarning = true;
    }
  }

  if (version >= 10) {
    let crossFileRenameFlag = '--cross-file-rename';

    if (!argsContainFlag(args, crossFileRenameFlag)) {
      let crossFileRename = config.get<boolean>('crossFileRename');
      args.push(`${crossFileRenameFlag}=${crossFileRename}`);
    } else {
      overridenOptionWarning = true;
    }
  }

  if (version >= 9) {
    let headerInsertionFlag = '--header-insertion';

    if (!argsContainFlag(args, headerInsertionFlag)) {
      let headerInsertion = config.get<string>('headerInsertion');
      args.push(`${headerInsertionFlag}=${headerInsertion}`);
    } else {
      overridenOptionWarning = true;
    }
  }

  let limitResultsFlag = '--limit-results';

  if (!argsContainFlag(args, limitResultsFlag)) {
    let limitResults = config.get<number>('limitResults');
    args.push(`${limitResultsFlag}=${limitResults}`);
  } else {
    overridenOptionWarning = true;
  }

  if (overridenOptionWarning) {
    let action = await vscode.window.showWarningMessage(
        'Setting "clangd.arguments" overrides one or more options that can now be set with more specific settings. This does not cause any error, but updating your configuration is advised.',
        'Open settings');

    if (action == 'Open settings') {
      vscode.commands.executeCommand(
          'workbench.action.openSettings',
          '@ext:llvm-vs-code-extensions.vscode-clangd')
    }
  }

  return args;
}