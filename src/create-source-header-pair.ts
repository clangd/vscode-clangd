//
// CREATE SOURCE HEADER PAIR
// =========================
// 
// PURPOSE:
// This module provides functionality to create matching header/source file pairs
// for C/C++ development. It intelligently detects language context, offers
// appropriate templates, and handles custom file extensions.
// 
// ARCHITECTURE:
// 1. PairCreatorService (Business Logic Layer)
//    - Language detection from file context
//    - File existence checking with caching
//    - Template content generation
//    - File system operations (read/write)
//    - Custom extension handling
// 
// 2. PairCreatorUI (User Interface Layer)
//    - User prompts and input validation
//    - Template selection dialogs
//    - Custom rule management integration
//    - Error handling and user feedback
// 
// 3. SourceHeaderPairCoordinator (Main Coordinator)
//    - Orchestrates the entire workflow
//    - Registers VS Code command
//    - Manages component lifecycle
// 
// WORKFLOW:
// Command triggered → Detect target directory → Analyze language context →
// Check for custom rules → Present template choices → Get file name →
// Validate uniqueness → Generate content → Write files → Open in editor
// 
// FEATURES:
// - Smart language detection (C vs C++)
// - Multiple template types (class, struct, empty)
// - Custom file extension support
// - Header guard generation
// - Cross-language template options
// - Workspace-aware directory selection
// - Input validation for C/C++ identifiers
// 
// INTEGRATION:
// Uses PairingRuleManager for custom extension configurations
// Integrates with VS Code file system and editor APIs
//

import * as path from 'path';
import * as vscode from 'vscode';

import { ClangdContext } from './clangd-context';
import { PairingRule, PairingRuleService } from './pairing-rule-manager';

// Types for better type safety
type Language = 'c' | 'cpp';
type TemplateKey = 'CPP_CLASS' | 'CPP_STRUCT' | 'C_STRUCT' | 'C_EMPTY' | 'CPP_EMPTY';

// Regular expression patterns to validate C/C++ identifiers
const VALIDATION_PATTERNS = {
  IDENTIFIER: /^[a-zA-Z_][a-zA-Z0-9_]*$/
};

// Default placeholder names for different file types
const DEFAULT_PLACEHOLDERS = {
  C_EMPTY: 'my_c_functions',
  C_STRUCT: 'MyStruct',
  CPP_EMPTY: 'utils',
  CPP_CLASS: 'MyClass',
  CPP_STRUCT: 'MyStruct'
};

// Template rules for available file pair types
const TEMPLATE_RULES: PairingRule[] = [
  {
    key: 'cpp_empty',
    label: '$(new-file) C++ Pair',
    description: 'Creates a basic Header/Source file pair with header guards.',
    language: 'cpp' as const,
    headerExt: '.h',
    sourceExt: '.cpp'
  },
  {
    key: 'cpp_class',
    label: '$(symbol-class) C++ Class',
    description:
      'Creates a Header/Source file pair with a boilerplate class definition.',
    language: 'cpp' as const,
    headerExt: '.h',
    sourceExt: '.cpp',
    isClass: true
  },
  {
    key: 'cpp_struct',
    label: '$(symbol-struct) C++ Struct',
    description:
      'Creates a Header/Source file pair with a boilerplate struct definition.',
    language: 'cpp' as const,
    headerExt: '.h',
    sourceExt: '.cpp',
    isStruct: true
  },
  {
    key: 'c_empty',
    label: '$(file-code) C Pair',
    description: 'Creates a basic .h/.c file pair for function declarations.',
    language: 'c' as const,
    headerExt: '.h',
    sourceExt: '.c'
  },
  {
    key: 'c_struct',
    label: '$(symbol-struct) C Struct',
    description: 'Creates a .h/.c file pair with a boilerplate typedef struct.',
    language: 'c' as const,
    headerExt: '.h',
    sourceExt: '.c',
    isStruct: true
  }
];

