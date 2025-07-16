import * as path from 'path';
import * as vscode from 'vscode';
import * as os from 'os';

import { ClangdContext } from './clangd-context';

// Constants for file extensions and validation
const FILE_EXTENSIONS = {
  HEADER: '.h',
  C_SOURCE: '.c',
  CPP_SOURCE: '.cpp'
} as const;

const LANGUAGE_TYPES = {
  C: 'c',
  CPP: 'cpp'
} as const;

const VALIDATION_PATTERNS = {
  IDENTIFIER: /^[a-zA-Z_][a-zA-Z0-9_]*$/
} as const;

// Interface for template configuration
interface TemplateConfig {
  fileName: string;
  headerGuard: string;
  eol: string;
  isC: boolean;
}

// Interface for file generation result
interface FileGenerationResult {
  headerPath: vscode.Uri;
  sourcePath: vscode.Uri;
  headerContent: string;
  sourceContent: string;
}

// Handles the "Create Source/Header Pair" command.
class PairCreator implements vscode.Disposable {
  private command: vscode.Disposable;

  constructor() {
    this.command = vscode.commands.registerCommand(
      'clangd.createSourceHeaderPair', this.create, this);
  }

  // Implements vscode.Disposable to clean up resources.
  dispose() { this.command.dispose(); }


  // Helper method to get platform-appropriate line ending
  private getLineEnding(): string {
    const eolSetting = vscode.workspace.getConfiguration('files').get<string>('eol');
    if (eolSetting === '\n' || eolSetting === '\r\n') {
      return eolSetting;
    }
    return os.EOL;
  }

  // Detects the language type based on the active editor
  private detectLanguage(): 'c' | 'cpp' {
    const activeEditor = vscode.window.activeTextEditor;
    if (activeEditor && !activeEditor.document.isUntitled) {
      const langId = activeEditor.document.languageId;
      return langId === LANGUAGE_TYPES.C ? LANGUAGE_TYPES.C : LANGUAGE_TYPES.CPP;
    }
    return LANGUAGE_TYPES.CPP; // Default to C++
  }

  // Validates user input for file name
  private validateFileName(text: string): string | null {
    if (!text || text.trim().length === 0) {
      return 'Name cannot be empty.';
    }
    if (!VALIDATION_PATTERNS.IDENTIFIER.test(text)) {
      return 'Name must start with a letter or underscore and contain only letters, numbers, and underscores.';
    }
    return null;
  }

  // Prompts user for file name input
  private async promptForFileName(isC: boolean): Promise<string | undefined> {
    return vscode.window.showInputBox({
      prompt: isC
        ? 'Please enter the name for the new C files.'
        : 'Please enter the name for the new C++ class.',
      placeHolder: isC ? 'my_c_functions' : 'MyClass',
      validateInput: this.validateFileName
    });
  }

  // Checks if files already exist
  private async checkFileExistence(headerPath: vscode.Uri, sourcePath: vscode.Uri): Promise<boolean> {
    const [headerExists, sourceExists] = await Promise.allSettled([
      vscode.workspace.fs.stat(headerPath),
      vscode.workspace.fs.stat(sourcePath)
    ]);

    if (headerExists.status === 'fulfilled') {
      vscode.window.showErrorMessage(`File already exists: ${headerPath.fsPath}`);
      return true;
    }

    if (sourceExists.status === 'fulfilled') {
      vscode.window.showErrorMessage(`File already exists: ${sourcePath.fsPath}`);
      return true;
    }

    return false;
  }

  // Generates C language templates
  private generateCTemplates(config: TemplateConfig): { header: string; source: string } {
    const headerTemplate = `#ifndef ${config.headerGuard}
#define ${config.headerGuard}

// Function declarations for ${config.fileName}.c

#endif  // ${config.headerGuard}
`;

    const sourceTemplate = `#include "${config.fileName}.h"

// Function implementations for ${config.fileName}.c
`;

    return {
      header: headerTemplate.replace(/\n/g, config.eol),
      source: sourceTemplate.replace(/\n/g, config.eol)
    };
  }

  // Generates C++ language templates
  private generateCppTemplates(config: TemplateConfig): { header: string; source: string } {
    const headerTemplate = `#ifndef ${config.headerGuard}
#define ${config.headerGuard}

class ${config.fileName} {
public:
  ${config.fileName}();
  ~${config.fileName}();

private:
  // Add private members here
};

#endif  // ${config.headerGuard}
`;

    const sourceTemplate = `#include "${config.fileName}.h"

${config.fileName}::${config.fileName}() {
  // Constructor implementation
}

${config.fileName}::~${config.fileName}() {
  // Destructor implementation
}
`;

    return {
      header: headerTemplate.replace(/\n/g, config.eol),
      source: sourceTemplate.replace(/\n/g, config.eol)
    };
  }

