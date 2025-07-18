//
// PAIR CREATOR UI
// ===============
//
// User interface layer for file pair creation functionality.
// Handles all user interactions, input validation, dialogs,
// and template selection.
//

import * as path from 'path';
import * as vscode from 'vscode';

import { PairingRule, PairingRuleService } from '../pairing-rule-manager';
import { PairCreatorService } from './service';
import { Language, VALIDATION_PATTERNS, TEMPLATE_RULES } from './templates';

// PairCreatorUI handles all user interface interactions for file pair creation.
// It manages dialogs, input validation, and user choices.
export class PairCreatorUI {
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
        const languageRules =
            allRules.filter((rule: PairingRule) => rule.language === language);
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
        const languageRules =
            allRules.filter((rule: PairingRule) => rule.language === language);

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
        } catch (error: unknown) {
            const errorMessage = error instanceof Error ? error.message : 'Unknown error occurred';
            vscode.window.showErrorMessage(
                `Failed to save custom rule: ${errorMessage}`);
            return undefined;
        }
    }

    // Prompts the user to select a pairing rule from available options
    // IMPROVED FLOW: 
    // 1. Check for existing custom rules first
    // 2. If no custom rules, show template choice (C/C++ types)
    // 3. If C++ selected, then choose extensions 
    // 4. After successful creation, offer to save as default
    // Returns selected pairing rule or undefined if cancelled
    public async promptForPairingRule(language: 'c' | 'cpp', uncertain: boolean):
        Promise<PairingRule | undefined> {

        // First check if there are existing custom rules
        if (language === 'cpp') {
            const allRules = this.service.getAllPairingRules();
            const languageRules = allRules.filter((rule: PairingRule) => rule.language === language);

            if (languageRules.length > 0) {
                // Use existing flow for custom rules
                const result = await this.selectFromCustomRules(allRules, language);
                if (result === null || result === undefined) return undefined; // User cancelled
                if (result === 'use_default') {
                    // Continue to default template selection
                } else if (result) {
                    return result;
                }
            }
        }

        // New improved flow: Choose template type first (C/C++ language and type)
        const templateChoice = await this.promptForTemplateTypeFirst(language, uncertain);
        if (!templateChoice) return undefined;

        // If C++ template was selected and no custom rules exist, ask for extensions
        if (templateChoice.language === 'cpp') {
            const allRules = this.service.getAllPairingRules();
            const cppRules = allRules.filter((rule: PairingRule) => rule.language === 'cpp');

            if (cppRules.length === 0) {
                // No custom C++ rules, let user choose extensions
                const extensionChoice = await this.promptForFileExtensions();
                if (!extensionChoice) return undefined;

                // Apply the chosen extensions to the template
                return {
                    ...templateChoice,
                    key: `${templateChoice.key}_custom`,
                    headerExt: extensionChoice.headerExt,
                    sourceExt: extensionChoice.sourceExt,
                    description: templateChoice.description.replace(/\.h\/\.cpp/g,
                        `${extensionChoice.headerExt}/${extensionChoice.sourceExt}`)
                };
            }
        }

        return templateChoice;
    }

    /**
     * First step: Choose template type (language and template kind)
     * Shows both C and C++ options regardless of detected language
     */
    private async promptForTemplateTypeFirst(language: 'c' | 'cpp', uncertain: boolean): Promise<PairingRule | undefined> {
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

    /**
     * New improved flow: First choose file extensions, then template type
     * This provides better UX by letting users see their choice immediately
     */
    private async promptForExtensionsAndTemplate(language: 'cpp'): Promise<PairingRule | undefined> {
        // Step 1: Choose file extensions
        const extensionChoice = await this.promptForFileExtensions();
        if (!extensionChoice) return undefined;

        // Step 2: Choose template type with the selected extensions
        const templateChoice = await this.promptForTemplateType(extensionChoice);
        return templateChoice;
    }

    /**
     * Let user choose file extensions (.h/.cpp, .hh/.cc, etc.)
     */
    private async promptForFileExtensions(): Promise<{ headerExt: string, sourceExt: string } | undefined> {
        const extensionOptions = [
            {
                label: '$(file-code) .h / .cpp',
                description: 'Standard C++ extensions (most common)',
                detail: 'Widely used, compatible with most tools and IDEs',
                headerExt: '.h',
                sourceExt: '.cpp'
            },
            {
                label: '$(file-code) .hh / .cc',
                description: 'Alternative C++ extensions',
                detail: 'Used by Google style guide and some projects',
                headerExt: '.hh',
                sourceExt: '.cc'
            },
            {
                label: '$(file-code) .hpp / .cpp',
                description: 'Header Plus Plus style',
                detail: 'Explicitly indicates C++ headers',
                headerExt: '.hpp',
                sourceExt: '.cpp'
            },
            {
                label: '$(file-code) .hxx / .cxx',
                description: 'Extended C++ extensions',
                detail: 'Less common but explicit C++ indicator',
                headerExt: '.hxx',
                sourceExt: '.cxx'
            }
        ];

        const selected = await vscode.window.showQuickPick(extensionOptions, {
            placeHolder: 'Choose file extensions for your C++ files',
            title: 'Step 1 of 2: Select File Extensions',
            matchOnDescription: true,
            matchOnDetail: true
        });

        return selected ? { headerExt: selected.headerExt, sourceExt: selected.sourceExt } : undefined;
    }

    /**
     * Let user choose template type (Class, Struct, Empty) with selected extensions
     */
    private async promptForTemplateType(extensions: { headerExt: string, sourceExt: string }): Promise<PairingRule | undefined> {
        const templateOptions: PairingRule[] = [
            {
                key: 'cpp_empty_custom',
                label: `$(new-file) C++ Pair`,
                description: `Creates ${extensions.headerExt}/${extensions.sourceExt} with header guards`,
                language: 'cpp',
                headerExt: extensions.headerExt,
                sourceExt: extensions.sourceExt
            },
            {
                key: 'cpp_class_custom',
                label: `$(symbol-class) C++ Class`,
                description: `Creates ${extensions.headerExt}/${extensions.sourceExt} with class template`,
                language: 'cpp',
                headerExt: extensions.headerExt,
                sourceExt: extensions.sourceExt,
                isClass: true
            },
            {
                key: 'cpp_struct_custom',
                label: `$(symbol-struct) C++ Struct`,
                description: `Creates ${extensions.headerExt}/${extensions.sourceExt} with struct template`,
                language: 'cpp',
                headerExt: extensions.headerExt,
                sourceExt: extensions.sourceExt,
                isStruct: true
            }
        ];

        return vscode.window.showQuickPick(templateOptions, {
            placeHolder: `Choose template type for ${extensions.headerExt}/${extensions.sourceExt} files`,
            title: 'Step 2 of 2: Select Template Type'
        });
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
        try {
            const document = await vscode.workspace.openTextDocument(headerPath);

            // Use setTimeout to make this non-blocking and avoid hanging
            setTimeout(async () => {
                try {
                    await vscode.window.showTextDocument(document);
                } catch (error) {
                    // Silently handle file opening errors
                }
            }, 100);

            // Make success message non-blocking by not awaiting it
            vscode.window.showInformationMessage(
                `Successfully created ${path.basename(headerPath.fsPath)} and ${path.basename(sourcePath.fsPath)}.`);
        } catch (error) {
            // Still show success message even if file opening fails
            vscode.window.showInformationMessage(
                `Successfully created ${path.basename(headerPath.fsPath)} and ${path.basename(sourcePath.fsPath)}.`);
        }
    }
}