// File templates with immutable structure
const FILE_TEMPLATES = {
  CPP_CLASS: {
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
  },
  CPP_STRUCT: {
    header: `#ifndef {{headerGuard}}
#define {{headerGuard}}

struct {{fileName}} {
  // Struct members
};

#endif  // {{headerGuard}}
`,
    source: '{{includeLine}}'
  },
  C_STRUCT: {
    header: `#ifndef {{headerGuard}}
#define {{headerGuard}}

typedef struct {
  // Struct members
} {{fileName}};

#endif  // {{headerGuard}}
`,
    source: '{{includeLine}}'
  },
  C_EMPTY: {
    header: `#ifndef {{headerGuard}}
#define {{headerGuard}}

// Declarations for {{fileName}}.c

#endif  // {{headerGuard}}
`,
    source: `{{includeLine}}

// Implementations for {{fileName}}.c
`
  },
  CPP_EMPTY: {
    header: `#ifndef {{headerGuard}}
#define {{headerGuard}}

// Declarations for {{fileName}}.cpp

#endif  // {{headerGuard}}
`,
    source: '{{includeLine}}'
  }
};

// Service Layer - Core business logic
class PairCreatorService {
  // Cache for expensive file system operations
  private static readonly fileStatCache = new Map<string, Promise<boolean>>();

  // Definitive file extensions for fast lookup
  private static readonly DEFINITIVE_EXTENSIONS = {
    c: new Set(['.c']),
    cpp: new Set(['.cpp', '.cc', '.cxx', '.hh', '.hpp', '.hxx'])
  };


  // Optimized file existence check with caching to improve performance
  private static async fileExists(filePath: string): Promise<boolean> {
    if (this.fileStatCache.has(filePath)) {
      return this.fileStatCache.get(filePath)!;
    }

    const promise =
      Promise.resolve(vscode.workspace.fs.stat(vscode.Uri.file(filePath))
        .then(() => true, () => false));

    this.fileStatCache.set(filePath, promise);

    // Auto-clear cache entry after 5 seconds to prevent memory leaks
    setTimeout(() => this.fileStatCache.delete(filePath), 5000);

    return promise;
  }

  // Detects programming language from file info (pure business logic)
  // DETECTION STRATEGY:
  // 1. Fast path: Check file extension against definitive lists
  //    - .c files are definitely C
  //    - .cpp/.cc/.cxx files are definitely C++
  // 2. Special case: .h files are ambiguous
  //    - Look for companion files in same directory
  //    - .h + .c companion → C language
  //    - .h + .cpp/.cc/.cxx companion → C++ language
  //    - No companion found → default to C++ (uncertain)
  // 3. Fallback: Use VS Code's language ID detection
  // Returns: language type and uncertainty flag for UI decisions
  public async detectLanguage(languageId?: string, filePath?: string):
    Promise<{ language: Language, uncertain: boolean }> {
    if (!languageId || !filePath) {
      return { language: 'cpp', uncertain: true };
    }

    const ext = path.extname(filePath);

    // Fast path for definitive extensions
    if (PairCreatorService.DEFINITIVE_EXTENSIONS.c.has(ext)) {
      return { language: 'c', uncertain: false };
    }
    if (PairCreatorService.DEFINITIVE_EXTENSIONS.cpp.has(ext)) {
      return { language: 'cpp', uncertain: false };
    }

    // Special handling for .h files with companion file detection
    if (ext === '.h') {
      const result = await this.detectLanguageForHeaderFile(filePath);
      if (result)
        return result;
    }

    // Fallback to language ID
    return { language: languageId === 'c' ? 'c' : 'cpp', uncertain: true };
  }

