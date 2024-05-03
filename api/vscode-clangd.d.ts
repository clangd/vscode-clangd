import {BaseLanguageClient} from 'vscode-languageclient';
import * as vscodelc from 'vscode-languageclient/node';

// The wire format: we send a position, and get back a tree of ASTNode.
export interface ASTParams {
  textDocument: vscodelc.TextDocumentIdentifier;
  range: vscodelc.Range;
}

export interface ASTNode {
  role: string;    // e.g. expression
  kind: string;    // e.g. BinaryOperator
  detail?: string; // e.g. ||
  arcana?: string; // e.g. BinaryOperator <0x12345> <col:12, col:1> 'bool' '||'
  children?: Array<ASTNode>;
  range?: vscodelc.Range;
}

export const ASTType = 'textDocument/ast';

export interface ClangdApiV1 {
  languageClient: BaseLanguageClient
}

export interface ClangdExtension {
  getApi(version: 1): ClangdApiV1;
}
