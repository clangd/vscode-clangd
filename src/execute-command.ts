import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';

import type {ClangdClientProvider} from './client-provider';

type ExecuteCommandClient =
    vscodelc.FeatureClient<vscodelc.ExecuteCommandMiddleware>;

interface RegisteredCommand {
  clients: Map<ExecuteCommandClient, number>;
  disposable: vscode.Disposable;
}

// vscode-languageclient normally registers every server command as a VS Code
// command. VS Code command identifiers are global, so that fails when multiple
// clangd clients advertise the same commands. Register each command once and
// route it to the client that owns the command's document instead.
export class ExecuteCommandRouter implements vscode.Disposable {
  private commands = new Map<string, RegisteredCommand>();
  private registrations =
      new Map<ExecuteCommandClient, Map<string, readonly string[]>>();
  private disposed = false;

  constructor(private provider: ClangdClientProvider) {}

  register(client: ExecuteCommandClient, id: string,
           commands: readonly string[]): void {
    if (this.disposed)
      return;
    this.unregister(client, id);
    let registrations = this.registrations.get(client);
    if (!registrations) {
      registrations = new Map();
      this.registrations.set(client, registrations);
    }
    registrations.set(id, commands);

    for (const command of commands) {
      let registered = this.commands.get(command);
      if (!registered) {
        registered = {
          clients: new Map(),
          disposable: vscode.commands.registerCommand(
              command, (...args: unknown[]) => this.execute(command, args)),
        };
        this.commands.set(command, registered);
      }
      registered.clients.set(client, (registered.clients.get(client) ?? 0) + 1);
    }
  }

  unregister(client: ExecuteCommandClient, id: string): void {
    const registrations = this.registrations.get(client);
    const commands = registrations?.get(id);
    if (!registrations || !commands)
      return;
    registrations.delete(id);
    if (!registrations.size)
      this.registrations.delete(client);

    for (const command of commands) {
      const registered = this.commands.get(command);
      if (!registered)
        continue;
      const count = registered.clients.get(client) ?? 0;
      if (count > 1) {
        registered.clients.set(client, count - 1);
      } else {
        registered.clients.delete(client);
      }
      if (!registered.clients.size) {
        registered.disposable.dispose();
        this.commands.delete(command);
      }
    }
  }

  clear(client: ExecuteCommandClient): void {
    const ids = Array.from(this.registrations.get(client)?.keys() ?? []);
    for (const id of ids)
      this.unregister(client, id);
  }

  dispose(): void {
    this.disposed = true;
    for (const registered of this.commands.values())
      registered.disposable.dispose();
    this.commands.clear();
    this.registrations.clear();
  }

  private execute(command: string,
                  args: unknown[]): vscode.ProviderResult<any> {
    const registered = this.commands.get(command);
    const client = registered && this.selectClient(registered.clients, args);
    if (!client)
      throw new Error(`No clangd client is available for ${command}`);

    const executeCommand = (selectedCommand: string, selectedArgs: any[]) => {
      const params: vscodelc.ExecuteCommandParams = {
        command: selectedCommand,
        arguments: selectedArgs,
      };
      return client.sendRequest(vscodelc.ExecuteCommandRequest.type, params)
          .then(undefined, error => client.handleFailedRequest(
                               vscodelc.ExecuteCommandRequest.type, undefined,
                               error, undefined));
    };
    return client.middleware.executeCommand
               ? client.middleware.executeCommand(command, args, executeCommand)
               : executeCommand(command, args);
  }

  private selectClient(clients: Map<ExecuteCommandClient, number>,
                       args: unknown[]): ExecuteCommandClient|undefined {
    for (const uri of commandUris(args)) {
      const client = this.provider.contextForUri(uri)?.client;
      if (client && clients.has(client))
        return client;
    }

    const activeClient = this.provider.activeContext()?.client;
    if (activeClient && clients.has(activeClient))
      return activeClient;
    if (clients.size === 1)
      return clients.keys().next().value;
    return undefined;
  }
}

export class RoutedExecuteCommandFeature implements
    vscodelc.DynamicFeature<vscodelc.ExecuteCommandRegistrationOptions> {
  private registrations = new Set<string>();

  constructor(private client: ExecuteCommandClient,
              private router: ExecuteCommandRouter) {}

  get registrationType() { return vscodelc.ExecuteCommandRequest.type; }

  getState(): vscodelc.FeatureState {
    return {
      kind: 'workspace',
      id: this.registrationType.method,
      registrations: this.registrations.size > 0,
    };
  }

  fillClientCapabilities(capabilities: vscodelc.ClientCapabilities): void {
    const workspace = capabilities.workspace ??= {};
    const executeCommand = workspace.executeCommand ??= {};
    executeCommand.dynamicRegistration = true;
  }

  initialize(capabilities: vscodelc.ServerCapabilities): void {
    if (!capabilities.executeCommandProvider)
      return;
    this.register({
      id: vscodelc.RAL().crypto.randomUUID(),
      registerOptions: {...capabilities.executeCommandProvider},
    });
  }

  register(data: vscodelc
               .RegistrationData<vscodelc.ExecuteCommandRegistrationOptions>):
      void {
    this.registrations.add(data.id);
    this.router.register(this.client, data.id, data.registerOptions.commands);
  }

  unregister(id: string): void {
    this.registrations.delete(id);
    this.router.unregister(this.client, id);
  }

  clear(): void {
    this.registrations.clear();
    this.router.clear(this.client);
  }
}

function commandUris(value: unknown): vscode.Uri[] {
  const uris: vscode.Uri[] = [];
  const seen = new Set<unknown>();

  const visit = (candidate: unknown) => {
    if (typeof candidate === 'string') {
      if (/^[A-Za-z][A-Za-z0-9+.-]*:/.test(candidate)) {
        try {
          uris.push(vscode.Uri.parse(candidate, true));
        } catch {
          // Not every string with a URI-like prefix is necessarily a URI.
        }
      }
      return;
    }
    if (!candidate || typeof candidate !== 'object' || seen.has(candidate))
      return;
    seen.add(candidate);
    if (Array.isArray(candidate)) {
      candidate.forEach(visit);
      return;
    }
    for (const [key, nested] of Object.entries(candidate)) {
      visit(key);
      visit(nested);
    }
  };

  visit(value);
  return uris;
}
