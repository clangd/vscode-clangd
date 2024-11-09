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

import {ClangdContext, clangdDocumentSelector} from './clangd-context';

export function activate(context: ClangdContext) {
  const feature = new InlayHintsFeature(context);
  context.client.registerFeature(feature);
}

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

class InlayHintsFeature implements vscodelc.StaticFeature {
  private commandRegistered = false;

  constructor(private readonly context: ClangdContext) {}

  fillClientCapabilities(_capabilities: vscodelc.ClientCapabilities) {}
  fillInitializeParams(_params: vscodelc.InitializeParams) {}

  initialize(capabilities: vscodelc.ServerCapabilities,
             _documentSelector: vscodelc.DocumentSelector|undefined) {
    const serverCapabilities: vscodelc.ServerCapabilities&
        {clangdInlayHintsProvider?: boolean, inlayHintProvider?: any} =
        capabilities;
    vscode.commands.executeCommand(
        'setContext', 'clangd.inlayHints.supported',
        serverCapabilities.clangdInlayHintsProvider ||
            serverCapabilities.inlayHintProvider);
    if (!this.commandRegistered) {
      // The command provides a quick way to toggle inlay hints
      // (key-bindable).
      // FIXME: this is a core VSCode setting, ideally they provide the
      // command. We toggle it globally, language-specific is nicer but
      // undiscoverable.
      this.commandRegistered = true;
      const enabledSetting = 'editor.inlayHints.enabled';
      this.context.subscriptions.push(
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
    // If the clangd server supports LSP 3.17 inlay hints, these are handled by
    // the vscode-languageclient library - don't send custom requests too!
    if (!serverCapabilities.clangdInlayHintsProvider ||
        serverCapabilities.inlayHintProvider)
      return;
    this.context.subscriptions.push(vscode.languages.registerInlayHintsProvider(
        clangdDocumentSelector, new Provider(this.context)));
  }
  getState(): vscodelc.FeatureState { return {kind: 'static'}; }
  clear() {}
}

class Provider implements vscode.InlayHintsProvider {
  constructor(private context: ClangdContext) {}

  decodeKind(kind: string): vscode.InlayHintKind|undefined {
    if (kind == 'type')
      return vscode.InlayHintKind.Type;
    if (kind == 'parameter')
      return vscode.InlayHintKind.Parameter;
    return undefined;
  }

  decode(hint: protocol.InlayHint): vscode.InlayHint {
    return {
      position:
          this.context.client.protocol2CodeConverter.asPosition(hint.position!),
      kind: this.decodeKind(hint.kind),
      label: hint.label.trim(),
      paddingLeft: hint.label.startsWith(' '),
      paddingRight: hint.label.endsWith(' '),
    };
  }

  async provideInlayHints(document: vscode.TextDocument, range: vscode.Range,
                          token: vscode.CancellationToken):
      Promise<vscode.InlayHint[]> {
    const request: protocol.InlayHintsParams = {
      textDocument: {uri: document.uri.toString()},
      range: this.context.client.code2ProtocolConverter.asRange(range),
    };

    const result = await this.context.client.sendRequest(
        protocol.InlayHintsRequest.type, request, token);
    return result.map(this.decode, this);
  }
}
