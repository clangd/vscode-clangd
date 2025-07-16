import * as path from 'path';
import * as vscode from 'vscode';
import * as os from 'os';

import { ClangdContext } from './clangd-context';

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

  // The core implementation of the command.
  // It handles user input, file creation, and opening the new file.
  public async create() {
    const targetDirectory = await this.getTargetDirectory();
    if (!targetDirectory) {
      vscode.window.showErrorMessage(
        'Could not determine a target directory. Please open a folder or a file first.');
      return;
    }

    const className = await vscode.window.showInputBox({
      prompt: 'Please enter the name for the new C++ class.',
      placeHolder: 'MyClass',
      validateInput: text => {
        if (!text || text.trim().length === 0) {
          return 'Class name cannot be empty.';
        }
        if (!text.match(/^[a-zA-Z_][a-zA-Z0-9_]*$/)) {
          return 'Name must start with a letter or underscore and contain only letters, numbers, and underscores.';
        }
        return null;
      }
    });

    if (!className) {
      return; // User canceled the input
    }

    const headerPath =
      vscode.Uri.file(path.join(targetDirectory.fsPath, `${className}.h`));
    const sourcePath =
      vscode.Uri.file(path.join(targetDirectory.fsPath, `${className}.cpp`));

    // Prevent overwriting existing files
    const [headerExists, sourceExists] = await Promise.allSettled([
      vscode.workspace.fs.stat(headerPath),
      vscode.workspace.fs.stat(sourcePath)
    ]);

    if (headerExists.status === 'fulfilled') {
      vscode.window.showErrorMessage(
        `File already exists: ${headerPath.fsPath}`);
      return;
    }
    if (sourceExists.status === 'fulfilled') {
      vscode.window.showErrorMessage(
        `File already exists: ${sourcePath.fsPath}`);
      return;
    }


    // Get the appropriate line ending for this platform/user setting
    const eol = this.getLineEnding();

    const headerGuard = `${className.toUpperCase()}_H_`;

    // Define the content using clean template literals.
    const headerTemplate = `#ifndef ${headerGuard}
#define ${headerGuard}

class ${className} {
public:
  ${className}();
  ~${className}();

private:
  // Add private members here
};

#endif  // ${headerGuard}
`;

    const sourceTemplate = `#include "${className}.h"

${className}::${className}() {
  // Constructor implementation
}

${className}::~${className}() {
  // Destructor implementation
}
`;

    // Replace all LF (\n) characters in the templates with the correct EOL sequence.
    const headerContent = headerTemplate.replace(/\n/g, eol);
    const sourceContent = sourceTemplate.replace(/\n/g, eol);


    try {
      await Promise.all([
        vscode.workspace.fs.writeFile(headerPath, Buffer.from(headerContent, 'utf8')),
        vscode.workspace.fs.writeFile(sourcePath, Buffer.from(sourceContent, 'utf8'))
      ]);
    } catch (error: any) {
      vscode.window.showErrorMessage(
        `Failed to create files: ${error.message}`);
      return;
    }

    await vscode.window.showTextDocument(
      await vscode.workspace.openTextDocument(headerPath));
    await vscode.window.showInformationMessage(
      `Successfully created ${className}.h and ${className}.cpp`);
  }

  // Helper method to intelligently determine the directory for new files.
  private async getTargetDirectory(): Promise<vscode.Uri | undefined> {
    if (vscode.window.activeTextEditor && !vscode.window.activeTextEditor.document.isUntitled) {
      return vscode.Uri.file(
        path.dirname(vscode.window.activeTextEditor.document.uri.fsPath));
    }

    if (vscode.workspace.workspaceFolders &&
      vscode.workspace.workspaceFolders.length > 0) {
      if (vscode.workspace.workspaceFolders.length === 1) {
        return vscode.workspace.workspaceFolders[0].uri;
      }
      const picked = await vscode.window.showWorkspaceFolderPick({
        placeHolder: 'Please select a workspace folder to create the files in'
      });
      return picked?.uri;
    }

    return undefined;
  }
}

// Registers the command and lifecycle handler for the create pair feature.
export function registerCreateSourceHeaderPairCommand(context: ClangdContext) {
  context.subscriptions.push(new PairCreator());

}