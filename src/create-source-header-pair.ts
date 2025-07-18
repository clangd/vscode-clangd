import * as os from 'os';
import * as path from 'path';
import * as vscode from 'vscode';

import {ClangdContext} from './clangd-context';

// --- Constants and Types ---

// Regular expressions for validating user input
const VALIDATION_PATTERNS = {
  IDENTIFIER: /^[a-zA-Z_][a-zA-Z0-9_]*$/ // Valid C/C++ identifier pattern
} as const;

// Default placeholder names for different template types
const DEFAULT_PLACEHOLDERS = {
  C_EMPTY: 'my_c_functions',
  C_STRUCT: 'MyStruct',
  CPP_EMPTY: 'utils',
  CPP_CLASS: 'MyClass',
  CPP_STRUCT: 'MyStruct'
} as const;

// Defines the structure of a file pair template rule
interface PairingRule {
  key: string;         // Unique identifier for this rule
  label: string;       // Human-readable name shown in UI
  description: string; // Detailed description of what this rule creates
  language: 'c'|'cpp'; // Target programming language
  headerExt: string;   // File extension for header file (e.g., '.h')
  sourceExt: string;   // File extension for source file (e.g., '.cpp', '.c')
  isClass?: boolean;   // Whether this rule creates a class template
  isStruct?: boolean;  // Whether this rule creates a struct template
}

// Available template rules for creating different types of file pairs
const TEMPLATE_RULES: ReadonlyArray<PairingRule> = [
  {
    key: 'cpp_empty',
    label: '$(new-file) Empty C++ Pair',
    description: 'Creates a basic .h/.cpp file pair with header guards.',
    language: 'cpp',
    headerExt: '.h',
    sourceExt: '.cpp'
  },
  {
    key: 'cpp_class',
    label: '$(symbol-class) C++ Class',
    description: 'Creates a .h/.cpp pair with a boilerplate class definition.',
    language: 'cpp',
    headerExt: '.h',
    sourceExt: '.cpp',
    isClass: true
  },
  {
    key: 'cpp_struct',
    label: '$(symbol-struct) C++ Struct',
    description: 'Creates a .h/.cpp pair with a boilerplate struct definition.',
    language: 'cpp',
    headerExt: '.h',
    sourceExt: '.cpp',
    isStruct: true
  },
  {
    key: 'c_empty',
    label: '$(file-code) Empty C Pair',
    description: 'Creates a basic .h/.c file pair for function declarations.',
    language: 'c',
    headerExt: '.h',
    sourceExt: '.c'
  },
  {
    key: 'c_struct',
    label: '$(symbol-struct) C Struct',
    description: 'Creates a .h/.c pair with a boilerplate typedef struct.',
    language: 'c',
    headerExt: '.h',
    sourceExt: '.c',
    isStruct: true
  }
];

// --- Main Class ---

// Main class responsible for creating header/source file pairs
class PairCreator implements vscode.Disposable {
  private command: vscode.Disposable;

  constructor() {
    this.command = vscode.commands.registerCommand('clangd.newSourcePair',
                                                   this.create, this);
  }

  dispose() { this.command.dispose(); }

  // Main entry point for creating a source/header file pair
  public async create(): Promise<void> {
    try {
      const targetDirectory = await this.getTargetDirectory();
      if (!targetDirectory) {
        vscode.window.showErrorMessage(
            'Cannot determine target directory. Please open a folder or a file first.');
        return;
      }

      const {language, uncertain} = await this.detectLanguage();

      const rule = await this.promptForPairingRule(language, uncertain);
      if (!rule)
        return;

      const fileName = await this.promptForFileName(rule);
      if (!fileName)
        return;

      const headerPath = vscode.Uri.file(
          path.join(targetDirectory.fsPath, `${fileName}${rule.headerExt}`));
      const sourcePath = vscode.Uri.file(
          path.join(targetDirectory.fsPath, `${fileName}${rule.sourceExt}`));

      const existingFilePath =
          await this.checkFileExistence(headerPath, sourcePath);
      if (existingFilePath) {
        vscode.window.showErrorMessage(
            `File already exists: ${existingFilePath}`);
        return;
      }

      const eol = this.getLineEnding();
      const {headerContent, sourceContent} =
          this.generateFileContent(fileName, eol, rule);

      await this.writeFiles(headerPath, sourcePath, headerContent,
                            sourceContent);
      await this.finalizeCreation(headerPath, sourcePath);

    } catch (error: any) {
      vscode.window.showErrorMessage(error.message ||
                                     'An unexpected error occurred.');
    }
  }

  // --- Helper Methods ---

  private getLineEnding(): string {
    const eolSetting =
        vscode.workspace.getConfiguration('files').get<string>('eol');
    return (eolSetting === '\n' || eolSetting === '\r\n') ? eolSetting : os.EOL;
  }

