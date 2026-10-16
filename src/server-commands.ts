// With several clangd instances, every client is told about the same server
// commands (clangd.applyFix, clangd.applyTweak) and vscode-languageclient would
// register each of them as a VS Code command once per client, which fails for
// all but the first client. Instead, the commands are registered once and each
// invocation is dispatched to the client serving the affected document.

import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

export class ServerCommandRegistry implements vscode.Disposable {
  private readonly commands = new Map<string, {
    owners: Set<vscodelc.BaseLanguageClient>,
    registration: vscode.Disposable
  }>();

  // clientFor() returns the client serving a document.
  constructor(private readonly clientFor:
                  (uri: vscode.Uri) => vscodelc.BaseLanguageClient |
                                       undefined) {}

  // Registers `command` unless it is already registered, and records `owner`
  // as a client that can execute it.
  add(command: string, owner: vscodelc.BaseLanguageClient) {
    let entry = this.commands.get(command);
    if (!entry) {
      const registration = vscode.commands.registerCommand(
          command, (...args: any[]) => this.execute(command, args));
      entry = {owners: new Set(), registration};
      this.commands.set(command, entry);
    }
    entry.owners.add(owner);
  }

  // Removes `owner` from all commands, and unregisters the commands that have
  // no owner left.
  remove(owner: vscodelc.BaseLanguageClient) {
    for (const [command, entry] of this.commands) {
      entry.owners.delete(owner);
      if (entry.owners.size === 0) {
        entry.registration.dispose();
        this.commands.delete(command);
      }
    }
  }

  async execute(command: string, args: any[]): Promise<unknown> {
    const owners = this.commands.get(command)?.owners;
    if (!owners)
      return undefined;
    const client = this.ownerFor(owners, args);
    if (!client) {
      vscode.window.showErrorMessage(
          `No clangd instance found to execute ${command}`);
      return undefined;
    }
    const params: vscodelc.ExecuteCommandParams = {command, arguments: args};
    try {
      return await client.sendRequest(vscodelc.ExecuteCommandRequest.type,
                                      params);
    } catch (error) {
      return client.handleFailedRequest(vscodelc.ExecuteCommandRequest.type,
                                        undefined, error, undefined);
    }
  }

  dispose() {
    this.commands.forEach((entry) => { entry.registration.dispose(); });
    this.commands.clear();
  }

  // Prefers the client serving the document the command applies to, then the
  // one serving the active editor, then the only owner.
  private ownerFor(owners: Set<vscodelc.BaseLanguageClient>,
                   args: any[]): vscodelc.BaseLanguageClient|undefined {
    for (const uri of [argumentUri(args),
                       vscode.window.activeTextEditor?.document.uri]) {
      const client = uri && this.clientFor(uri);
      if (client && owners.has(client))
        return client;
    }
    if (owners.size === 1)
      return owners.values().next().value;
    return undefined;
  }
}

// Returns the document a clangd command applies to. See ClangdLSPServer.cpp:
// clangd.applyTweak takes TweakArgs {file, selection, tweakID} and
// clangd.applyFix takes a WorkspaceEdit.
function argumentUri(args: any[]): vscode.Uri|undefined {
  const arg = args[0];
  if (!arg || typeof arg !== 'object')
    return undefined;
  let uri: unknown = arg.file;
  if (uri === undefined && Array.isArray(arg.documentChanges))
    uri = arg.documentChanges.find((c: any) => c?.textDocument?.uri)
              ?.textDocument.uri;
  if (uri === undefined && arg.changes && typeof arg.changes === 'object')
    uri = Object.keys(arg.changes)[0];
  if (typeof uri !== 'string')
    return undefined;
  try {
    return vscode.Uri.parse(uri, true);
  } catch {
    return undefined;
  }
}

// Moves the server commands of a client to a ServerCommandRegistry.
export class SharedServerCommandsFeature implements vscodelc.StaticFeature {
  private detached = false;

  constructor(private readonly registry: ServerCommandRegistry,
              private readonly client: vscodelc.BaseLanguageClient) {}

  // Removes the client's commands from the registry for good, e.g. when the
  // client is about to be stopped. A client can't be stopped while it starts,
  // and must not add its commands once it has started.
  detach() {
    this.detached = true;
    this.registry.remove(this.client);
  }

  fillClientCapabilities(_capabilities: vscodelc.ClientCapabilities) {}

  // Runs before any feature is initialized, so the built-in
  // ExecuteCommandFeature registers nothing.
  preInitialize(capabilities: vscodelc.ServerCapabilities) {
    const provider = capabilities.executeCommandProvider;
    if (!provider)
      return;
    // Replace the property rather than modifying the array: capabilities is a
    // shallow copy of the server's capabilities, so client.initializeResult
    // still lists all commands.
    capabilities.executeCommandProvider = {...provider, commands: []};
    if (this.detached)
      return;
    for (const command of provider.commands)
      this.registry.add(command, this.client);
  }

  initialize(_capabilities: vscodelc.ServerCapabilities,
             _documentSelector: vscodelc.DocumentSelector|undefined) {}
  getState(): vscodelc.FeatureState { return {kind: 'static'}; }
  // Called when the client stops.
  clear() { this.registry.remove(this.client); }
}