  // Optimized header file language detection by checking companion files
  // COMPANION FILE DETECTION STRATEGY:
  // 1. Extract base name from header file (remove .h extension)
  // 2. Check for C companion first (.c) - less common, early exit optimization
  //    - If found: definitely C language
  // 3. Check for C++ companions in parallel (.cpp, .cc, .cxx)
  //    - If any found: definitely C++ language
  // 4. No companion found: default to C++ but mark as uncertain
  // This helps determine appropriate templates and reduces user confusion
  private async detectLanguageForHeaderFile(filePath: string):
    Promise<{ language: Language, uncertain: boolean } | null> {
    const baseName = path.basename(filePath, '.h');
    const dirPath = path.dirname(filePath);

    // Check for C companion file first (less common, check first for early
    // exit)
    const cFile = path.join(dirPath, `${baseName}.c`);
    if (await PairCreatorService.fileExists(cFile)) {
      return { language: 'c', uncertain: false };
    }

    // Check for C++ companion files in parallel
    const cppExtensions = ['.cpp', '.cc', '.cxx'];
    const cppChecks =
      cppExtensions.map(ext => PairCreatorService.fileExists(
        path.join(dirPath, `${baseName}${ext}`)));

    const results = await Promise.all(cppChecks);
    if (results.some((exists: boolean) => exists)) {
      return { language: 'cpp', uncertain: false };
    }

    return { language: 'cpp', uncertain: true };
  }

  // Gets all available pairing rules (custom + workspace + user)
  public getAllPairingRules(): PairingRule[] {
    return [
      ...(PairingRuleService.getRules('workspace') ?? []),
      ...(PairingRuleService.getRules('user') ?? [])
    ];
  }

  // Gets custom C++ extensions if available from configuration
  public getCustomCppExtensions(): { headerExt: string, sourceExt: string } | null {
    const allRules = this.getAllPairingRules();
    const cppCustomRule = allRules.find((rule: PairingRule) => rule.language === 'cpp');
    return cppCustomRule ? {
      headerExt: cppCustomRule.headerExt,
      sourceExt: cppCustomRule.sourceExt
    }
      : null;
  }

  // Converts string to PascalCase efficiently (pure function)
  public toPascalCase(input: string): string {
    return input.split(/[-_]+/)
      .map(word => word.charAt(0).toUpperCase() + word.slice(1))
      .join('');
  }

  // Gets default placeholder based on rule type (pure function)
  public getDefaultPlaceholder(rule: PairingRule): string {
    if (rule.isClass) {
      return DEFAULT_PLACEHOLDERS.CPP_CLASS;
    }

    if (rule.isStruct) {
      return rule.language === 'cpp' ? DEFAULT_PLACEHOLDERS.CPP_STRUCT
        : DEFAULT_PLACEHOLDERS.C_STRUCT;
    }

    return rule.language === 'c' ? DEFAULT_PLACEHOLDERS.C_EMPTY
      : DEFAULT_PLACEHOLDERS.CPP_EMPTY;
  }

  // Optimized line ending detection based on VS Code settings and platform
  public getLineEnding(): string {
    const eolSetting =
      vscode.workspace.getConfiguration('files').get<string>('eol');

    return eolSetting === '\n' || eolSetting === '\r\n' ? eolSetting
      : process.platform === 'win32' ? '\r\n'
        : '\n';
  }

