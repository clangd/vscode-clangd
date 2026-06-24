# VS Code clangd Extension API

The VS Code clangd extension exposes an API that other extensions can consume.

## API Versions

### API V2 (Multi-Root Support)

The V2 API provides full support for multi-root workspaces where each workspace folder may have its own clangd instance:

```typescript
import * as vscode from 'vscode';
import type { ClangdExtension, ClangdApiV2, ASTParams, ASTNode } from '@clangd/vscode-clangd';

const CLANGD_EXTENSION = 'llvm-vs-code-extensions.vscode-clangd';
const CLANGD_API_VERSION = 2;

const ASTRequestMethod = 'textDocument/ast';

async function getClangdApi(): Promise<ClangdApiV2 | undefined> {
    const clangdExtension = vscode.extensions.getExtension<ClangdExtension>(CLANGD_EXTENSION);
    if (!clangdExtension) {
        return undefined;
    }
    return (await clangdExtension.activate()).getApi(CLANGD_API_VERSION);
}

// Get the language client for a specific document
const provideHover = async (document: vscode.TextDocument, position: vscode.Position, _token: vscode.CancellationToken): Promise<vscode.Hover | undefined> => {
    const api = await getClangdApi();
    if (!api) {
        return undefined;
    }

    // Get the client for this specific document's workspace folder
    const client = api.getLanguageClient(document.uri);
    if (!client) {
        return undefined;
    }

    const textDocument = client.code2ProtocolConverter.asTextDocumentIdentifier(document);
    const range = client.code2ProtocolConverter.asRange(new vscode.Range(position, position));
    const params: ASTParams = { textDocument, range };

    const ast: ASTNode | undefined = await client.sendRequest(ASTRequestMethod, params);

    if (!ast) {
        return undefined;
    }

    return {
        contents: [ast.kind]
    };
};

vscode.languages.registerHoverProvider(['c', 'cpp'], { provideHover });
```

#### V2 API Methods

- `getLanguageClient(uri?: vscode.Uri)`: Get the language client for a specific URI. If no URI is provided, returns the client for the active editor.
- `getAllLanguageClients()`: Get all active language clients (one per workspace folder in per-folder mode, or a single global client otherwise).
- `onDidChangeClients`: Event fired when clients are added or removed (e.g., when workspace folders change or when clangd instances are restarted).

#### Listening for Client Changes

```typescript
const api = await getClangdApi();
if (api) {
    // React to client changes (restart, workspace folder added/removed)
    api.onDidChangeClients(() => {
        console.log('Clangd clients changed');
        // Refresh your extension's state as needed
    });
}
```

#### Working with All Clients

```typescript
const api = await getClangdApi();
if (api) {
    // Send a request to all active clangd instances
    const clients = api.getAllLanguageClients();
    for (const client of clients) {
        // Perform operations on each client
        const memoryUsage = await client.sendRequest('$/memoryUsage');
        console.log('Memory usage:', memoryUsage);
    }
}
```

### API V1 (Legacy)

The original API that returns a single language client. This works with both single-root and multi-root workspaces, returning the client for the currently active editor:

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
