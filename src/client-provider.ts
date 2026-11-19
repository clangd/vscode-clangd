import * as vscode from 'vscode';

import type {ClangdContext} from './clangd-context';

export type ClangdFeature = 'ast'|'inlayHints'|'memoryUsage'|'typeHierarchy';

// Routes editor-facing features to the clangd instance for a document.
export interface ClangdClientProvider {
  readonly onDidRemoveContext: vscode.Event<ClangdContext>;
  contextForUri(uri: vscode.Uri): ClangdContext|undefined;
  activeContext(): ClangdContext|undefined;
  setFeatureSupport(context: ClangdContext, feature: ClangdFeature,
                    supported: boolean): void;
  restart(folder?: vscode.WorkspaceFolder): Promise<void>;
}
