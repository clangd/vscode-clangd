import {BaseLanguageClient} from 'vscode-languageclient';
import * as vscodelc from 'vscode-languageclient/node';

export interface ClangdApiV1 {
  // vscode-clangd's language client which can be used to send requests to the
  // clangd language server
  // Standard requests:
  // https://microsoft.github.io/language-server-protocol/specifications/specification-current
  // clangd custom requests:
  // https://clangd.llvm.org/extensions
  languageClient: BaseLanguageClient|undefined
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

export interface ASTSearchParams {
  textDocument: vscodelc.TextDocumentIdentifier;
  query: string;
}

export interface ASTSearchResult {
  [binding: string]: ASTNode;
}

// clangd/completeASTMatcher wire format
// Send: the matcher expression typed so far, and the cursor offset into it.
export interface ASTMatcherCompletionParams {
  searchQuery: string;
  offset: number;
}

// Receive: candidate completions for the token at `offset`.
export interface ASTMatcherCompletion {
  // Text to insert at `offset` to complete the matcher.
  typedText: string;
  // Full matcher declaration (name, parameter types), shown to the user.
  matcherDecl: string;
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
