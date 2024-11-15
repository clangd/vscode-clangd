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

        // Extension may be disabled
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
