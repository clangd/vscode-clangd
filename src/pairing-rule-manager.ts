import * as vscode from 'vscode';

// Public interface for pairing rules
export interface PairingRule {
  key: string;
  label: string;
  description: string;
  language: 'c'|'cpp';
  headerExt: string;
  sourceExt: string;
  isClass?: boolean;
  isStruct?: boolean;
}

// Type aliases for QuickPick items
type RuleQuickPickItem = vscode.QuickPickItem&{rule: PairingRule};
type ActionQuickPickItem = vscode.QuickPickItem&{key: string};

// Configuration management service class
export class PairingRuleService {
  private static readonly CONFIG_KEY = 'createPair.rules';

  // Validate a single pairing rule to ensure it has all required fields
  private static validateRule(rule: PairingRule): void {
    if (!rule.key || !rule.language || !rule.headerExt || !rule.sourceExt) {
      throw new Error(`Invalid rule: ${JSON.stringify(rule)}`);
    }
  }

  // Show error message and re-throw the error for proper error handling
  private static handleError(error: unknown, operation: string,
                             scope: string): never {
    const message = `Failed to ${operation} pairing rules for ${scope}: ${
        error instanceof Error ? error.message : 'Unknown error'}`;
    vscode.window.showErrorMessage(message);
    throw error;
  }

  // Get the currently active pairing rules from configuration
  public static getActiveRules(): ReadonlyArray<PairingRule> {
    return vscode.workspace.getConfiguration('clangd').get<PairingRule[]>(
        PairingRuleService.CONFIG_KEY, []);
  }

  // Check if custom rules exist for the specified scope (workspace or user)
  public static hasCustomRules(scope: 'workspace'|'user'): boolean {
    const inspection =
        vscode.workspace.getConfiguration('clangd').inspect<PairingRule[]>(
            PairingRuleService.CONFIG_KEY);
    const value = scope === 'workspace' ? inspection?.workspaceValue
                                        : inspection?.globalValue;
    return Array.isArray(value);
  }

  // Get pairing rules for a specific scope (workspace or user)
  public static getRules(scope: 'workspace'|'user'): PairingRule[]|undefined {
    const inspection =
        vscode.workspace.getConfiguration('clangd').inspect<PairingRule[]>(
            PairingRuleService.CONFIG_KEY);
    return scope === 'workspace' ? inspection?.workspaceValue
                                 : inspection?.globalValue;
  }

  // Write pairing rules to the specified scope (workspace or user)
  public static async writeRules(rules: PairingRule[],
                                 scope: 'workspace'|'user'): Promise<void> {
    try {
      if (!Array.isArray(rules))
        throw new Error('Rules must be an array');
      rules.forEach(PairingRuleService.validateRule);

      const target = scope === 'workspace'
                         ? vscode.ConfigurationTarget.Workspace
                         : vscode.ConfigurationTarget.Global;
      await vscode.workspace.getConfiguration('clangd').update(
          PairingRuleService.CONFIG_KEY, rules, target);
    } catch (error) {
      PairingRuleService.handleError(error, 'save', scope);
    }
  }

  // Reset pairing rules for the specified scope (remove custom rules)
  public static async resetRules(scope: 'workspace'|'user'): Promise<void> {
    try {
      const target = scope === 'workspace'
                         ? vscode.ConfigurationTarget.Workspace
                         : vscode.ConfigurationTarget.Global;
      await vscode.workspace.getConfiguration('clangd').update(
          PairingRuleService.CONFIG_KEY, undefined, target);
    } catch (error) {
      PairingRuleService.handleError(error, 'reset', scope);
    }
  }
}

// User interface management class
export class PairingRuleUI {
  // Predefined extension combinations for C++ file pairs
  private static readonly EXTENSION_OPTIONS = [
    {
      label: '.h / .cpp',
      description: 'Standard C++ extensions',
      headerExt: '.h',
      sourceExt: '.cpp',
      language: 'cpp' as const,
    },
    {
      label: '.hh / .cc',
      description: 'Alternative C++ extensions',
      headerExt: '.hh',
      sourceExt: '.cc',
      language: 'cpp' as const,
    },
    {
      label: '.hpp / .cpp',
      description: 'Header Plus Plus style',
      headerExt: '.hpp',
      sourceExt: '.cpp',
      language: 'cpp' as const,
    },
    {
      label: '.hxx / .cxx',
      description: 'Extended C++ extensions',
      headerExt: '.hxx',
      sourceExt: '.cxx',
      language: 'cpp' as const,
    },
  ];

  // Create rule choices from extension options for QuickPick display
  private static createRuleChoices(): RuleQuickPickItem[] {
    return PairingRuleUI.EXTENSION_OPTIONS.map(
        (option, index) => ({
          label: `$(file-code) ${option.label}`,
          description: option.description,
          rule: {
            key: `custom_${option.language}_${index}`,
            label: `${option.language.toUpperCase()} Pair (${
                option.headerExt}/${option.sourceExt})`,
            description: `Creates a ${option.headerExt}/${
                option.sourceExt} file pair with header guards.`,
            language: option.language,
            headerExt: option.headerExt,
            sourceExt: option.sourceExt,
          },
        }));
  }