  // Detects the programming language context (C or C++) based on the current
  // active file
  private async detectLanguage():
      Promise<{language: 'c' | 'cpp', uncertain: boolean}> {
    const activeEditor = vscode.window.activeTextEditor;
    if (!activeEditor || activeEditor.document.isUntitled) {
      return {language: 'cpp', uncertain: true};
    }

    const document = activeEditor.document;
    const langId = document.languageId;
    const filePath = document.uri.fsPath;
    const ext = path.extname(filePath);

    // Strategy 1: Trust definitive source file extensions
    if (ext === '.c')
      return {language: 'c', uncertain: false};
    if (ext === '.cpp' || ext === '.cc' || ext === '.cxx')
      return {language: 'cpp', uncertain: false};

    // Strategy 2: For header files (.h), infer language from companion source
    // files
    if (ext === '.h') {
      const baseName = path.basename(filePath, '.h');
      const dirPath = path.dirname(filePath);

      const companionChecks = [
        path.join(dirPath, `${baseName}.c`),   // C source file
        path.join(dirPath, `${baseName}.cpp`), // C++ source file
        path.join(dirPath, `${baseName}.cc`),  // Alternative C++ extension
        path.join(dirPath, `${baseName}.cxx`)  // Alternative C++ extension
      ];

      const results = await Promise.allSettled(companionChecks.map(
          file => vscode.workspace.fs.stat(vscode.Uri.file(file))));

      if (results[0].status === 'fulfilled')
        return {language: 'c', uncertain: false};
      if (results.slice(1).some(r => r.status === 'fulfilled'))
        return {language: 'cpp', uncertain: false};

      return {language: 'cpp', uncertain: true};
    }

    // Strategy 3: Fallback to VS Code's language detection
    return {language: (langId === 'c' ? 'c' : 'cpp'), uncertain: true};
  }

  // Converts a string to PascalCase by splitting on hyphens and underscores
  private toPascalCase(input: string): string {
    return input.split(/[-_]/)
        .map(word => word.charAt(0).toUpperCase() + word.slice(1))
        .join('');
  }

  // Generates an appropriate placeholder for the file name input
  private getPlaceholder(rule: PairingRule): string {
    const activeEditor = vscode.window.activeTextEditor;
    if (activeEditor && !activeEditor.document.isUntitled) {
      const currentFileName =
          path.basename(activeEditor.document.fileName,
                        path.extname(activeEditor.document.fileName));
      // C uses snake_case by convention, C++ typically uses PascalCase for
      // classes
      if (rule.language === 'c')
        return currentFileName;
      return this.toPascalCase(currentFileName);
    }

    if (rule.isClass)
      return DEFAULT_PLACEHOLDERS.CPP_CLASS;
    if (rule.isStruct && rule.language === 'cpp')
      return DEFAULT_PLACEHOLDERS.CPP_STRUCT;
    if (rule.isStruct && rule.language === 'c')
      return DEFAULT_PLACEHOLDERS.C_STRUCT;
    if (rule.language === 'c')
      return DEFAULT_PLACEHOLDERS.C_EMPTY;
    return DEFAULT_PLACEHOLDERS.CPP_EMPTY;
  }

  // Prompts the user to select a file pair template type from available options
  private async promptForPairingRule(language: 'c'|'cpp', uncertain: boolean):
      Promise<PairingRule|undefined> {
    let desiredOrder: string[];

    if (uncertain) {
      desiredOrder =
          ['cpp_empty', 'c_empty', 'cpp_class', 'cpp_struct', 'c_struct'];
    } else if (language === 'c') {
      desiredOrder =
          ['c_empty', 'c_struct', 'cpp_empty', 'cpp_class', 'cpp_struct'];
    } else {
      desiredOrder =
          ['cpp_empty', 'cpp_class', 'cpp_struct', 'c_empty', 'c_struct'];
    }

    const choices = [...TEMPLATE_RULES].sort(
        (a, b) => desiredOrder.indexOf(a.key) - desiredOrder.indexOf(b.key));

    const result = await vscode.window.showQuickPick(choices, {
      placeHolder: 'Please select the type of file pair to create.',
      title: 'Create Pair - Step 1 of 2'
    });

    return result;
  }

  // Prompts the user to enter a name for the new file pair
  private async promptForFileName(rule: PairingRule):
      Promise<string|undefined> {
    let prompt = 'Please enter a name.';
    if (rule.isClass)
      prompt = 'Please enter the name for the new C++ class.';
    else if (rule.isStruct)
      prompt = `Please enter the name for the new ${
          rule.language.toUpperCase()} struct.`;
    else
      prompt = `Please enter the base name for the new ${
          rule.language.toUpperCase()} file pair.`;

    return vscode.window.showInputBox({
      prompt,
      placeHolder: this.getPlaceholder(rule),
      validateInput: (text) =>
          VALIDATION_PATTERNS.IDENTIFIER.test(text?.trim() || '')
              ? null
              : 'Invalid C/C++ identifier.',
      title: 'Create Pair - Step 2 of 2'
    });
  }
  // Generates the content for both header and source files based on the
  // selected template rule
  private generateFileContent(fileName: string, eol: string, rule: PairingRule):
      {headerContent: string, sourceContent: string} {
    const templates = this.getTemplatesByRule(rule);

    const context = {
      fileName,
      headerGuard: `${fileName.toUpperCase()}_H_`,
      includeLine: `#include "${fileName}${rule.headerExt}"`
    };

    const headerContent = this.applyTemplate(templates.header, context);
    const sourceContent = this.applyTemplate(templates.source, context);

    return {
      headerContent: headerContent.replace(/\n/g, eol),
      sourceContent: sourceContent.replace(/\n/g, eol)
    };
  }

