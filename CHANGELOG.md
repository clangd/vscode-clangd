# Change Log

## Version 0.1.35: May 23, 2025

* On config changed watcher can be force enabled [#300](https://github.com/clangd/vscode-clangd/issues/300).
* Extension is only enabled in trusted workspaces [#812](https://github.com/clangd/vscode-clangd/pull/812).

## Version 0.1.34: March 15, 2025

* Create pre-release flows [#748](https://github.com/clangd/vscode-clangd/issues/748) [#753](https://github.com/clangd/vscode-clangd/issues/753) [#765](https://github.com/clangd/vscode-clangd/issues/765)
* Clean up disposables in early-return path of ClangdContext.create() [#743](https://github.com/clangd/vscode-clangd/issues/743)
* Publish API package to the default registry [#746](https://github.com/clangd/vscode-clangd/issues/746)
* Documentation
  * Mention the clangd config file in the README [#752](https://github.com/clangd/vscode-clangd/issues/752)
  * Mention the editor.defaultFormatter setting [#768](https://github.com/clangd/vscode-clangd/issues/768)
* Add an option to allow "clangd.path" to point to a shell script [#683](https://github.com/clangd/vscode-clangd/issues/683)
* Automatically restart clangd language server after it is installed [#749](https://github.com/clangd/vscode-clangd/issues/749)
* Handle workspace symbols searches with a '::' prefix [#769](https://github.com/clangd/vscode-clangd/issues/769)

## Version 0.1.33: November 21, 2024

* Reverted [#730](https://github.com/clangd/vscode-clangd/pull/730) for causing [#734](https://github.com/clangd/vscode-clangd/issues/734)


## Version 0.1.32: November 21, 2024

* Allow "clangd.path" to point to a shell script (restoring behavior from VS Code 1.91), behind option "clangd.useScriptAsExecutable" [#730](https://github.com/clangd/vscode-clangd/pull/730)
* Handle language client being null if clangd is disabled or failed to initialize in the API [#728](https://github.com/clangd/vscode-clangd/pull/728), [#731](https://github.com/clangd/vscode-clangd/pull/731)


## Version 0.1.31: November 13, 2024

* Reverted [#708](https://github.com/clangd/vscode-clangd/pull/708) and [#715](https://github.com/clangd/vscode-clangd/pull/715) for causing [#722](https://github.com/clangd/vscode-clangd/issues/722)


## Version 0.1.30: November 13, 2024

* Added option to disable hovers [#703](https://github.com/clangd/vscode-clangd/pull/703)
* Added option to disable clangd [#636](https://github.com/clangd/vscode-clangd/pull/636)
* Key bindings are restricted to the supported files [#690](https://github.com/clangd/vscode-clangd/pull/690)
* Allow "clangd.path" to point to a shell script (restoring behavior from VS Code 1.91) [#708](https://github.com/clangd/vscode-clangd/pull/708), [#715](https://github.com/clangd/vscode-clangd/pull/715)

## Version 0.1.29: July 12, 2024

* vscode-clangd now exposes an [API](https://github.com/clangd/vscode-clangd/blob/master/api/README.md)
  to other VSCode extensions, allowing them to make requests of their own to the clangd server [#575](https://github.com/clangd/vscode-clangd/pull/575)
* The predefined variable `${workspaceFolderBasename}` is now recognized in settings values
  such as `"clangd.arguments"` [#147](https://github.com/clangd/vscode-clangd/issues/147)
* Bug fixes to inactive region highlighting
  * Decorations are now cleared on clangd restart [#600](https://github.com/clangd/vscode-clangd/issues/600)
  * Decorations are now updated when their settings are changed [#613](https://github.com/clangd/vscode-clangd/pull/613)

## Version 0.1.28: March 20, 2024

* Fix a regression in the behaviour of `clangd.restart` introduced in 0.1.27 [#599](https://github.com/clangd/vscode-clangd/issues/599)

## Version 0.1.27: March 16, 2024

* Trigger signature help when accepting code completions, where appropriate [#390](https://github.com/clangd/vscode-clangd/issues/390)
* Gracefully handle `clangd.restart` being invoked when the extension hasn't been activated yet [#502](https://github.com/clangd/vscode-clangd/issues/502)
* Add an option to disable code completion [#588](https://github.com/clangd/vscode-clangd/issues/588)

## Version 0.1.26: December 20, 2023

* Bump @clangd/install dependency to 0.1.17. This works around a bug in a
  dependent library affecting node versions 18.16 and later that can cause
  the downloaded clangd executable to be corrupt after unzipping.

## Version 0.1.25: August 15, 2023

* Combine inactive region style with client-side (textmate) token colors [#193](https://github.com/clangd/vscode-clangd/pull/193).
  Requires clangd 17 or later.
  * The default inactive region style is reduced opacity. The opacity level can be
    customized with `clangd.inactiveRegions.opacity`.
  * An alternative inactive region style of a background highlight can be enabled with
    `clangd.inactiveRegions.useBackgroundHighlight=true`. The highlight color can be
    customized with `clangd.inactiveRegions.background` in `workbench.colorCustomizations`.
* The variable substitution `${userHome}` is now supported in clangd configuration setting values [#486](https://github.com/clangd/vscode-clangd/pull/486)

## Version 0.1.24: April 21, 2023

- Fix an undefined object access in ClangdContext.dispose() [#461](https://github.com/clangd/vscode-clangd/pull/461)
- Remove custom secure configuration items and rely on workspace trust from vscode [#451](https://github.com/clangd/vscode-clangd/pull/451)

## Version 0.1.23: October 23, 2022

* Update vscode-language client to 8.0.2, with apparent bugfixes
* clangd autoupdate: correctly detect versions of distro-modified clangd
* clangd autoupdate: improved error messages
* fix clangd not detected due to permission issues [#267](https://github.com/clangd/vscode-clangd/issues/267)

## Version 0.1.22: October 12, 2022

* Type-hierarchy: Prefer standard/extension version of the feature depending on
  the clangd, rather than showing both.

## Version 0.1.21: July 15, 2022

* Fix: clangd status bar is missing, [#362](https://github.com/clangd/vscode-clangd/issues/362)

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
