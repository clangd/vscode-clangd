# Development

A guide of developing `vscode-clangd` extension.

## Requirements

* VS Code
* node.js and npm

## Steps

1. Make sure you disable the installed `vscode-clangd` extension in VS Code.
2. Make sure you have clangd in `/usr/bin/clangd` or edit `src/extension.ts` to
point to the binary.
3. To start a development instance of VS code extended with this, run:

```bash
   $ cd vscode-clangd
   $ npm install
   $ npm run compile # it runs in watch mode, so you can hit ctrl^c.
   $ code . --extensionDevelopmentPath=$PWD
```

# Contributing

Please follow the existing code style when contributing to the extension, we
recommend to run `npm run format` before sending a patch.

# Publish to VS Code Marketplace

New changes to `vscode-clangd` are not released until a new version is published
to the marketplace.

## Requirements

* Make sure install the `vsce` command (`npm install -g vsce`)
* `llvm-vs-code-extensions` account
* Bump the version in `package.json`, and commit the change.

The extension is published under `llvm-vs-code-extensions` account, which is
maintained by clangd developers. If you want to make a new release, please
contact clangd-dev@lists.llvm.org.

## Steps

```bash
  $ cd vscode-clangd
  # For the first time, you need to login into the account. vsce will ask you
    for the Personal Access Token and will remember it for future commands.
  $ vsce login llvm-vs-code-extensions
  # Publish the extension to the VSCode marketplace.
  $ npm run publish
```