  // Generates file templates based on language type
  private generateTemplates(config: TemplateConfig): { header: string; source: string } {
    return config.isC
      ? this.generateCTemplates(config)
      : this.generateCppTemplates(config);
  }

  // Prepares file generation data
  private prepareFileGeneration(
    fileName: string,
    targetDirectory: vscode.Uri,
    isC: boolean
  ): FileGenerationResult {
    const sourceExt = isC ? FILE_EXTENSIONS.C_SOURCE : FILE_EXTENSIONS.CPP_SOURCE;
    const headerPath = vscode.Uri.file(
      path.join(targetDirectory.fsPath, `${fileName}${FILE_EXTENSIONS.HEADER}`)
    );
    const sourcePath = vscode.Uri.file(
      path.join(targetDirectory.fsPath, `${fileName}${sourceExt}`)
    );

    const config: TemplateConfig = {
      fileName,
      headerGuard: `${fileName.toUpperCase()}_H_`,
      eol: this.getLineEnding(),
      isC
    };

    const templates = this.generateTemplates(config);

    return {
      headerPath,
      sourcePath,
      headerContent: templates.header,
      sourceContent: templates.source
    };
  }

  // Writes files to the filesystem
  private async writeFiles(generation: FileGenerationResult): Promise<void> {
    try {
      await Promise.all([
        vscode.workspace.fs.writeFile(
          generation.headerPath,
          Buffer.from(generation.headerContent, 'utf8')
        ),
        vscode.workspace.fs.writeFile(
          generation.sourcePath,
          Buffer.from(generation.sourceContent, 'utf8')
        )
      ]);
    } catch (error: any) {
      throw new Error(`Failed to create files: ${error.message}`);
    }
  }

  // Opens the created header file and shows success message
  private async finalizeCreation(generation: FileGenerationResult): Promise<void> {
    const fileName = path.basename(generation.headerPath.fsPath, FILE_EXTENSIONS.HEADER);
    const sourceExt = path.extname(generation.sourcePath.fsPath);

    await vscode.window.showTextDocument(
      await vscode.workspace.openTextDocument(generation.headerPath)
    );

    await vscode.window.showInformationMessage(
      `Successfully created ${fileName}${FILE_EXTENSIONS.HEADER} and ${fileName}${sourceExt}`
    );
  }

  // The core implementation of the command.
  // It handles user input, file creation, and opening the new file.
  public async create(): Promise<void> {
    try {
      // Step 1: Get target directory
      const targetDirectory = await this.getTargetDirectory();
      if (!targetDirectory) {
        vscode.window.showErrorMessage(
          'Could not determine a target directory. Please open a folder or a file first.'
        );
        return;
      }

      // Step 2: Detect language
      const language = this.detectLanguage();
      const isC = language === LANGUAGE_TYPES.C;

      // Step 3: Get file name from user
      const fileName = await this.promptForFileName(isC);
      if (!fileName) {
        return; // User canceled the input
      }

      // Step 4: Prepare file generation data
      const generation = this.prepareFileGeneration(fileName, targetDirectory, isC);

      // Step 5: Check if files already exist
      const filesExist = await this.checkFileExistence(generation.headerPath, generation.sourcePath);
      if (filesExist) {
        return;
      }

      // Step 6: Write files
      await this.writeFiles(generation);

      // Step 7: Finalize creation (open file and show message)
      await this.finalizeCreation(generation);

    } catch (error: any) {
      vscode.window.showErrorMessage(error.message || 'An unexpected error occurred.');
    }
  }

  // Helper method to intelligently determine the directory for new files.
  private async getTargetDirectory(): Promise<vscode.Uri | undefined> {
    // Try to use the directory of the currently active file
    const activeEditor = vscode.window.activeTextEditor;
    if (activeEditor && !activeEditor.document.isUntitled) {
      return vscode.Uri.file(path.dirname(activeEditor.document.uri.fsPath));
    }

    // Fall back to workspace folders
    const workspaceFolders = vscode.workspace.workspaceFolders;
    if (!workspaceFolders || workspaceFolders.length === 0) {
      return undefined;
    }

    // If there's only one workspace folder, use it
    if (workspaceFolders.length === 1) {
      return workspaceFolders[0].uri;
    }

    // If there are multiple workspace folders, let the user choose
    const picked = await vscode.window.showWorkspaceFolderPick({
      placeHolder: 'Please select a workspace folder to create the files in'
    });

    return picked?.uri;
  }
}

// Registers the command and lifecycle handler for the create pair feature.
export function registerCreateSourceHeaderPairCommand(context: ClangdContext) {
  context.subscriptions.push(new PairCreator());
}