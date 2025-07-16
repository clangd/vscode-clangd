import * as path from 'path';
import * as vscode from 'vscode';
import * as os from 'os';

import { ClangdContext } from './clangd-context';

// --- Constants and Types ---

const FILE_EXTENSIONS = {
  HEADER: '.h',
  C_SOURCE: '.c',
  CPP_SOURCE: '.cpp'
} as const;

const VALIDATION_PATTERNS = {
  IDENTIFIER: /^[a-zA-Z_][a-zA-Z0-9_]*$/
} as const;

const DEFAULT_PLACEHOLDERS = {
  C_FILES: 'my_c_functions',
  C_STRUCT: 'MyStruct',
  CPP_FILES: 'utils',
  CPP_CLASS: 'MyClass',
  CPP_STRUCT: 'MyStruct'
} as const;

const TEMPLATE_KEYS = {
  CPP_FILES: 'cpp_files',
  CPP_CLASS: 'cpp_class',
  CPP_STRUCT: 'cpp_struct',
  C_FILES: 'c_files',
  C_STRUCT: 'c_struct'
} as const;
type TemplateType = typeof TEMPLATE_KEYS[keyof typeof TEMPLATE_KEYS];

const TEMPLATE_CHOICES: ReadonlyArray<vscode.QuickPickItem & { key: TemplateType }> = [
  { label: '$(new-file) C++ Files', description: 'Create an empty C++ header/source pair.', key: TEMPLATE_KEYS.CPP_FILES },
  { label: '$(symbol-class) C++ Class', description: 'Create a C++ header/source pair with a class definition.', key: TEMPLATE_KEYS.CPP_CLASS },
  { label: '$(symbol-struct) C++ Struct', description: 'Create a C++ header/source pair with a struct definition.', key: TEMPLATE_KEYS.CPP_STRUCT },
  { label: '$(file-code) C Files', description: 'Create a standard C header/source pair for functions.', key: TEMPLATE_KEYS.C_FILES },
  { label: '$(symbol-struct) C Struct', description: 'Create a standard C header/source pair with a struct definition.', key: TEMPLATE_KEYS.C_STRUCT }
];

// --- Main Class ---

class PairCreator implements vscode.Disposable {
  private command: vscode.Disposable;

  constructor() {
    this.command = vscode.commands.registerCommand('clangd.createSourceHeaderPair', this.create, this);
  }

  dispose() { this.command.dispose(); }

  public async create(): Promise<void> {
    try {
      const targetDirectory = await this.getTargetDirectory();
      if (!targetDirectory) {
        vscode.window.showErrorMessage('Could not determine a target directory. Please open a folder or a file first.');
        return;
      }

      const language = this.detectLanguage();
      const templateType = await this.promptForTemplateType(language);
      if (!templateType) return;

      const fileName = await this.promptForFileName(templateType);
      if (!fileName) return;

      const isC = templateType === TEMPLATE_KEYS.C_FILES || templateType === TEMPLATE_KEYS.C_STRUCT;
      const sourceExt = isC ? FILE_EXTENSIONS.C_SOURCE : FILE_EXTENSIONS.CPP_SOURCE;
      const headerPath = vscode.Uri.file(path.join(targetDirectory.fsPath, `${fileName}${FILE_EXTENSIONS.HEADER}`));
      const sourcePath = vscode.Uri.file(path.join(targetDirectory.fsPath, `${fileName}${sourceExt}`));

      const existingFilePath = await this.checkFileExistence(headerPath, sourcePath);
      if (existingFilePath) {
        vscode.window.showErrorMessage(`File already exists: ${existingFilePath}`);
        return;
      }

      const eol = this.getLineEnding();
      const { headerContent, sourceContent } = this.generateFileContent(fileName, eol, templateType);

      await this.writeFiles(headerPath, sourcePath, headerContent, sourceContent);
      await this.finalizeCreation(headerPath, sourcePath);

    } catch (error: any) {
      vscode.window.showErrorMessage(error.message || 'An unexpected error occurred.');
    }
  }

  // --- Helper Methods ---