  // Create advanced options separator and menu item for advanced management
  private static createAdvancedOptions(): ActionQuickPickItem[] {
    return [
      {
        label: 'Advanced Management',
        kind: vscode.QuickPickItemKind.Separator,
        key: 'separator',
      },
      {
        label: '$(settings-gear) Advanced Options...',
        description: 'Edit or reset rules manually',
        key: 'advanced_options',
      },
    ];
  }

  // Create advanced menu items based on current settings and available actions
  private static createAdvancedMenuItems(): ActionQuickPickItem[] {
    const items: ActionQuickPickItem[] = [
      {
        label: '$(edit) Edit Workspace Rules...',
        description: 'Opens .vscode/settings.json',
        key: 'edit_workspace',
      },
    ];

    if (PairingRuleService.hasCustomRules('workspace')) {
      items.push({
        label: '$(clear-all) Reset Workspace Rules',
        description: 'Use global or default rules instead',
        key: 'reset_workspace',
      });
    }

    items.push({
      label: 'Global (User) Settings',
      kind: vscode.QuickPickItemKind.Separator,
      key: 'separator_global',
    },
               {
                 label: '$(edit) Edit Global Rules...',
                 description: 'Opens your global settings.json',
                 key: 'edit_global',
               });

    if (PairingRuleService.hasCustomRules('user')) {
      items.push({
        label: '$(clear-all) Reset Global Rules',
        description: 'Use the extension default rules instead',
        key: 'reset_global',
      });
    }

    return items;
  }

  // Handle rule selection and ask for save scope (workspace or global)
  private static async handleRuleSelection(rule: PairingRule): Promise<void> {
    const selection = await vscode.window.showQuickPick(
        [
          {
            label: 'Save for this Workspace',
            description: 'Recommended. Creates a .vscode/settings.json file.',
            scope: 'workspace',
          },
          {
            label: 'Save for all my Projects (Global)',
            description: 'Modifies your global user settings.',
            scope: 'user',
          },
        ],
        {
          placeHolder: 'Where would you like to save this rule?',
          title: 'Save Configuration Scope',
        });

    if (!selection)
      return;

    await PairingRuleService.writeRules([rule], selection.scope as 'workspace' |
                                                    'user');
    vscode.window.showInformationMessage(`Successfully set '${
        rule.label}' as the default extension for the ${selection.scope}.`);
  }

  // Handle advanced menu selection and execute the corresponding action
  private static async handleAdvancedMenuSelection(key: string): Promise<void> {
    const actions = {
      edit_workspace: () => vscode.commands.executeCommand(
          'workbench.action.openWorkspaceSettingsFile'),
      edit_global: () =>
          vscode.commands.executeCommand('workbench.action.openSettingsJson'),
      reset_workspace: async () => {
        await PairingRuleService.resetRules('workspace');
        vscode.window.showInformationMessage(
            'Workspace pairing rules have been reset.');
      },
      reset_global: async () => {
        await PairingRuleService.resetRules('user');
        vscode.window.showInformationMessage(
            'Global pairing rules have been reset.');
      },
    };

    const action = actions[key as keyof typeof actions];
    if (action)
      await action();
  }

  // Main configuration wizard - entry point for setting up pairing rules
  public static async showConfigurationWizard(): Promise<void> {
    const quickPick =
        vscode.window.createQuickPick<RuleQuickPickItem|ActionQuickPickItem>();
    quickPick.title = 'Quick Setup: Choose File Extensions';
    quickPick.placeholder =
        'Choose file extension combination for this workspace, or go to advanced options.';
    quickPick.items = [
      ...PairingRuleUI.createRuleChoices(),
      ...PairingRuleUI.createAdvancedOptions()
    ];

    quickPick.onDidChangeSelection(async (selection) => {
      if (!selection[0])
        return;
      quickPick.hide();

      const item = selection[0];
      if ('rule' in item) {
        await PairingRuleUI.handleRuleSelection(item.rule);
      } else if (item.key === 'advanced_options') {
        await PairingRuleUI.showAdvancedManagementMenu();
      }
    });

    quickPick.onDidHide(() => quickPick.dispose());
    quickPick.show();
  }

  // Advanced management menu for editing and resetting pairing rules
  public static async showAdvancedManagementMenu(): Promise<void> {
    const selection = await vscode.window.showQuickPick(
        PairingRuleUI.createAdvancedMenuItems(), {
          title: 'Advanced Rule Management',
        });

    if (selection?.key) {
      await PairingRuleUI.handleAdvancedMenuSelection(selection.key);
    }
  }
}

// Backward compatibility export for the main configuration wizard
export const showConfigurationWizard = PairingRuleUI.showConfigurationWizard;