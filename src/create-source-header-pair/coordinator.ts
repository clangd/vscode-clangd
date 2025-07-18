//
// PAIR COORDINATOR
// ================
//
// Main coordinator class that orchestrates the entire source/header pair
// creation workflow. Acts as the main entry point and manages the 
// interaction between service and UI layers.
//

import * as path from 'path';
import * as vscode from 'vscode';

import { showConfigurationWizard } from '../pairing-rule-manager';
import { PairCreatorService } from './service';
import { PairCreatorUI } from './ui';

// PairCoordinator coordinates the UI and Service layers to handle the complete
// file pair creation workflow. It serves as the main entry point and
// orchestrates the entire process.
export class PairCoordinator implements vscode.Disposable {
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
            const targetDirectory = await this.ui.getTargetDirectory();
            if (!targetDirectory) {
                vscode.window.showErrorMessage(
                    'Cannot determine target directory. Please open a folder or a file first.');
                return;
            }

            const { language, uncertain } = await this.ui.detectLanguage();
            const rule = await this.ui.promptForPairingRule(language, uncertain);
            if (!rule)
                return;

            const fileName = await this.ui.promptForFileName(rule);
            if (!fileName)
                return;

            const headerPath = vscode.Uri.file(
                path.join(targetDirectory.fsPath, `${fileName}${rule.headerExt}`));
            const sourcePath = vscode.Uri.file(
                path.join(targetDirectory.fsPath, `${fileName}${rule.sourceExt}`));

            const existingFilePath =
                await this.service.checkFileExistence(headerPath, sourcePath);
            if (existingFilePath) {
                vscode.window.showErrorMessage(
                    `File already exists: ${existingFilePath}`);
                return;
            }

            const eol = this.service.getLineEnding();
            const { headerContent, sourceContent } =
                this.service.generateFileContent(fileName, eol, rule);

            await this.service.writeFiles(headerPath, sourcePath, headerContent,
                sourceContent);
            await this.ui.showSuccessAndOpenFile(headerPath, sourcePath);

        } catch (error: any) {
            vscode.window.showErrorMessage(error.message ||
                'An unexpected error occurred.');
        }
    }

    // Opens the configuration wizard for source/header pairing rules
    // Delegates to the PairingRuleManager's configuration interface
    public async configureRules(): Promise<void> {
        await showConfigurationWizard();
    }
}
