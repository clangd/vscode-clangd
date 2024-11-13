import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext} from './clangd-context';
import * as config from './config';

// Parameters for the inactive regions (server-side) push notification.
export interface InactiveRegionsParams {
  // The text document that has to be decorated with the inactive regions
  // information.
  textDocument: vscodelc.VersionedTextDocumentIdentifier;
  // An array of inactive regions.
  regions: vscodelc.Range[];
}

// Language server push notification providing the inactive regions
// information for a text document.
export const NotificationType =
    new vscodelc.NotificationType<InactiveRegionsParams>(
        'textDocument/inactiveRegions');

export function activate(context: ClangdContext) {
  const feature = new InactiveRegionsFeature(context);
  if (context.client) {
    context.client.registerFeature(feature);
    context.client.onNotification(NotificationType,
                                  feature.handleNotification.bind(feature));
  }
}

export class InactiveRegionsFeature implements vscodelc.StaticFeature {
  private decorationType?: vscode.TextEditorDecorationType;
  private files: Map<string, vscode.Range[]> = new Map();
  private context: ClangdContext;

  constructor(context: ClangdContext) { this.context = context; }

  fillClientCapabilities(capabilities: vscodelc.ClientCapabilities) {
    // Extend the ClientCapabilities type and add inactive regions
    // capability to the object.
    if (capabilities.textDocument) {
      const textDocumentCapabilities: vscodelc.TextDocumentClientCapabilities&
          {inactiveRegionsCapabilities?: {inactiveRegions: boolean}} =
          capabilities.textDocument;
      textDocumentCapabilities.inactiveRegionsCapabilities = {
        inactiveRegions: true,
      };
    }
  }
  initialize(capabilities: vscodelc.ServerCapabilities,
             documentSelector: vscodelc.DocumentSelector|undefined) {
    const serverCapabilities: vscodelc.ServerCapabilities&
        {inactiveRegionsProvider?: any} = capabilities;
    if (serverCapabilities.inactiveRegionsProvider) {
      this.updateDecorationType();
      this.context.subscriptions.push(
          vscode.window.onDidChangeVisibleTextEditors(
              (editors) => editors.forEach(
                  (e) => this.applyHighlights(e.document.fileName))));
      this.context.subscriptions.push(
          vscode.workspace.onDidChangeConfiguration((conf) => {
            const inactiveSettingsChanged =
                conf.affectsConfiguration(
                    'clangd.inactiveRegions.useBackgroundHighlight') ||
                conf.affectsConfiguration('clangd.inactiveRegions.opacity');
            if (!(conf.affectsConfiguration('workbench.colorTheme') ||
                  inactiveSettingsChanged))
              return;
            if (inactiveSettingsChanged) {
              this.updateDecorationType();
            }
            vscode.window.visibleTextEditors.forEach((e) => {
              if (!this.decorationType)
                return;
              const ranges = this.files.get(e.document.fileName);
              if (!ranges)
                return;
              e.setDecorations(this.decorationType, ranges);
            });
          }));
    }
  }

  handleNotification(params: InactiveRegionsParams) {
    if (!this.context.client) {
      return;
    }
    const filePath = vscode.Uri.parse(params.textDocument.uri, true).fsPath;
    const ranges: vscode.Range[] = params.regions.map(
        (r) => this.context.client!.protocol2CodeConverter.asRange(r));
    this.files.set(filePath, ranges);
    this.applyHighlights(filePath);
  }

  updateDecorationType() {
    this.decorationType?.dispose();
    if (config.get<boolean>('inactiveRegions.useBackgroundHighlight')) {
      this.decorationType = vscode.window.createTextEditorDecorationType({
        isWholeLine: true,
        backgroundColor:
            new vscode.ThemeColor('clangd.inactiveRegions.background'),
      });
    } else {
      this.decorationType = vscode.window.createTextEditorDecorationType({
        isWholeLine: true,
        opacity: config.get<number>('inactiveRegions.opacity').toString()
      });
    }
  }

  applyHighlights(filePath: string) {
    const ranges = this.files.get(filePath);
    if (!ranges)
      return;
    this.context.visibleClangdEditors.forEach((e) => {
      if (!this.decorationType)
        return;
      if (e.document.fileName !== filePath)
        return;
      e.setDecorations(this.decorationType, ranges);
    });
  }

  getState(): vscodelc.FeatureState { return {kind: 'static'}; }

  // clears inactive region decorations on disposal so they don't persist after
  // extension is deactivated
  dispose() { this.decorationType?.dispose(); }
}