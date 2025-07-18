//
// PAIR COORDINATOR
// ================
//
// Main coordinator class that orchestrates the entire source/header pair
// creation workflow. Acts as the main entry point and manages the 
// interaction between service and UI layers.
//

import * as vscode from 'vscode';

import { PairingRule, showConfigurationWizard } from '../pairing-rule-manager';
import { PairCreatorService } from './service';
import { PairCreatorUI } from './ui';

// PairCoordinator coordinates the UI and Service layers to handle the complete
// file pair creation workflow. It serves as the main entry point and
// orchestrates the entire process.
export class PairCoordinator implements vscode.Disposable {
    private static readonly ERROR_MESSAGES = {
        NO_TARGET_DIRECTORY: 'Cannot determine target directory. Please open a folder or a file first.',
        FILE_EXISTS: (filePath: string) => `File already exists: ${filePath}`,
        UNEXPECTED_ERROR: 'An unexpected error occurred.'
    } as const;

    private newPairCommand: vscode.Disposable;
    private configureRulesCommand: vscode.Disposable;
    private service: PairCreatorService;
    private ui: PairCreatorUI;

    // Constructor registers the VS Code commands for creating source/header pairs
    // and configuring rules
    constructor() {
        this.service = new PairCreatorService();
        this.ui = new PairCreatorUI(this.service);

        // Register the main command for creating new source/header pairs
        this.newPairCommand = vscode.commands.registerCommand(
            'clangd.newSourcePair', this.create, this);

        // Register the command for configuring pairing rules
        this.configureRulesCommand = vscode.commands.registerCommand(
            'clangd.newSourcePair.configureRules', this.configureRules, this);
    }

    // Dispose method for cleanup when extension is deactivated
    dispose() {
        this.newPairCommand.dispose();
        this.configureRulesCommand.dispose();
    }

    // Main entry point for the file pair creation process
    // Orchestrates the entire workflow using the service and UI layers
    public async create(): Promise<void> {
        try {
            // 1. Get target directory
            const targetDirectory = await this.service.getTargetDirectory(
                vscode.window.activeTextEditor?.document?.uri.fsPath,
                vscode.workspace.workspaceFolders) ??
                await this.showWorkspaceFolderPicker();

            if (!targetDirectory) {
                vscode.window.showErrorMessage(PairCoordinator.ERROR_MESSAGES.NO_TARGET_DIRECTORY);
                return;
            }

            await this.createPairInDirectory(targetDirectory);

        } catch (error: unknown) {
            const errorMessage = error instanceof Error ? error.message : 'An unexpected error occurred.';
            vscode.window.showErrorMessage(errorMessage);
        }
    }

    // Show workspace folder picker when multiple folders are available
    private async showWorkspaceFolderPicker(): Promise<vscode.Uri | undefined> {
        const workspaceFolders = vscode.workspace.workspaceFolders;
        if (!workspaceFolders || workspaceFolders.length <= 1) {
            return undefined;
        }

        const selected = await vscode.window.showQuickPick(
            workspaceFolders.map(folder => ({
                label: folder.name,
                description: folder.uri.fsPath,
                folder: folder
            })),
            { placeHolder: 'Select workspace folder for new files' }
        );

        return selected?.folder.uri;
    }

    // Create file pair in the specified directory
    private async createPairInDirectory(targetDirectory: vscode.Uri): Promise<void> {
        // 2. Detect language and get user inputs
        const { language, uncertain } = await this.service.detectLanguageFromEditor();
        const rule = await this.ui.promptForPairingRule(language, uncertain);
        if (!rule) return;

        const fileName = await this.ui.promptForFileName(rule);
        if (!fileName) return;

        // 3. Create and validate file paths
        const { headerPath, sourcePath } = this.service.createFilePaths(targetDirectory, fileName, rule);

        const existingFilePath = await this.service.checkFileExistence(headerPath, sourcePath);
        if (existingFilePath) {
            vscode.window.showErrorMessage(
                PairCoordinator.ERROR_MESSAGES.FILE_EXISTS(existingFilePath));
            return;
        }

        // 4. Generate and write files
        await this.generateAndWriteFiles(fileName, rule, headerPath, sourcePath);

        // 5. Show success and open files
        await this.ui.showSuccessAndOpenFile(headerPath, sourcePath);

        // 6. Offer to save as default if it's a custom rule without existing config
        await this.offerToSaveAsDefault(rule, language);
    }

