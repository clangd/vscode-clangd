import {BaseLanguageClient} from 'vscode-languageclient';
import * as vscodelc from 'vscode-languageclient/node';

export interface ClangdApiV1 {
  // vscode-clangd's language client which can be used to send requests to the
  // clangd language server
  // Standard requests:
  // https://microsoft.github.io/language-server-protocol/specifications/specification-current
  // clangd custom requests:
  // https://clangd.llvm.org/extensions
  languageClient: BaseLanguageClient;
}

export interface ClangdApiV2 {
  // vscode-clangd's language client which can be used to send requests to the
  // clangd language server
  // Standard requests:
  // https://microsoft.github.io/language-server-protocol/specifications/specification-current
  // clangd custom requests:
  // https://clangd.llvm.org/extensions
  languageClient: BaseLanguageClient | undefined;
}

export interface ClangdExtension {
  getApi(version: 1): ClangdApiV1;
  getApi(version: 2): ClangdApiV2;
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
