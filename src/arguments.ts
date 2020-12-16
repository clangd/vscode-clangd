import * as config from './config';

function argsContainFlag(args: string[], flag: string) {
  for (let arg of args) {
    if (arg.startsWith(flag))
      return true;
  }

  return false;
}

export function getClangdArguments() {
  let args = config.get<string[]>('arguments');

  let compileCommandsDirFlag = '--compile-commands-dir';

  if (!argsContainFlag(args, compileCommandsDirFlag)) {
    let compileCommandsDir = config.get<string>('compileCommandsDir');
    if (compileCommandsDir.length > 0)
      args.push(`${compileCommandsDirFlag}=${compileCommandsDir}`);
  }

  let queryDriversFlag = '--query-driver';

  if (!argsContainFlag(args, queryDriversFlag)) {
    let queryDrivers = config.get<string[]>('queryDrivers');
    if (queryDrivers.length > 0) {
      let drivers = queryDrivers.join(',');
      args.push(`${queryDriversFlag}=${drivers}`);
    }
  }

  let backgroundIndexFlag = '--background-index';

  if (!argsContainFlag(args, backgroundIndexFlag)) {
    let backgroundIndex = config.get<boolean>('backgroundIndex');
    args.push(`${backgroundIndexFlag}=${backgroundIndex}`);
  }

  let clangTidyFlag = '--clang-tidy';

  if (!argsContainFlag(args, clangTidyFlag)) {
    let clangTidy = config.get<boolean>('clangTidy');
    args.push(`${clangTidyFlag}=${clangTidy}`)
  }

  let crossFileRenameFlag = '--cross-file-rename';

  if (!argsContainFlag(args, crossFileRenameFlag)) {
    let crossFileRename = config.get<boolean>('crossFileRename');
    args.push(`${crossFileRenameFlag}=${crossFileRename}`);
  }

  let headerInsertionFlag = '--header-insertion';

  if (!argsContainFlag(args, headerInsertionFlag)) {
    let headerInsertion = config.get<string>('headerInsertion');
    args.push(`${headerInsertionFlag}=${headerInsertion}`);
  }

  let limitResultsFlag = '--limit-results';

  if (!argsContainFlag(args, limitResultsFlag)) {
    let limitResults = config.get<number>('limitResults');
    args.push(`${limitResultsFlag}=${limitResults}`);
  }

  return args;
}