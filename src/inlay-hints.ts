// This file implements the client side of clangd's inlay hints
// extension to LSP: https://clangd.llvm.org/extensions#inlay-hints
//
// The feature allows the server to provide the client with inline
// annotations to display for e.g. parameter names at call sites.
//
// This extension predates the textDocument/inlayHints request from LSP 3.17.
// The standard protocol is used when available (via vscode-languageclient) and
// this logic is disabled in that case. It will eventually be removed.

import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import {ClangdContext, ClangdLanguageClient} from './clangd-context';

namespace protocol {

export interface InlayHint {
  range: vscodelc.Range;
  position?: vscodelc.Position; // omitted by old servers, see hintSide.
  kind: string;
  label: string;
}

export interface InlayHintsParams {
  textDocument: vscodelc.TextDocumentIdentifier;
  range?: vscodelc.Range;
}

export namespace InlayHintsRequest {
export const type =
    new vscodelc.RequestType<InlayHintsParams, InlayHint[], void>(
        'clangd/inlayHints');
}

} // namespace protocol

export class InlayHintsFeature implements vscodelc.StaticFeature {
  public static activate(context: ClangdContext) {
    const enabledSetting = 'editor.inlayHints.enabled';
    context.subscriptions.push(
      vscode.commands.registerCommand('clangd.inlayHints.toggle', () => {
        // This used to be a boolean, and then became a 4-state enum.
        var val = vscode.workspace.getConfiguration().get<boolean|string>(
            enabledSetting, 'on');
        if (val === true || val === 'on')
          val = 'off';
        else if (val === false || val === 'off')
          val = 'on';
        else if (val === 'offUnlessPressed')
          val = 'onUnlessPressed';
        else if (val == 'onUnlessPressed')
          val = 'offUnlessPressed';
        else
          return;
        vscode.workspace.getConfiguration().update(
            enabledSetting, val, vscode.ConfigurationTarget.Global);
      }));
  }

  constructor(client: ClangdLanguageClient) {
    client.registerFeature(this);
  }

  fillClientCapabilities(_capabilities: vscodelc.ClientCapabilities) {}
  fillInitializeParams(_params: vscodelc.InitializeParams) {}

  initialize(capabilities: vscodelc.ServerCapabilities,
             _documentSelector: vscodelc.DocumentSelector|undefined) {
    const serverCapabilities: vscodelc.ServerCapabilities&
        {clangdInlayHintsProvider?: boolean, inlayHintProvider?: any} =
        capabilities;

    if (serverCapabilities.clangdInlayHintsProvider || serverCapabilities.inlayHintProvider) {
      vscode.commands.executeCommand('setContext', 'clangd.inlayHints.supported', true);
    }
  }
  getState(): vscodelc.FeatureState { return {kind: 'static'}; }
  clear() {}
}