  // Generates file content with improved template selection
  public generateFileContent(fileName: string, eol: string, rule: PairingRule): { headerContent: string; sourceContent: string; } {
    const templateKey: TemplateKey =
      rule.isClass ? 'CPP_CLASS'
        : rule.isStruct ? (rule.language === 'cpp' ? 'CPP_STRUCT' : 'C_STRUCT')
          : rule.language === 'c' ? 'C_EMPTY'
            : 'CPP_EMPTY';

    const templates = FILE_TEMPLATES[templateKey];
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

  // Optimized template variable substitution using regex replacement
  private applyTemplate(template: string,
    context: Record<string, string>): string {
    // Pre-compile regex for better performance if used frequently
    return template.replace(/\{\{(\w+)\}\}/g, (_, key) => context[key] ?? '');
  }

  // File existence check with parallel processing for multiple files
  public async checkFileExistence(headerPath: vscode.Uri,
    sourcePath: vscode.Uri):
    Promise<string | null> {
    const checks = [headerPath, sourcePath].map(async (uri) => {
      try {
        await vscode.workspace.fs.stat(uri);
        return uri.fsPath;
      } catch {
        return null;
      }
    });

    const results = await Promise.all(checks);
    return results.find(path => path !== null) ?? null;
  }

  // Optimized file writing with error handling and parallel writes
  public async writeFiles(headerPath: vscode.Uri, sourcePath: vscode.Uri,
    headerContent: string,
    sourceContent: string): Promise<void> {
    try {
      await Promise.all([
        vscode.workspace.fs.writeFile(headerPath,
          Buffer.from(headerContent, 'utf8')),
        vscode.workspace.fs.writeFile(sourcePath,
          Buffer.from(sourceContent, 'utf8'))
      ]);
    } catch (error) {
      throw new Error(`Failed to create files: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  // Smart target directory detection (pure business logic)
  public async getTargetDirectory(activeDocumentPath?: string,
    workspaceFolders
      ?: readonly vscode.WorkspaceFolder[]):
    Promise<vscode.Uri | undefined> {
    // Prefer current file's directory
    if (activeDocumentPath) {
      return vscode.Uri.file(path.dirname(activeDocumentPath));
    }

    // Return single workspace folder directly
    if (workspaceFolders?.length === 1) {
      return workspaceFolders[0].uri;
    }

    // Multiple workspace folders require UI selection
    return undefined;
  }

  // Language mismatch warning logic (pure business logic)
  public async shouldShowLanguageMismatchWarning(language: Language,
    result: PairingRule,
    currentDir?: string,
    activeFilePath
      ?: string): Promise<boolean> {
    if (!currentDir || !activeFilePath) {
      return true;
    }

    if (language === 'c' && result.language === 'cpp') {
      return this.checkForCppFilesInDirectory(currentDir);
    }

    return this.checkForCorrespondingSourceFiles(currentDir, activeFilePath,
      language);
  }

  // Check for C++ files in directory to inform language mismatch warnings
  private async checkForCppFilesInDirectory(dirPath: string): Promise<boolean> {
    try {
      const entries =
        await vscode.workspace.fs.readDirectory(vscode.Uri.file(dirPath));
      const hasCppFiles =
        entries.some(([fileName, fileType]) =>
          fileType === vscode.FileType.File &&
          PairCreatorService.DEFINITIVE_EXTENSIONS.cpp.has(
            path.extname(fileName)));
      return !hasCppFiles; // Show warning if NO C++ files found
    } catch {
      return true; // Show warning if can't check
    }
  }

  // Check for corresponding source files to inform language mismatch warnings
  private async checkForCorrespondingSourceFiles(
    dirPath: string, filePath: string, language: Language): Promise<boolean> {
    const baseName = path.basename(filePath, path.extname(filePath));
    const extensions = language === 'c' ? ['.c'] : ['.cpp', '.cc', '.cxx'];

    const checks = extensions.map(ext => PairCreatorService.fileExists(
      path.join(dirPath, `${baseName}${ext}`)));

    try {
      const results = await Promise.all(checks);
      return !results.some(
        (exists: boolean) => exists); // Show warning if NO corresponding files found
    } catch {
      return true; // Show warning if can't check
    }
  }
}

// UI Layer

// PairCreatorUI handles all user interface interactions for file pair creation.
// It manages dialogs, input validation, and user choices.
class PairCreatorUI {
  private service: PairCreatorService;

  constructor(service: PairCreatorService) { this.service = service; }

  // Gets placeholder name for input dialog, considering active file context
  private getPlaceholder(rule: PairingRule): string {
    const activeEditor = vscode.window.activeTextEditor;

    if (activeEditor?.document && !activeEditor.document.isUntitled) {
      const fileName =
        path.basename(activeEditor.document.fileName,
          path.extname(activeEditor.document.fileName));
      return rule.language === 'c' ? fileName
        : this.service.toPascalCase(fileName);
    }

    return this.service.getDefaultPlaceholder(rule);
  }

  // Gets target directory with UI fallback for multiple workspace folders
  public async getTargetDirectory(): Promise<vscode.Uri | undefined> {
    const activeEditor = vscode.window.activeTextEditor;
    const activeDocumentPath =
      activeEditor?.document && !activeEditor.document.isUntitled
        ? activeEditor.document.uri.fsPath
        : undefined;
    const workspaceFolders = vscode.workspace.workspaceFolders;

    // Try service layer first
    const result = await this.service.getTargetDirectory(activeDocumentPath,
      workspaceFolders);
    if (result) {
      return result;
    }

    // Handle multiple workspace folders with UI
    if (workspaceFolders && workspaceFolders.length > 1) {
      const selected = await vscode.window.showWorkspaceFolderPick(
        { placeHolder: 'Select workspace folder for new files' });
      return selected?.uri;
    }

    return undefined;
  }

  // Checks if language mismatch warning should be shown with UI context
  private async shouldShowLanguageMismatchWarning(language: Language,
    result: PairingRule):
    Promise<boolean> {
    const activeEditor = vscode.window.activeTextEditor;
    const currentDir =
      activeEditor?.document && !activeEditor.document.isUntitled
        ? path.dirname(activeEditor.document.uri.fsPath)
        : undefined;
    const activeFilePath =
      activeEditor?.document && !activeEditor.document.isUntitled
        ? activeEditor.document.uri.fsPath
        : undefined;

    return this.service.shouldShowLanguageMismatchWarning(
      language, result, currentDir, activeFilePath);
  }

  // Detects programming language from active editor context
  public async detectLanguage():
    Promise<{ language: Language, uncertain: boolean }> {
    const activeEditor = vscode.window.activeTextEditor;
    const languageId = activeEditor?.document?.languageId;
    const filePath = activeEditor?.document && !activeEditor.document.isUntitled
      ? activeEditor.document.uri.fsPath
      : undefined;

    return this.service.detectLanguage(languageId, filePath);
  }

  // Adapts template rules for display in UI based on custom extensions
  private adaptRuleForDisplay(rule: PairingRule): PairingRule {
    if (rule.language !== 'cpp') {
      return rule;
    }

    const customExtensions = this.service.getCustomCppExtensions();
    if (!customExtensions) {
      return rule;
    }

    const { headerExt, sourceExt } = customExtensions;

    // Adapt description for display
    const replacementPattern =
      /Header\/Source|\.h(?:h|pp|xx)?\/\.c(?:pp|c|xx)?/g;
    const newDescription = rule.description.replace(
      replacementPattern, `${headerExt}/${sourceExt}`);

    return { ...rule, description: newDescription, headerExt, sourceExt };
  }

  // Prepares template choices for UI display with proper ordering and adaptation
  private prepareTemplateChoices(language: 'c' | 'cpp',
    uncertain: boolean): PairingRule[] {
    const desiredOrder =
      uncertain
        ? ['cpp_empty', 'c_empty', 'cpp_class', 'cpp_struct', 'c_struct']
        : language === 'c'
          ? ['c_empty', 'c_struct', 'cpp_empty', 'cpp_class', 'cpp_struct']
          : ['cpp_empty', 'cpp_class', 'cpp_struct', 'c_empty', 'c_struct'];

    return [...TEMPLATE_RULES]
      .sort((a, b) =>
        desiredOrder.indexOf(a.key) - desiredOrder.indexOf(b.key))
      .map(rule => this.adaptRuleForDisplay(rule));
  }

  // Filters custom rules by language and prepares them for display
  private prepareCustomRulesChoices(allRules: PairingRule[],
    language: 'c' | 'cpp'): {
      languageRules: PairingRule[],
      adaptedDefaultTemplates: PairingRule[],
      otherLanguageTemplates: PairingRule[],
      cleanedCustomRules: PairingRule[]
    } {
    const languageRules = allRules.filter((rule: PairingRule) => rule.language === language);
    const customExt = languageRules.length > 0 ? languageRules[0] : null;

    let adaptedDefaultTemplates: PairingRule[] = [];

    if (customExt && language === 'cpp') {
      // For C++, adapt default templates with custom extensions
      adaptedDefaultTemplates =
        TEMPLATE_RULES
          .filter(
            template =>
              template.language === 'cpp' &&
              !languageRules.some(
                customRule =>
                  customRule.isClass === template.isClass &&
                  customRule.isStruct === template.isStruct &&
                  (customRule.isClass || customRule.isStruct ||
                    (!customRule.isClass && !customRule.isStruct &&
                      !template.isClass && !template.isStruct))))
          .map(
            template => ({
              ...template,
              key: `${template.key}_adapted`,
              headerExt: customExt.headerExt,
              sourceExt: customExt.sourceExt,
              description:
                template.description
                  .replace(
                    /Header\/Source/g,
                    `${customExt.headerExt}/${customExt.sourceExt}`)
                  .replace(/\.h\/\.cpp/g, `${customExt.headerExt}/${customExt.sourceExt}`)
                  .replace(/basic \.h\/\.cpp/g,
                    `basic ${customExt.headerExt}/${customExt.sourceExt}`)
                  .replace(/Creates a \.h\/\.cpp/g,
                    `Creates a ${customExt.headerExt}/${customExt.sourceExt}`)
            }));
    } else {
      // Standard adaptation for non-custom or C language
      adaptedDefaultTemplates =
        TEMPLATE_RULES
          .filter(template =>
            template.language === language &&
            !languageRules.some(
              customRule =>
                customRule.headerExt === template.headerExt &&
                customRule.sourceExt === template.sourceExt &&
                customRule.isClass === template.isClass &&
                customRule.isStruct === template.isStruct))
          .map(template => this.adaptRuleForDisplay(template));
    }

    const otherLanguageTemplates =
      TEMPLATE_RULES.filter(template => template.language !== language)
        .map(template => this.adaptRuleForDisplay(template));

    const cleanedCustomRules = allRules.map(
      (rule: PairingRule) => ({
        ...rule,
        label: rule.label.includes('$(')
          ? rule.label
          : `$(new-file) ${rule.language === 'cpp' ? 'C++' : 'C'} Pair (${rule.headerExt}/${rule.sourceExt})`,
        description: rule.description.startsWith('Creates a')
          ? rule.description
          : `Creates a ${rule.headerExt}/${rule.sourceExt} file pair with header guards.`
      }));

    return {
      languageRules,
      adaptedDefaultTemplates,
      otherLanguageTemplates,
      cleanedCustomRules
    };
  }

  // Checks for existing custom pairing rules and offers to create them if not found
  // For C++, presents options to use custom rules or create new ones.
  // For C, always uses default templates.
  // Returns selected rule, null if cancelled, undefined for defaults, or 'use_default' flag
  public async checkAndOfferCustomRules(language: 'c' | 'cpp',
    uncertain: boolean):
    Promise<PairingRule | null | undefined | 'use_default'> {
    if (language === 'c')
      return undefined; // Always use default C templates

    const allRules = this.service.getAllPairingRules();
    const languageRules = allRules.filter((rule: PairingRule) => rule.language === language);

    if (languageRules.length > 0) {
      const result = await this.selectFromCustomRules(allRules, language);
      return result === undefined ? null : result;
    }

    if (!uncertain) {
      const shouldCreateRules = await this.offerToCreateCustomRules(language);
      if (shouldCreateRules === null)
        return null;
      if (shouldCreateRules) {
        const result = await this.createCustomRules(language);
        return result === undefined ? null : result;
      }
    }

    return undefined;
  }

  // Presents a selection dialog for custom pairing rules
  // Combines custom rules with adapted default templates and cross-language options
  // Returns selected rule, undefined if cancelled, or 'use_default' flag
  public async selectFromCustomRules(allRules: PairingRule[],
    language: 'c' | 'cpp'):
    Promise<PairingRule | undefined | 'use_default'> {

    const {
      cleanedCustomRules,
      adaptedDefaultTemplates,
      otherLanguageTemplates
    } = this.prepareCustomRulesChoices(allRules, language);

    const choices = [
      ...cleanedCustomRules, ...adaptedDefaultTemplates,
      ...otherLanguageTemplates, {
        key: 'use_default',
        label: '$(list-unordered) Use Default Templates',
        description:
          'Use the built-in default pairing rules instead of custom rules',
        isSpecial: true
      }
    ];

    const result = await vscode.window.showQuickPick(choices, {
      placeHolder: `Select a ${language.toUpperCase()} pairing rule`,
      title: 'Custom Pairing Rules Available',
    });

    if (!result)
      return undefined;
    if ('isSpecial' in result && result.isSpecial &&
      result.key === 'use_default')
      return 'use_default';
    return result as PairingRule;
  }

  // Shows a dialog offering to create custom pairing rules for C++
  // Only applicable for C++ since C uses standard .c/.h extensions
  // Returns true to create rules, false to dismiss, null if cancelled
  public async offerToCreateCustomRules(language: 'c' |
    'cpp'): Promise<boolean | null> {
    if (language === 'c')
      return false;

    const result = await vscode.window.showInformationMessage(
      `No custom pairing rules found for C++. Would you like to create custom rules to use different file extensions (e.g., .cc/.hh instead of .cpp/.h)?`,
      { modal: false }, 'Create Custom Rules', 'Dismiss');

    return result === 'Create Custom Rules' ? true
      : result === 'Dismiss' ? false
        : null;
  }

  // Guides the user through creating custom pairing rules for C++
  // Offers common extension combinations or allows custom input
  // Saves the rule to workspace or global settings
  // Returns the created custom rule or undefined if cancelled
  public async createCustomRules(language: 'c' |
    'cpp'): Promise<PairingRule | undefined> {
    if (language === 'c')
      return undefined;

    const commonExtensions = [
      { label: '.h / .cpp (Default)', headerExt: '.h', sourceExt: '.cpp' },
      { label: '.hh / .cc (Alternative)', headerExt: '.hh', sourceExt: '.cc' }, {
        label: '.hpp / .cpp (Header Plus Plus)',
        headerExt: '.hpp',
        sourceExt: '.cpp'
      },
      { label: '.hxx / .cxx (Extended)', headerExt: '.hxx', sourceExt: '.cxx' },
      { label: 'Custom Extensions', headerExt: '', sourceExt: '' }
    ];

    const selectedExtension =
      await vscode.window.showQuickPick(commonExtensions, {
        placeHolder: `Select file extensions for C++ files`,
        title: 'Choose File Extensions'
      });

    if (!selectedExtension)
      return undefined;

    let { headerExt, sourceExt } = selectedExtension;

    if (!headerExt || !sourceExt) {
      const validateExt = (text: string) =>
        (!text || !text.startsWith('.') || text.length < 2)
          ? 'Please enter a valid file extension starting with a dot (e.g., .h)'
          : null;

      headerExt = await vscode.window.showInputBox({
        prompt: 'Enter header file extension (e.g., .h, .hh, .hpp)',
        placeHolder: '.h',
        validateInput: validateExt
      }) || '';

      if (!headerExt)
        return undefined;

      sourceExt = await vscode.window.showInputBox({
        prompt: `Enter source file extension for C++ (e.g., .cpp, .cc, .cxx)`,
        placeHolder: '.cpp',
        validateInput: validateExt
      }) || '';

      if (!sourceExt)
        return undefined;
    }

    const customRule: PairingRule = {
      key: `custom_cpp_${Date.now()}`,
      label: `$(new-file) C++ Pair (${headerExt}/${sourceExt})`,
      description:
        `Creates a ${headerExt}/${sourceExt} file pair with header guards.`,
      language: 'cpp',
      headerExt,
      sourceExt
    };

    const saveLocation = await vscode.window.showQuickPick(
      [
        {
          label: 'Workspace Settings',
          description: 'Save to current workspace only',
          value: 'workspace'
        },
        {
          label: 'Global Settings',
          description: 'Save to user settings (available in all workspaces)',
          value: 'user'
        }
      ],
      {
        placeHolder: 'Where would you like to save this custom rule?',
        title: 'Save Location'
      });

    if (!saveLocation)
      return undefined;

    try {
      const existingRules = PairingRuleService.getRules(
        saveLocation.value as 'workspace' | 'user') ||
        [];
      await PairingRuleService.writeRules([...existingRules, customRule],
        saveLocation.value as 'workspace' |
        'user');

      const locationText =
        saveLocation.value === 'workspace' ? 'workspace' : 'global';
      vscode.window.showInformationMessage(
        `Custom pairing rule saved to ${locationText} settings.`);

      return customRule;
    } catch (error: any) {
      vscode.window.showErrorMessage(
        `Failed to save custom rule: ${error.message}`);
      return undefined;
    }
  }

  // Prompts the user to select a pairing rule from available options
  // First checks for custom rules, then falls back to default templates
  // Returns selected pairing rule or undefined if cancelled
  public async promptForPairingRule(language: 'c' | 'cpp', uncertain: boolean):
    Promise<PairingRule | undefined> {
    const customRulesResult =
      await this.checkAndOfferCustomRules(language, uncertain);

    if (customRulesResult === null)
      return undefined;
    if (customRulesResult === 'use_default') {
      // Continue to default template selection
    } else if (customRulesResult) {
      return customRulesResult;
    }

    const choices = this.prepareTemplateChoices(language, uncertain);

    const result = await vscode.window.showQuickPick(choices, {
      placeHolder: 'Please select the type of file pair to create.',
      title: 'Create Source/Header Pair'
    });

    if (result && !uncertain && language !== result.language) {
      const shouldShowWarning =
        await this.shouldShowLanguageMismatchWarning(language, result);

      if (shouldShowWarning) {
        const detectedLangName = language === 'c' ? 'C' : 'C++';
        const selectedLangName = result.language === 'c' ? 'C' : 'C++';

        const shouldContinue = await vscode.window.showWarningMessage(
          `You're working in a ${detectedLangName} file but selected a ${selectedLangName} template. This may create files with incompatible extensions or content.`,
          'Continue', 'Cancel');

        if (shouldContinue !== 'Continue')
          return undefined;
      }
    }

    return result;
  }

