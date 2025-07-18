//
// CREATE SOURCE HEADER PAIR - MODULE INDEX
// ========================================
//
// This module provides functionality to create matching header/source file pairs
// for C/C++ development. It intelligently detects language context, offers
// appropriate templates, and handles custom file extensions.
//
// ARCHITECTURE:
// - templates.ts: Template rules and file content templates
// - service.ts: Business logic layer (language detection, file operations)
// - ui.ts: User interface layer (dialogs, input validation)
// - coordinator.ts: Main coordinator (orchestrates workflow, registers commands)
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

import { ClangdContext } from '../clangd-context';
import { PairCoordinator } from './coordinator';
import { PairCreatorService } from './service';
import { PairCreatorUI } from './ui';

// Registers the create source/header pair command with the VS Code extension context
// Uses dependency injection to create properly configured instances
export function registerCreateSourceHeaderPairCommand(context: ClangdContext) {
    // Create instances with proper dependencies
    const service = new PairCreatorService();
    const ui = new PairCreatorUI(service);
    const coordinator = new PairCoordinator(service, ui);

    context.subscriptions.push(coordinator);
}

// Re-export main types and classes for external usage
export { PairCoordinator } from './coordinator';
export { PairCreatorService } from './service';
export { PairCreatorUI } from './ui';
export { Language, TemplateKey } from './templates';
