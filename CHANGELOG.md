# Change Log

## Version 0.1.20: July 12, 2022

* Update to vscode-languageclient 8. this enables standard LSP inlay hints with a recent clangd, and may carry other behavior changes.
* Fix inlay-hints not shown when using a standard-inlay-hint-supported clangd. [#342](https://github.com/clangd/vscode-clangd/issues/342)
* "toggle inlay hints" command now respects the 4 states on/off/onUnlessPressed/offUnlessPressed
* Commit characters in code completion are explicitly disable (previously ignored due to a bug, see [#357]https://github.com/clangd/vscode-clangd/issues/357)).

## Version 0.1.19: July 11, 2022

* This is a rollback to address regressions in code completion introduced by 0.1.18.
* Unfortunately this breaks inlay hints again by reverting #342.

## Version 0.1.18: July 11, 2022

* Fix inlay-hints not shown when using a standard-inlay-hint-supported clangd. [#342](https://github.com/clangd/vscode-clangd/issues/342)

## Version 0.1.17: May 4, 2022

* Fix errors in "download language server" command introduced in 0.1.16. [#325](https://github.com/clangd/vscode-clangd/issues/325)

## Version 0.1.16: April 25, 2022

* Fix: "command clangd.inlayHints.toggle already exists" error on restarting clangd, [#302](https://github.com/clangd/vscode-clangd/pull/302)
* Inlay hints: switch to use VSCode's native implementation, [#301](https://github.com/clangd/vscode-clangd/pull/301)
* Bundle extension code into a single js file, reducing vsix file size, [#287](https://github.com/clangd/vscode-clangd/pull/287)

## Version 0.1.15: January 27, 2022

* Fix: clangd extension fails to restart, [#291](https://github.com/clangd/vscode-clangd/pull/291)
* Legacy semantic-highlighting is no longer supported, [#273](https://github.com/clangd/vscode-clangd/pull/273)

## Version 0.1.14: January 25, 2022

* Inlay hints: fully support clangd's
  [documented protocol extension](https://clangd.llvm.org/extensions#inlay-hints):
  arbitrary types and the `position` property.
* Inlay hints: cache hints more and refresh them less often.
* Inlay hints: respect editor.inlayHints.enabled, and provide a toggle command.
* CUDA: use VSCode's built-in language detection
* Switch header/source: constrain to applicable languages, add to context menu
* Releases now pushed to OpenVSX as well as VSCode Marketplace

## Version 0.1.13: August 24, 2021

* Add option to suppress warning about the Microsoft C/C++ extension
  incompatibility [#221](https://github.com/clangd/vscode-clangd/pull/221)

## Version 0.1.12: July 19, 2021

* Clicking on status bar opens output panel,
  [#177](https://github.com/clangd/vscode-clangd/pull/177)
* Commands to open user and project clangd confguration files
  [#181](https://github.com/clangd/vscode-clangd/pull/181)
* Inlay hints support for parameters
  [#168](https://github.com/clangd/vscode-clangd/pull/168) and types
  [#188](https://github.com/clangd/vscode-clangd/pull/188). Requires a clangd
  built after
  [llvm/llvm-project@cea736e5b](https://github.com/llvm/llvm-project/commit/cea736e5b8a48065007a591d71699b53c04d95b3)
  and `-inlay-hints` flag.

## Version 0.1.11: March 2, 2021

* Prompt when workspace overrides clangd.path and clangd.arguments, [#160](https://github.com/clangd/vscode-clangd/pull/160)
* Fix semanticTokens flickering issue, [#150](https://github.com/clangd/vscode-clangd/pull/150)
* clangd.path takes vscode.workspace.rootPath into account if available. [#153](https://github.com/clangd/vscode-clangd/pull/153)

## Version 0.1.10: February 15, 2021

* Warn about conflict with ms-vscode.cpptools, [#141](https://github.com/clangd/vscode-clangd/pull/141)

## Version 0.1.9: January 12, 2021

* Add "show AST" feature (clangd 12 or later), [#104](https://github.com/clangd/vscode-clangd/pull/104)
* Add "restartAfterCrash" option, [#108](https://github.com/clangd/vscode-clangd/pull/108)
* Add "serverCompletionRanking" option, [#63](https://github.com/clangd/vscode-clangd/pull/63)
* Client side will not watch config files for clangd 12 or later, as clangd natively supports it [#128](https://github.com/clangd/vscode-clangd/pull/128)
* Fix: improve workspace-symbol ranking for unqualified name, [#81](https://github.com/clangd/vscode-clangd/issues/81)
* Fix: error on restarting clangd when using onConfigChanged, [#98](https://github.com/clangd/vscode-clangd/issues/98)
* Fix: Resrict "show memory usage" button on its own view, [#97](https://github.com/clangd/vscode-clangd/pull/97)

## Version 0.1.8: November 10, 2020

* file watcher support for `compile_commands.json`, [#33](https://github.com/clangd/vscode-clangd/pull/33)
* restart clangd will respect the latest VSCode config, [#78](https://github.com/clangd/vscode-clangd/pull/78)
* improvement for type-hierarchy, [#44](https://github.com/clangd/vscode-clangd/pull/44), [#68](https://github.com/clangd/vscode-clangd/pull/68)
* add tree view for clangd's `$/memoryUsage` feature (clangd 12 or later), [#86](https://github.com/clangd/vscode-clangd/pull/86)

## Version 0.1.7: September 7, 2020

* (experimental) Type hierarchy support, [#20](https://github.com/clangd/vscode-clangd/pull/20)
* Switch to official semanticToken API (required clangd 11), [#54](https://github.com/clangd/vscode-clangd/pull/54)
* Fix autoinstall writes local config in remote settings, [#56](https://github.com/clangd/vscode-clangd/pull/56), [#65](https://github.com/clangd/vscode-clangd/pull/65)