  // Prompts the user to enter a name for the new file pair
  // Validates input as a valid C/C++ identifier and provides context-appropriate prompts
  // Returns the entered file name or undefined if cancelled
  public async promptForFileName(rule: PairingRule): Promise<string | undefined> {
    const prompt = rule.isClass ? 'Please enter the name for the new C++ class.'
      : rule.isStruct
        ? `Please enter the name for the new ${rule.language.toUpperCase()} struct.`
        : `Please enter the base name for the new ${rule.language.toUpperCase()} file pair.`;

    return vscode.window.showInputBox({
      prompt,
      placeHolder: this.getPlaceholder(rule),
      validateInput: (text) =>
        VALIDATION_PATTERNS.IDENTIFIER.test(text?.trim() || '')
          ? null
          : 'Invalid C/C++ identifier.',
      title: 'Create Source/Header Pair'
    });
  }

  // Shows success message and opens the newly created header file
  public async showSuccessAndOpenFile(headerPath: vscode.Uri,
    sourcePath: vscode.Uri): Promise<void> {
    await vscode.window.showTextDocument(
      await vscode.workspace.openTextDocument(headerPath));
    await vscode.window.showInformationMessage(
      `Successfully created ${path.basename(headerPath.fsPath)} and ${path.basename(sourcePath.fsPath)}.`);
  }
}

// Main Coordinator Class

// PairCreator coordinates the UI and Service layers to handle the complete file
// pair creation workflow. It serves as the main entry point and orchestrates
// the entire process.
class PairCreator implements vscode.Disposable {
  private command: vscode.Disposable;
  private service: PairCreatorService;
  private ui: PairCreatorUI;

  // Constructor registers the VS Code command for creating source/header pairs
  constructor() {
    this.service = new PairCreatorService();
    this.ui = new PairCreatorUI(this.service);
    this.command = vscode.commands.registerCommand(
      'clangd.createSourceHeaderPair', this.create, this);
  }

  // Dispose method for cleanup when extension is deactivated
  dispose() { this.command.dispose(); }

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
}

// Registers the create source/header pair command with the VS Code extension context
// This function should be called during extension activation to make the command available
export function registerCreateSourceHeaderPairCommand(context: ClangdContext) {
  context.subscriptions.push(new PairCreator());
}
