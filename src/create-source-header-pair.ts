import * as path from 'path';
import * as vscode from 'vscode';
import * as os from 'os';

import { ClangdContext } from './clangd-context';

// --- Constants and Types ---

const VALIDATION_PATTERNS = {
  IDENTIFIER: /^[a-zA-Z_][a-zA-Z0-9_]*$/
} as const;

const DEFAULT_PLACEHOLDERS = {
  C_EMPTY: 'my_c_functions',
  C_STRUCT: 'MyStruct',
  CPP_EMPTY: 'utils',
  CPP_CLASS: 'MyClass',
  CPP_STRUCT: 'MyStruct'
} as const;

interface PairingRule {
  key: string;
  label: string;
  description: string;
  language: 'c' | 'cpp';
  headerExt: string;
  sourceExt: string;
  isClass?: boolean;
  isStruct?: boolean;
}

const TEMPLATE_RULES: ReadonlyArray<PairingRule> = [
  { key: 'cpp_empty', label: 'New Empty C++ Pair', description: 'Creates .h/.cpp files with header guards.', language: 'cpp', headerExt: '.h', sourceExt: '.cpp' },
  { key: 'cpp_class', label: 'New C++ Class', description: 'Creates .h/.cpp files for a C++ class.', language: 'cpp', headerExt: '.h', sourceExt: '.cpp', isClass: true },
  { key: 'cpp_struct', label: 'New C++ Struct', description: 'Creates .h/.cpp files for a C++ struct.', language: 'cpp', headerExt: '.h', sourceExt: '.cpp', isStruct: true },
  { key: 'c_empty', label: 'New Empty C Pair', description: 'Creates .h/.c files for C functions.', language: 'c', headerExt: '.h', sourceExt: '.c' },
  { key: 'c_struct', label: 'New C Struct', description: 'Creates .h/.c files for a C struct (using typedef).', language: 'c', headerExt: '.h', sourceExt: '.c', isStruct: true }
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
        vscode.window.showErrorMessage('Cannot determine target directory. Please open a folder or a file first.');
        return;
      }

      // Step 2: Perform language detection once to get all context.
      const { language, uncertain } = await this.detectLanguage();

      // Step 3: Prompt for the rule, passing all context.
      const rule = await this.promptForPairingRule(language, uncertain);
      if (!rule) return;

      const fileName = await this.promptForFileName(rule);
      if (!fileName) return;

      const headerPath = vscode.Uri.file(path.join(targetDirectory.fsPath, `${fileName}${rule.headerExt}`));
      const sourcePath = vscode.Uri.file(path.join(targetDirectory.fsPath, `${fileName}${rule.sourceExt}`));

      const existingFilePath = await this.checkFileExistence(headerPath, sourcePath);
      if (existingFilePath) {
        vscode.window.showErrorMessage(`File already exists: ${existingFilePath}`);
        return;
      }

      const eol = this.getLineEnding();
      const { headerContent, sourceContent } = this.generateFileContent(fileName, eol, rule);

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

  // --- START: The Refined, Unified detectLanguage Method ---
  private async detectLanguage(): Promise<{ language: 'c' | 'cpp', uncertain: boolean }> {
    const activeEditor = vscode.window.activeTextEditor;
    if (!activeEditor || activeEditor.document.isUntitled) {
      return { language: 'cpp', uncertain: true };
    }

    const document = activeEditor.document;
    const langId = document.languageId;
    const filePath = document.uri.fsPath;
    const ext = path.extname(filePath);

    // Strategy 1: Trust definitive source file language IDs
    if (ext === '.c') return { language: 'c', uncertain: false };
    if (ext === '.cpp' || ext === '.cc' || ext === '.cxx') return { language: 'cpp', uncertain: false };

    // Strategy 2: For header files, infer from companion files
    if (ext === '.h') {
      const baseName = path.basename(filePath, '.h');
      const dirPath = path.dirname(filePath);

      const companionChecks = [
        path.join(dirPath, `${baseName}.c`),
        path.join(dirPath, `${baseName}.cpp`),
        path.join(dirPath, `${baseName}.cc`),
        path.join(dirPath, `${baseName}.cxx`)
      ];

      const results = await Promise.allSettled(
        companionChecks.map(file => vscode.workspace.fs.stat(vscode.Uri.file(file)))
      );

      if (results[0].status === 'fulfilled') return { language: 'c', uncertain: false };
      if (results.slice(1).some(r => r.status === 'fulfilled')) return { language: 'cpp', uncertain: false };

      // If we are in a header file with no companion, the context is uncertain.
      return { language: 'cpp', uncertain: true };
    }

    // Strategy 3: Fallback using the potentially unreliable language ID
    return { language: (langId === 'c' ? 'c' : 'cpp'), uncertain: true };
  }
  // --- END: The Refined, Unified detectLanguage Method ---

  private toPascalCase(input: string): string {
    return input.split(/[-_]/).map(word => word.charAt(0).toUpperCase() + word.slice(1)).join('');
  }

  private getPlaceholder(rule: PairingRule): string {
    const activeEditor = vscode.window.activeTextEditor;
    if (activeEditor && !activeEditor.document.isUntitled) {
      const currentFileName = path.basename(activeEditor.document.fileName, path.extname(activeEditor.document.fileName));
      if (rule.language === 'c') return currentFileName;
      return this.toPascalCase(currentFileName);
    }

    if (rule.isClass) return DEFAULT_PLACEHOLDERS.CPP_CLASS;
    if (rule.isStruct && rule.language === 'cpp') return DEFAULT_PLACEHOLDERS.CPP_STRUCT;
    if (rule.isStruct && rule.language === 'c') return DEFAULT_PLACEHOLDERS.C_STRUCT;
    if (rule.language === 'c') return DEFAULT_PLACEHOLDERS.C_EMPTY;
    return DEFAULT_PLACEHOLDERS.CPP_EMPTY;
  }

  // --- START: The Refined, Simplified promptForPairingRule Method ---
  private async promptForPairingRule(language: 'c' | 'cpp', uncertain: boolean): Promise<PairingRule | undefined> {
    let desiredOrder: string[];

    if (uncertain) {
      // For orphan headers, present a neutral, user-friendly order.
      desiredOrder = ['cpp_empty', 'c_empty', 'cpp_class', 'cpp_struct', 'c_struct'];
    } else if (language === 'c') {
      // For C context, prioritize C options.
      desiredOrder = ['c_empty', 'c_struct', 'cpp_empty', 'cpp_class', 'cpp_struct'];
    } else {
      // For C++ context, prioritize C++ options.
      desiredOrder = ['cpp_empty', 'cpp_class', 'cpp_struct', 'c_empty', 'c_struct'];
    }

    const choices = [...TEMPLATE_RULES]
      .sort((a, b) => desiredOrder.indexOf(a.key) - desiredOrder.indexOf(b.key))
      .map(rule => ({ ...rule, label: `$(file-code) ${rule.label}` }));

    const result = await vscode.window.showQuickPick(choices, {
      placeHolder: 'Please select the type of file pair to create.',
      title: 'Create Pair - Step 1 of 2'
    });

    return result;
  }
  // --- END: The Refined, Simplified promptForPairingRule Method ---

  private async promptForFileName(rule: PairingRule): Promise<string | undefined> {
    let prompt = 'Please enter a name.';
    if (rule.isClass) prompt = 'Please enter the name for the new C++ class.';
    else if (rule.isStruct) prompt = `Please enter the name for the new ${rule.language.toUpperCase()} struct.`;
    else prompt = `Please enter the base name for the new ${rule.language.toUpperCase()} file pair.`;

    return vscode.window.showInputBox({
      prompt,
      placeHolder: this.getPlaceholder(rule),
      validateInput: (text) => VALIDATION_PATTERNS.IDENTIFIER.test(text?.trim() || '') ? null : 'Invalid C/C++ identifier.',
      title: 'Create Pair - Step 2 of 2'
    });
  }

  private generateFileContent(fileName: string, eol: string, rule: PairingRule): { headerContent: string, sourceContent: string } {
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

  private getTemplatesByRule(rule: PairingRule): { header: string, source: string } {
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

    // C++ Empty (default)
    return {
      header: `#ifndef {{headerGuard}}
#define {{headerGuard}}

// Declarations for {{fileName}}.cpp

#endif  // {{headerGuard}}
`,
      source: '{{includeLine}}'
    };
  }

  private applyTemplate(template: string, context: Record<string, string>): string {
    return template.replace(/\{\{(\w+)\}\}/g, (match, key) => {
      return context[key] || match; // If key exists in context, replace it. Otherwise, keep the original placeholder.
    });
  }

  private async checkFileExistence(headerPath: vscode.Uri, sourcePath: vscode.Uri): Promise<string | null> {
    const pathsToCheck = [headerPath, sourcePath];
    for (const filePath of pathsToCheck) {
      try {
        await vscode.workspace.fs.stat(filePath);
        return filePath.fsPath;
      } catch { }
    }
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
    if (workspaceFolders?.length === 1) return workspaceFolders[0].uri;
    if (workspaceFolders && workspaceFolders.length > 1) {
      const selectedFolder = await vscode.window.showWorkspaceFolderPick({ placeHolder: 'Please select a workspace folder for the new files.' });
      return selectedFolder?.uri;
    }
    return undefined;
  }
}

export function registerCreateSourceHeaderPairCommand(context: ClangdContext) {
  context.subscriptions.push(new PairCreator());
}