  private getLineEnding(): string {
    const eolSetting = vscode.workspace.getConfiguration('files').get<string>('eol');
    return (eolSetting === '\n' || eolSetting === '\r\n') ? eolSetting : os.EOL;
  }

  private detectLanguage(): 'c' | 'cpp' {
    const activeEditor = vscode.window.activeTextEditor;
    return (activeEditor && !activeEditor.document.isUntitled && activeEditor.document.languageId === 'c') ? 'c' : 'cpp';
  }

  private toPascalCase(input: string): string {
    return input.split(/[-_]/).map(word => word.charAt(0).toUpperCase() + word.slice(1)).join('');
  }

  private getPlaceholder(templateType: TemplateType): string {
    const activeEditor = vscode.window.activeTextEditor;
    if (activeEditor && !activeEditor.document.isUntitled) {
      const currentFileName = path.basename(activeEditor.document.fileName, path.extname(activeEditor.document.fileName));
      if (templateType === TEMPLATE_KEYS.C_FILES || templateType === TEMPLATE_KEYS.C_STRUCT) {
        return currentFileName;
      }
      return this.toPascalCase(currentFileName);
    }

    switch (templateType) {
      case TEMPLATE_KEYS.C_FILES: return DEFAULT_PLACEHOLDERS.C_FILES;
      case TEMPLATE_KEYS.C_STRUCT: return DEFAULT_PLACEHOLDERS.C_STRUCT;
      case TEMPLATE_KEYS.CPP_CLASS: return DEFAULT_PLACEHOLDERS.CPP_CLASS;
      case TEMPLATE_KEYS.CPP_STRUCT: return DEFAULT_PLACEHOLDERS.CPP_STRUCT;
      default: return DEFAULT_PLACEHOLDERS.CPP_FILES;
    }
  }

  private async promptForTemplateType(language: 'c' | 'cpp'): Promise<TemplateType | undefined> {
    const cppOrder: TemplateType[] = [TEMPLATE_KEYS.CPP_FILES, TEMPLATE_KEYS.CPP_CLASS, TEMPLATE_KEYS.CPP_STRUCT, TEMPLATE_KEYS.C_FILES, TEMPLATE_KEYS.C_STRUCT];
    const cOrder: TemplateType[] = [TEMPLATE_KEYS.C_FILES, TEMPLATE_KEYS.C_STRUCT, TEMPLATE_KEYS.CPP_FILES, TEMPLATE_KEYS.CPP_CLASS, TEMPLATE_KEYS.CPP_STRUCT];
    const desiredOrder = language === 'c' ? cOrder : cppOrder;

    const orderedChoices = [...TEMPLATE_CHOICES].sort((a, b) => desiredOrder.indexOf(a.key) - desiredOrder.indexOf(b.key));
    orderedChoices.forEach((choice, index) => choice.picked = index === 0);

    const result = await vscode.window.showQuickPick(orderedChoices, {
      placeHolder: 'Please select the type of files to create.',
      title: 'Create Pair: Step 1 of 2'
    });
    return result?.key;
  }

  private validateIdentifier(text: string): string | null {
    if (!text?.trim()) return 'Name cannot be empty.';
    if (!VALIDATION_PATTERNS.IDENTIFIER.test(text)) return 'Invalid C/C++ identifier.';
    return null;
  }

  private async promptForFileName(templateType: TemplateType): Promise<string | undefined> {
    let prompt: string;
    switch (templateType) {
      case TEMPLATE_KEYS.C_FILES: prompt = 'Please enter the name for the new C files.'; break;
      case TEMPLATE_KEYS.C_STRUCT: prompt = 'Please enter the struct name for the new C files.'; break;
      case TEMPLATE_KEYS.CPP_CLASS: prompt = 'Please enter the class name.'; break;
      case TEMPLATE_KEYS.CPP_STRUCT: prompt = 'Please enter the struct name.'; break;
      default: prompt = 'Please enter the base name for the new C++ files.'; break;
    }

    return vscode.window.showInputBox({
      prompt,
      placeHolder: this.getPlaceholder(templateType),
      validateInput: this.validateIdentifier,
      title: 'Create Pair: Step 2 of 2'
    });
  }

