//
// PAIR CREATOR SERVICE
// ===================
//
// Business logic layer for file pair creation functionality.
// Handles language detection, file operations, template processing,
// and configuration management.
//

import * as path from 'path';
import * as vscode from 'vscode';

import { PairingRule, PairingRuleService } from '../pairing-rule-manager';
import {
    Language,
    TemplateKey,
    DEFAULT_PLACEHOLDERS,
    FILE_TEMPLATES
} from './templates';

// Service Layer - Core business logic
export class PairCreatorService {
    // Cache for expensive file system operations with TTL
    private static readonly fileStatCache = new Map<string, Promise<boolean>>();
    private static readonly CACHE_TTL = 5000; // 5 seconds

    // Definitive file extensions for fast lookup
    private static readonly DEFINITIVE_EXTENSIONS = {
        c: new Set(['.c']),
        cpp: new Set(['.cpp', '.cc', '.cxx', '.hh', '.hpp', '.hxx'])
    } as const;

    /**
     * Creates file paths for header and source files
     * @param targetDirectory Target directory URI
     * @param fileName Base file name without extension  
     * @param rule Pairing rule with extensions
     * @returns Object with headerPath and sourcePath URIs
     */
    public createFilePaths(targetDirectory: vscode.Uri, fileName: string, rule: PairingRule): {
        headerPath: vscode.Uri;
        sourcePath: vscode.Uri;
    } {
        return {
            headerPath: vscode.Uri.file(
                path.join(targetDirectory.fsPath, `${fileName}${rule.headerExt}`)),
            sourcePath: vscode.Uri.file(
                path.join(targetDirectory.fsPath, `${fileName}${rule.sourceExt}`))
        };
    }

    // Optimized file existence check with caching to improve performance
    private static async fileExists(filePath: string): Promise<boolean> {
        if (this.fileStatCache.has(filePath)) {
            return this.fileStatCache.get(filePath)!;
        }

        const promise = Promise.resolve(
            vscode.workspace.fs.stat(vscode.Uri.file(filePath))
                .then(() => true, () => false)
        );

        this.fileStatCache.set(filePath, promise);

        // Auto-clear cache entry after TTL to prevent memory leaks
        setTimeout(() => this.fileStatCache.delete(filePath), this.CACHE_TTL);

        return promise;
    }

    // Detects programming language from VS Code editor context
    // ARCHITECTURE NOTE: This method accesses VS Code APIs but belongs in service layer
    // because it contains the business logic for determining language context.
    // UI layer should only handle user interactions, not business decisions.
    public async detectLanguageFromEditor():
        Promise<{ language: Language, uncertain: boolean }> {
        const activeEditor = vscode.window.activeTextEditor;
        const languageId = activeEditor?.document?.languageId;
        const filePath = activeEditor?.document && !activeEditor.document.isUntitled
            ? activeEditor.document.uri.fsPath
            : undefined;

        return this.detectLanguage(languageId, filePath);
    }

    // Detects programming language from file info (pure business logic)
    // DETECTION STRATEGY:
    // 1. Fast path: Check file extension against definitive extension sets
    //    - .c files are definitely C
    //    - .cpp/.cc/.cxx/.hh/.hpp/.hxx are definitely C++
    // 2. Special case: .h files are ambiguous (could be C or C++)
    //    - Look for companion files in same directory to determine context
    //    - If MyClass.h exists with MyClass.cpp -> C++
    //    - If utils.h exists with utils.c -> C
    // 3. Fallback: Use VS Code's language ID detection
    //    - Based on file content analysis or user settings
    // 4. Default: When all else fails, assume C++ (more common in modern development)
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
    // HEADER FILE DETECTION STRATEGY:
    // Problem: .h files are used by both C and C++, making language detection ambiguous
    // Solution: Look for companion source files in the same directory
    // 
    // Algorithm:
    // 1. Extract base name from header file (e.g., "utils" from "utils.h")
    // 2. Check for C companion first (utils.c) - early exit optimization
    //    - C projects are less common, so checking first allows quick determination
    // 3. Check for C++ companions in parallel (utils.cpp, utils.cc, utils.cxx)
    //    - Use Promise.all for concurrent file existence checks
    // 4. Return definitive result if companion found, otherwise default to C++
    // 
    // Examples:
    // - math.h + math.c exists → Detected as C language
    // - Vector.h + Vector.cpp exists → Detected as C++ language  
    // - standalone.h (no companions) → Default to C++ (uncertain=true)
    private async detectLanguageForHeaderFile(filePath: string):
        Promise<{ language: Language, uncertain: boolean } | null> {
        const baseName = path.basename(filePath, '.h');
        const dirPath = path.dirname(filePath);

        // Check for C companion file first (less common, check first for early exit)
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
        const cppCustomRule =
            allRules.find((rule: PairingRule) => rule.language === 'cpp');
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

    /**
     * Checks if should offer to save rule as default and handles the save process
     */
    public async handleOfferToSaveAsDefault(rule: PairingRule, language: 'c' | 'cpp'): Promise<void> {
        // Only offer for C++ rules
        if (language !== 'cpp') {
            return;
        }

        // Check if user already has custom C++ rules configured
        const customRules = this.getAllPairingRules();
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

    /**
     * Saves a rule as the default configuration with user choice of scope
     */
    public async saveRuleAsDefault(rule: PairingRule): Promise<void> {
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

        if (!scopeChoice) {
            return;
        }

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
    public async generateAndWriteFiles(
        fileName: string,
        rule: PairingRule,
        headerPath: vscode.Uri,
        sourcePath: vscode.Uri
    ): Promise<void> {
        const eol = this.getLineEnding();
        const { headerContent, sourceContent } = this.generateFileContent(fileName, eol, rule);
        await this.writeFiles(headerPath, sourcePath, headerContent, sourceContent);
    }
}
