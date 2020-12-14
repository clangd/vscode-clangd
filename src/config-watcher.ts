import * as vscode from 'vscode';

const WATCHED_SETTINGS: String[] =
    ['path', 'arguments', 'trace', 'fallbackFlags', 'semanticHighlighting'];

async function handleConfigurationChanged(e: vscode.ConfigurationChangeEvent) {
  for (let setting of WATCHED_SETTINGS) {
    if (e.affectsConfiguration(`clangd.${setting}`)) {
      vscode.commands.executeCommand('clangd.restart');
      break;
    }
  }
}

export function activate(context: vscode.ExtensionContext) {
  context.subscriptions.push(
      vscode.workspace.onDidChangeConfiguration(handleConfigurationChanged))
}
