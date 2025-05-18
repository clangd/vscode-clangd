import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdLanguageClient} from './clangd-context';
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

export class InactiveRegionsFeature implements vscodelc.StaticFeature {
  private decorationType?: vscode.TextEditorDecorationType;
  private files: Map<string, vscode.Range[]> = new Map();

  constructor(private readonly client: ClangdLanguageClient) {
    client.registerFeature(this);
    client.subscriptions.push(client.onNotification(NotificationType, this.handleNotification.bind(this)));
  }

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
      this.client.subscriptions.push(
          vscode.window.onDidChangeVisibleTextEditors(
              () => this.client.visibleEditors().forEach(
                  (e) => this.applyHighlights(e.document.fileName))));
      this.client.subscriptions.push(
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
    const filePath = vscode.Uri.parse(params.textDocument.uri, true).fsPath;
    const ranges: vscode.Range[] = params.regions.map(
        (r) => this.client.protocol2CodeConverter.asRange(r));
    this.files.set(filePath, ranges);
    this.applyHighlights(filePath);
  }

  updateDecorationType() {
    this.decorationType?.dispose();
    if (config.get<boolean>('inactiveRegions.useBackgroundHighlight', this.client.clientOptions.workspaceFolder)) {
      this.decorationType = vscode.window.createTextEditorDecorationType({
        isWholeLine: true,
        backgroundColor:
            new vscode.ThemeColor('clangd.inactiveRegions.background'),
      });
    } else {
      this.decorationType = vscode.window.createTextEditorDecorationType({
        isWholeLine: true,
        opacity: config.get<number>('inactiveRegions.opacity', this.client.clientOptions.workspaceFolder).toString()
      });
    }
  }

  applyHighlights(filePath: string) {
    const ranges = this.files.get(filePath);
    if (!ranges)
      return;
    this.client.visibleEditors().forEach((e) => {
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
  clear() { this.decorationType?.dispose(); }
}