    /**
     * Offers to save the rule as default configuration after successful file creation
     * Only shows this for C++ rules when no existing configuration exists
     */
    private async offerToSaveAsDefault(rule: PairingRule, language: 'c' | 'cpp'): Promise<void> {
        // Only offer for C++ rules
        if (language !== 'cpp') {
            return;
        }

        // Check if user already has custom C++ rules configured
        const customRules = this.service.getAllPairingRules();
        const hasCppCustomRules = customRules.some(r => r.language === 'cpp');
        if (hasCppCustomRules) {
            return; // Don't prompt if they already have C++ configuration
        }

        // Check if this is a custom rule (has _custom suffix means user went through extension selection)
        const isCustomRule = rule.key.includes('custom');
        if (!isCustomRule) {
            return; // Don't prompt for built-in rules
        }

        const choice = await vscode.window.showInformationMessage(
            `Files created successfully! Would you like to save "${rule.headerExt}/${rule.sourceExt}" as your default C++ extensions for this workspace?`,
            'Save as Default',
            'Not Now'
        );

        if (choice === 'Save as Default') {
            await this.saveRuleAsDefault(rule);
        }
    }

    //Saves a rule as the default configuration with user choice of scope
    private async saveRuleAsDefault(rule: PairingRule): Promise<void> {
        const { PairingRuleService } = await import('../pairing-rule-manager');

        const scopeChoice = await vscode.window.showQuickPick([
            {
                label: 'Save for this Workspace',
                description: 'Recommended. Creates a .vscode/settings.json file.',
                scope: 'workspace'
            },
            {
                label: 'Save for all my Projects (Global)',
                description: 'Modifies your global user settings.',
                scope: 'user'
            }
        ], {
            placeHolder: 'Where would you like to save this configuration?',
            title: 'Save Configuration Scope'
        });

        if (!scopeChoice) return;

        try {
            // Create a clean rule for saving (remove the 'custom' key suffix)
            const cleanRule: PairingRule = {
                ...rule,
                key: rule.key.replace('_custom', ''),
                label: `${rule.language.toUpperCase()} Pair (${rule.headerExt}/${rule.sourceExt})`,
                description: `Creates a ${rule.headerExt}/${rule.sourceExt} file pair with header guards.`
            };

            await PairingRuleService.writeRules([cleanRule], scopeChoice.scope as 'workspace' | 'user');

            vscode.window.showInformationMessage(
                `Successfully saved '${rule.headerExt}/${rule.sourceExt}' as the default extension for ${scopeChoice.scope === 'workspace' ? 'this workspace' : 'all projects'}.`
            );
        } catch (error) {
            vscode.window.showErrorMessage(
                `Failed to save configuration: ${error instanceof Error ? error.message : 'Unknown error'}`
            );
        }
    }

    /**
     * Generates file content and writes both header and source files
     */
    private async generateAndWriteFiles(
        fileName: string,
        rule: PairingRule,
        headerPath: vscode.Uri,
        sourcePath: vscode.Uri
    ): Promise<void> {
        const eol = this.service.getLineEnding();
        const { headerContent, sourceContent } =
            this.service.generateFileContent(fileName, eol, rule);

        await this.service.writeFiles(headerPath, sourcePath, headerContent, sourceContent);
    }

    // Opens the configuration wizard for source/header pairing rules
    // Delegates to the PairingRuleManager's configuration interface
    public async configureRules(): Promise<void> {
        await showConfigurationWizard();
    }
}