  private generateFileContent(fileName: string, eol: string, templateType: TemplateType): { headerContent: string, sourceContent: string } {
    const headerGuard = `${fileName.toUpperCase()}_H_`;
    const includeLine = `#include "${fileName}.h"`;
    const headerGuardBlock = `#ifndef ${headerGuard}\n#define ${headerGuard}`;
    const endifLine = `\n#endif  // ${headerGuard}\n`;

    let headerTemplate: string, sourceTemplate: string;

    switch (templateType) {
      case TEMPLATE_KEYS.C_FILES:
        headerTemplate = `${headerGuardBlock}\n\n// Function declarations for ${fileName}.c\n${endifLine}`;
        sourceTemplate = `${includeLine}\n\n// Function implementations for ${fileName}.c\n`;
        break;
      case TEMPLATE_KEYS.C_STRUCT:
      case TEMPLATE_KEYS.CPP_STRUCT:
        headerTemplate = `${headerGuardBlock}\n\nstruct ${fileName} {\n  // Struct members\n};\n${endifLine}`;
        sourceTemplate = includeLine;
        break;
      case TEMPLATE_KEYS.CPP_CLASS:
        headerTemplate = `${headerGuardBlock}\n\nclass ${fileName} {\npublic:\n  ${fileName}();\n  ~${fileName}();\n\nprivate:\n  // Add private members here\n};\n${endifLine}`;
        sourceTemplate = `${includeLine}\n\n${fileName}::${fileName}() {\n  // Constructor implementation\n}\n\n${fileName}::~${fileName}() {\n  // Destructor implementation\n}\n`;
        break;
      default: // CPP_FILES
        headerTemplate = `${headerGuardBlock}\n\n// Declarations for ${fileName}\n${endifLine}`;
        sourceTemplate = includeLine;
        break;
    }

    return {
      headerContent: headerTemplate.replace(/\n/g, eol),
      sourceContent: sourceTemplate.replace(/\n/g, eol)
    };
  }

  private async checkFileExistence(headerPath: vscode.Uri, sourcePath: vscode.Uri): Promise<string | null> {
    const checks = await Promise.allSettled([
      vscode.workspace.fs.stat(headerPath),
      vscode.workspace.fs.stat(sourcePath)
    ]);

    if (checks[0].status === 'fulfilled') return headerPath.fsPath;
    if (checks[1].status === 'fulfilled') return sourcePath.fsPath;
    return null;
  }

  private async writeFiles(headerPath: vscode.Uri, sourcePath: vscode.Uri, headerContent: string, sourceContent: string): Promise<void> {
    try {
      await Promise.all([
        vscode.workspace.fs.writeFile(headerPath, Buffer.from(headerContent, 'utf8')),
        vscode.workspace.fs.writeFile(sourcePath, Buffer.from(sourceContent, 'utf8'))
      ]);
    } catch (error: any) {
      throw new Error(`Failed to create files: ${error.message}.`);
    }
  }

  private async finalizeCreation(headerPath: vscode.Uri, sourcePath: vscode.Uri): Promise<void> {
    await vscode.window.showTextDocument(await vscode.workspace.openTextDocument(headerPath));
    await vscode.window.showInformationMessage(`Successfully created ${path.basename(headerPath.fsPath)} and ${path.basename(sourcePath.fsPath)}.`);
  }

  private async getTargetDirectory(): Promise<vscode.Uri | undefined> {
    const activeEditor = vscode.window.activeTextEditor;
    if (activeEditor && !activeEditor.document.isUntitled) {
      return vscode.Uri.file(path.dirname(activeEditor.document.uri.fsPath));
    }

    const workspaceFolders = vscode.workspace.workspaceFolders;
    if (workspaceFolders?.length === 1) {
      return workspaceFolders[0].uri;
    }

    if (workspaceFolders && workspaceFolders.length > 1) {
      const picked = await vscode.window.showWorkspaceFolderPick({ placeHolder: 'Please select a workspace folder to create the files in.' });
      return picked?.uri;
    }

    return undefined;
  }
}

export function registerCreateSourceHeaderPairCommand(context: ClangdContext) {
  context.subscriptions.push(new PairCreator());
}