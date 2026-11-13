# VS Code clangd Extension API

The VS Code clangd extension exposes an API that other extensions can consume:

```typescript
import * as vscode from 'vscode';
import type { ClangdExtension, ASTParams, ASTNode } from '@clangd/vscode-clangd';

const CLANGD_EXTENSION = 'llvm-vs-code-extensions.vscode-clangd';
const CLANGD_API_VERSION = 1;

const ASTRequestMethod = 'textDocument/ast';

const provideHover = async (document: vscode.TextDocument, position: vscode.Position, _token: vscode.CancellationToken): Promise<vscode.Hover | undefined> => {

    const clangdExtension = vscode.extensions.getExtension<ClangdExtension>(CLANGD_EXTENSION);

    if (clangdExtension) {
        const api = (await clangdExtension.activate()).getApi(CLANGD_API_VERSION);

        // Extension may be disabled or have failed to initialize
        if (!api.languageClient) {
          return undefined;
        }
 
        const textDocument = api.languageClient.code2ProtocolConverter.asTextDocumentIdentifier(document);
        const range = api.languageClient.code2ProtocolConverter.asRange(new vscode.Range(position, position));
        const params: ASTParams = { textDocument, range };
 
        const ast: ASTNode | undefined = await api.languageClient.sendRequest(ASTRequestMethod, params);

        if (!ast) {
            return undefined;
        }

        return {
            contents: [ast.kind]
        };
    }
};

vscode.languages.registerHoverProvider(['c', 'cpp'], { provideHover });
```

`languageClient` is the client at the time `getApi` is called. It is undefined
while clangd is stopped, e.g. after the `clangd.shutdown` command or while a
restart reinstalls clangd.

The type declarations refer to the `vscode` module, so `@types/vscode` is a peer
dependency: consumers already depend on it as VS Code extensions.

## API version 2

Version 2 adds access to the clients and projects of the multi-project mode
(`clangd.enablePerFolderServer`), where every project is served by its own clangd
instance. In the default mode, there is a single client and no projects.

- `getLanguageClient(uri)` returns the client serving `uri`, and without `uri`
  the client of the active editor if it shows a C/C++ file, otherwise the one
  of the clangd editor focused last. Unlike `languageClient` of version 1, it
  is not a snapshot taken by `getApi`.
- `getAllLanguageClients()` returns all clients, and `onDidChangeClients` fires
  when they change.
- `startLanguageClient(uri)` starts the client serving `uri` if needed. Clients
  start lazily, when one of their files is shown in an editor, so requests that
  are not about a file, such as `workspace/symbol`, only reach the clients that
  have started.

Only clients that are starting or running are returned. Requests sent to a
starting client are delayed until it runs; clients that have stopped, e.g.
after crashing too often, are left out.

`projects`, `getProject(uri)`, `activeProject` and the corresponding events
describe the projects. They are kept while clangd restarts or is shut down.

By default, there is one project per workspace folder. Extensions that know
the layout of the code, e.g. build system integrations, can contribute projects
instead, for instance one per build directory below a workspace folder:

```typescript
const api = (await clangdExtension.activate()).getApi(2);

const changed = new vscode.EventEmitter<void>();
context.subscriptions.push(api.registerProjectProvider('my-build-system', {
  provideProjects: async () => [
    { uri: vscode.Uri.joinPath(folder.uri, 'release_project'), name: 'release' },
    { uri: vscode.Uri.joinPath(folder.uri, 'debug_project'), name: 'debug',
      arguments: ['--log=verbose'] },
  ],
  onDidChangeProjects: changed.event,
}));
```

While any provider is registered, the projects are the union of the results of
all providers, and the workspace folders are no longer projects. Projects that
are not on disk, duplicates and projects nested in another project are ignored:
of nested projects, only the outermost one is served. Files outside all
projects are not served. If a provider fails or doesn't respond within 10
seconds, its previous projects are kept; a late answer is still applied.
Providers take effect only in multi-project mode.

A provider that registers after clangd has started replaces the clients of the
workspace folders. To avoid starting those first, a provider extension should
activate on the same events as clangd, e.g. `onLanguage:cpp`. When a project's
name or arguments change, its clangd restarts.
