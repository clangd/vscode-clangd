import {BaseLanguageClient} from 'vscode-languageclient';
import * as vscodelc from 'vscode-languageclient/node';

export interface ClangdApiV1 {
  // vscode-clangd's language client which can be used to send requests to the
  // clangd language server
  // Standard requests:
  // https://microsoft.github.io/language-server-protocol/specifications/specification-current
  // clangd custom requests:
  // https://clangd.llvm.org/extensions
  languageClient: BaseLanguageClient|undefined;

  // Set the directory containing compile_commands.json that clangd should use,
  // without modifying the user's `clangd.arguments` setting. Triggers a clangd
  // restart; the returned Thenable resolves once the new client has been
  // scheduled to start. Resolves to undefined.
  setCompilationDatabasePath(path: string): Thenable<void>;
}

export interface ClangdExtension {
  getApi(version: 1): ClangdApiV1;
}

// clangd custom request types
// (type declarations for other requests may be added later)

// textDocument/ast wire format
// Send: position
export interface ASTParams {
  textDocument: vscodelc.TextDocumentIdentifier;
  range: vscodelc.Range;
}

// Receive: tree of ASTNode
export interface ASTNode {
  role: string;    // e.g. expression
  kind: string;    // e.g. BinaryOperator
  detail?: string; // e.g. ||
  arcana?: string; // e.g. BinaryOperator <0x12345> <col:12, col:1> 'bool' '||'
  children?: Array<ASTNode>;
  range?: vscodelc.Range;
}
