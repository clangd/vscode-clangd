//
// TEMPLATES AND CONSTANTS
// =======================
//
// This module contains all the hardcoded data for file pair creation:
// - Template rules for different file types
// - File content templates with placeholders
// - Default placeholder names
// - Validation patterns
//

import {PairingRule} from '../pairing-rule-manager';

// Types for better type safety
export type Language = 'c'|'cpp';
export type TemplateKey =
    'CPP_CLASS'|'CPP_STRUCT'|'C_STRUCT'|'C_EMPTY'|'CPP_EMPTY';

// Regular expression patterns to validate C/C++ identifiers
export const VALIDATION_PATTERNS = {
  IDENTIFIER: /^[a-zA-Z_][a-zA-Z0-9_]*$/
};

// Default placeholder names for different file types
export const DEFAULT_PLACEHOLDERS = {
  C_EMPTY: 'my_c_functions',
  C_STRUCT: 'MyStruct',
  CPP_EMPTY: 'utils',
  CPP_CLASS: 'MyClass',
  CPP_STRUCT: 'MyStruct'
};

// Template rules for available file pair types
export const TEMPLATE_RULES: PairingRule[] = [
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
export const FILE_TEMPLATES = {
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
