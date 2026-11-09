import * as vscode from 'vscode';
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

export interface ClangdApiV2 {
  /**
   * Whether clangd.enablePerFolderServer is in effect, i.e. every project has
   * its own clangd instance.
   */
  readonly multiProject: boolean;

  /**
   * Returns the client serving `uri`. Without `uri`, returns the client of the
   * active editor if it shows a C/C++ file, else the one of the clangd editor
   * focused last. In the default mode, that is the single client, whatever
   * the uri.
   *
   * Like all functions of this API, it returns only clients that are starting
   * or running; requests sent to a starting client are delayed until it runs.
   * Clients that have stopped, e.g. after crashing too often, are left out.
   */
  getLanguageClient(uri?: vscode.Uri): BaseLanguageClient|undefined;

  /**
   * Returns all clients. In multi-project mode, a client starts once one of
   * its files is shown in an editor, so requests that are not about a file,
   * e.g. workspace/symbol, only reach the clients that have started.
   */
  getAllLanguageClients(): BaseLanguageClient[];

  /**
   * Starts the client serving `uri` if needed, and resolves to it once it
   * runs. Resolves to undefined if no client serves `uri` (in multi-project
   * mode: no project contains it, or its clangd was shut down), or if the
   * client fails to start.
   */
  startLanguageClient(uri: vscode.Uri): Promise<BaseLanguageClient|undefined>;

  /**
   * Fired when the clients may have changed, e.g. when a client starts or
   * stops.
   */
  readonly onDidChangeClients: vscode.Event<void>;
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
