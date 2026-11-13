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

/**
 * A directory served by its own clangd instance when
 * clangd.enablePerFolderServer is set.
 */
export interface ClangdProject {
  /** The root directory. Files below it are served by the project's clangd. */
  readonly uri: vscode.Uri;
  /**
   * Shown in the UI, e.g. in the output channel name. Defaults to the
   * basename of the root directory.
   */
  readonly name?: string;
  /** Arguments passed to the project's clangd after clangd.arguments. */
  readonly arguments?: readonly string[];
}

export interface ClangdProjectsChangeEvent {
  readonly added: readonly ClangdProject[];
  readonly removed: readonly ClangdProject[];
  /** Projects whose name or arguments changed. Their clangd restarts. */
  readonly updated: readonly ClangdProject[];
}

/** Contributes projects in multi-project mode, e.g. from a build system. */
export interface ClangdProjectProvider {
  /**
   * Lists all projects up front: a client serves the files below a project's
   * root from the start. Projects that are not on disk (scheme other than
   * `file`), duplicates and projects nested in another project are ignored.
   *
   * If the provider fails, doesn't respond within 10 seconds, or returns
   * something other than a list of projects, the projects of its last
   * successful call are kept (none if no call has succeeded yet). This is
   * logged in the clangd output channel. An answer after the 10 seconds is
   * still applied, unless provideProjects() was called again meanwhile.
   * `token` is cancelled when the result is no longer needed.
   */
  provideProjects(token: vscode.CancellationToken):
      vscode.ProviderResult<readonly ClangdProject[]>;
  /** Fired when provideProjects() would return a different result. */
  readonly onDidChangeProjects?: vscode.Event<void>;
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

  /**
   * The projects served in multi-project mode; empty in the default mode.
   * They are kept while clangd restarts or is shut down.
   */
  readonly projects: readonly ClangdProject[];

  /** Returns the project containing `uri`, if any. */
  getProject(uri: vscode.Uri): ClangdProject|undefined;

  /**
   * The project of the clangd editor focused last. Without one, the project
   * whose client is created next becomes active.
   */
  readonly activeProject: ClangdProject|undefined;

  /** Fired when activeProject changes. */
  readonly onDidChangeActiveProject: vscode.Event<ClangdProject|undefined>;

  /** Fired when projects are added, removed or updated. */
  readonly onDidChangeProjects: vscode.Event<ClangdProjectsChangeEvent>;

  /**
   * Registers a source of projects. While any provider is registered, the
   * projects are the union of the results of all providers; otherwise there
   * is one project per workspace folder. Providers take effect only in
   * multi-project mode; in the default mode, they are kept but not queried.
   * Throws if `id` is already registered, or if subscribing to the
   * provider's onDidChangeProjects throws.
   */
  registerProjectProvider(id: string,
                          provider: ClangdProjectProvider): vscode.Disposable;
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