  // Retrieves the appropriate file templates based on the selected pairing rule
  private getTemplatesByRule(rule: PairingRule):
      {header: string, source: string} {
    // C++ Class template with constructor/destructor
    if (rule.isClass) {
      return {
        header: `#ifndef {{headerGuard}}
#define {{headerGuard}}

class {{fileName}} {
public:
  {{fileName}}();
  ~{{fileName}}();

private:
  // Add private members here
};

#endif  // {{headerGuard}}
`,
        source: `{{includeLine}}

{{fileName}}::{{fileName}}() {
  // Constructor implementation
}

{{fileName}}::~{{fileName}}() {
  // Destructor implementation
}
`
      };
    }

    // C++ Struct template
    if (rule.isStruct && rule.language === 'cpp') {
      return {
        header: `#ifndef {{headerGuard}}
#define {{headerGuard}}

struct {{fileName}} {
  // Struct members
};

#endif  // {{headerGuard}}
`,
        source: '{{includeLine}}'
      };
    }

    // C Struct template with typedef
    if (rule.isStruct && rule.language === 'c') {
      return {
        header: `#ifndef {{headerGuard}}
#define {{headerGuard}}

typedef struct {
  // Struct members
} {{fileName}};

#endif  // {{headerGuard}}
`,
        source: '{{includeLine}}'
      };
    }

    // C Empty file template
    if (rule.language === 'c') {
      return {
        header: `#ifndef {{headerGuard}}
#define {{headerGuard}}

// Declarations for {{fileName}}.c

#endif  // {{headerGuard}}
`,
        source: `{{includeLine}}

// Implementations for {{fileName}}.c
`
      };
    }

    // C++ Empty file template (default)
    return {
      header: `#ifndef {{headerGuard}}
#define {{headerGuard}}

// Declarations for {{fileName}}.cpp

#endif  // {{headerGuard}}
`,
      source: '{{includeLine}}'
    };
  }

  // Applies template variable substitution using {{variable}} placeholders
  private applyTemplate(template: string,
                        context: Record<string, string>): string {
    return template.replace(/\{\{(\w+)\}\}/g,
                            (match, key) => { return context[key] || match; });
  }

  private async checkFileExistence(headerPath: vscode.Uri,
                                   sourcePath: vscode.Uri):
      Promise<string|null> {
    const pathsToCheck = [headerPath, sourcePath];
    for (const filePath of pathsToCheck) {
      try {
        await vscode.workspace.fs.stat(filePath);
        return filePath.fsPath;
      } catch {
        // File doesn't exist, continue checking
      }
    }
    return null;
  }

  private async writeFiles(headerPath: vscode.Uri, sourcePath: vscode.Uri,
                           headerContent: string,
                           sourceContent: string): Promise<void> {
    try {
      await Promise.all([
        vscode.workspace.fs.writeFile(headerPath,
                                      Buffer.from(headerContent, 'utf8')),
        vscode.workspace.fs.writeFile(sourcePath,
                                      Buffer.from(sourceContent, 'utf8'))
      ]);
    } catch (error: any) {
      throw new Error(`Failed to create files: ${error.message}.`);
    }
  }

  private async finalizeCreation(headerPath: vscode.Uri,
                                 sourcePath: vscode.Uri): Promise<void> {
    await vscode.window.showTextDocument(
        await vscode.workspace.openTextDocument(headerPath));
    await vscode.window.showInformationMessage(
        `Successfully created ${path.basename(headerPath.fsPath)} and ${
            path.basename(sourcePath.fsPath)}.`);
  }

  private async getTargetDirectory(): Promise<vscode.Uri|undefined> {
    const activeEditor = vscode.window.activeTextEditor;

    if (activeEditor && !activeEditor.document.isUntitled) {
      return vscode.Uri.file(path.dirname(activeEditor.document.uri.fsPath));
    }

    const workspaceFolders = vscode.workspace.workspaceFolders;
    if (workspaceFolders?.length === 1)
      return workspaceFolders[0].uri;

    if (workspaceFolders && workspaceFolders.length > 1) {
      const selectedFolder = await vscode.window.showWorkspaceFolderPick(
          {placeHolder: 'Please select a workspace folder for the new files.'});
      return selectedFolder?.uri;
    }

    return undefined;
  }
}

// Registers the create source/header pair command with the VS Code extension
// context
export function registerCreateSourceHeaderPairCommand(context: ClangdContext) {
  context.subscriptions.push(new PairCreator());
}