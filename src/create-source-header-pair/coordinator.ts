//
// PAIR COORDINATOR
// ================
//
// Lean coordinator that orchestrates the source/header pair creation workflow.
// Uses dependency injection and delegates all implementation details to
// service and UI layers.

import * as vscode from 'vscode';

import {showConfigurationWizard} from '../pairing-rule-manager';

import {PairCreatorService} from './service';
import {PairCreatorUI} from './ui';

// PairCoordinator orchestrates the workflow between UI and Service layers.
// It follows the single responsibility principle and uses dependency injection.
export class PairCoordinator implements vscode.Disposable {
  private static readonly ERROR_MESSAGES = {
    NO_TARGET_DIRECTORY:
        'Cannot determine target directory. Please open a folder or a file first.',
    FILE_EXISTS: (filePath: string) => `File already exists: ${filePath}`,
    UNEXPECTED_ERROR: 'An unexpected error occurred.'
  } as const;

  private newPairCommand: vscode.Disposable;
  private configureRulesCommand: vscode.Disposable;

  // Constructor with dependency injection - receives pre-configured instances
  constructor(private readonly service: PairCreatorService,
              private readonly ui: PairCreatorUI) {
    // Register commands
    this.newPairCommand = vscode.commands.registerCommand(
        'clangd.newSourcePair', this.create, this);
    this.configureRulesCommand = vscode.commands.registerCommand(
        'clangd.newSourcePair.configureRules', this.configureRules, this);
  }

  // Dispose method for cleanup when extension is deactivated
  dispose() {
    this.newPairCommand.dispose();
    this.configureRulesCommand.dispose();
  }

  // Main workflow orchestration
  public async create(): Promise<void> {
    try {
      // 1. Determine where to create files
      const targetDirectory = await this.getTargetDirectory();
      if (!targetDirectory) {
        vscode.window.showErrorMessage(
            PairCoordinator.ERROR_MESSAGES.NO_TARGET_DIRECTORY);
        return;
      }

      // 2. Get user preferences for what to create
      const {language, uncertain} =
          await this.service.detectLanguageFromEditor();
      const rule = await this.ui.promptForPairingRule(language, uncertain);
      if (!rule)
        return;

      const fileName = await this.ui.promptForFileName(rule);
      if (!fileName)
        return;

      // 3. Prepare file paths and check for conflicts
      const {headerPath, sourcePath} =
          this.service.createFilePaths(targetDirectory, fileName, rule);
      const existingFilePath =
          await this.service.checkFileExistence(headerPath, sourcePath);
      if (existingFilePath) {
        vscode.window.showErrorMessage(
            PairCoordinator.ERROR_MESSAGES.FILE_EXISTS(existingFilePath));
        return;
      }

      // 4. Create the files
      await this.service.generateAndWriteFiles(fileName, rule, headerPath,
                                               sourcePath);

      // 5. Show success and handle post-creation tasks
      await this.ui.showSuccessAndOpenFile(headerPath, sourcePath);
      await this.service.handleOfferToSaveAsDefault(rule, language);

    } catch (error: unknown) {
      const errorMessage =
          error instanceof Error
              ? error.message
              : PairCoordinator.ERROR_MESSAGES.UNEXPECTED_ERROR;
      vscode.window.showErrorMessage(errorMessage);
    }
  }

  // Determine target directory with fallback to workspace picker
  private async getTargetDirectory(): Promise<vscode.Uri|undefined> {
    return await this.service.getTargetDirectory(
               vscode.window.activeTextEditor?.document?.uri.fsPath,
               vscode.workspace.workspaceFolders) ??
           await this.ui.showWorkspaceFolderPicker();
  }

  // Opens the configuration wizard for pairing rules
  public async configureRules(): Promise<void> {
    await showConfigurationWizard();
  }
}
