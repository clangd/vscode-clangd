//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// key.c --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:06:35 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Aug 22 10:43:12 2024
// Update Count     : 132
//
// This  library is free  software; you  can redistribute  it and/or  modify it
// under the terms of the GNU Lesser General Public License as published by the
// Free Software  Foundation; either  version 2.1 of  the License, or  (at your
// option) any later version.
//
// This library is distributed in the  hope that it will be useful, but WITHOUT
// ANY  WARRANTY;  without even  the  implied  warranty  of MERCHANTABILITY  or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
// for more details.
//
// You should  have received a  copy of the  GNU Lesser General  Public License
// along  with this library.
//

#include "key.h"

keyword_t key[] = {
	{ "asm", ASM },
	{ "__asm", ASM },									// gcc specific
	{ "__asm__", ASM },									// gcc specific
	{ "__attribute", ATTRIBUTE },						// gcc specific
	{ "__attribute__", ATTRIBUTE },						// gcc specific
	{ "bool", BOOL },
	{ "break", BREAK },
	{ "case", CASE },
	{ "catch", CATCH },
	{ "char", CHAR },
	{ "char16_t", CHAR16_t },
	{ "char32_t", CHAR32_t },
	{ "class", CLASS },
	{ "__complex__", COMPLEX },							// gcc specific
	{ "_Complex", COMPLEX },							// c99 specific
	{ "const", CONST },
	{ "__const", CONST },								// gcc specific
	{ "__const__", CONST },								// gcc specific
	{ "const_cast", CONST_CAST },
	{ "continue", CONTINUE },
	{ "default", DEFAULT },
	{ "delete", DELETE },
	{ "do", DO },
	{ "double", DOUBLE },
	{ "dynamic_cast", DYNAMIC_CAST },
	{ "else", ELSE },
	{ "enum", ENUM },
	{ "explicit", EXPLICIT },
	{ "export", EXPORT },
	{ "__extension__", EXTENSION },						// gcc specific
	{ "extern", EXTERN },
	{ "false", FALSE },
	{ "float", FLOAT },
	{ "for", FOR },
	{ "friend", FRIEND },
	{ "goto", GOTO },
	{ "if", IF },
	{ "inline", INLINE },
	{ "__inline", INLINE },								// gcc specific
	{ "__inline__", INLINE },							// gcc specific
	{ "__int128", INT },								// gcc specific
	{ "int", INT },
	{ "long", LONG },
	{ "mutable", MUTABLE },
	{ "namespace", NAMESPACE },
	{ "new", NEW },
	{ "operator", OPERATOR },
	{ "private", PRIVATE },
	{ "protected", PROTECTED },
	{ "public", PUBLIC },
	{ "register", REGISTER },
	{ "reinterpret_cast", REINTERPRET_CAST },
	{ "restrict", RESTRICT },
	{ "__restrict", RESTRICT },
	{ "__restrict__", RESTRICT },
	{ "return", RETURN },
	{ "short", SHORT },
	{ "signed", SIGNED },
	{ "__signed", SIGNED },								// gcc specific
	{ "__signed__", SIGNED },							// gcc specific
	{ "sizeof", SIZEOF },
	{ "static", STATIC },
	{ "static_cast", STATIC_CAST },
	{ "struct", STRUCT },
	{ "switch", SWITCH },
	{ "template", TEMPLATE },
	{ "this", THIS },
	{ "__thread", THREAD },
	{ "throw", THROW },
	{ "true", TRUE },
	{ "try", TRY },
	{ "typedef", TYPEDEF },
	{ "typeof", TYPEOF },								// gcc specific
	{ "__typeof", TYPEOF },								// gcc specific
	{ "__typeof__", TYPEOF },							// gcc specific
	{ "typeid", TYPEID },
	{ "typename", TYPENAME },
	{ "__underlying_type", UNDERLYING_TYPE },			// gcc specific
	{ "union", UNION },
	{ "unsigned", UNSIGNED },
	{ "using", USING },
	{ "virtual", VIRTUAL },
	{ "void", VOID },
	{ "volatile", VOLATILE },
	{ "__volatile", VOLATILE },							// gcc specific
	{ "__volatile__", VOLATILE },						// gcc specific
	{ "wchar_t", WCHAR_T },
	{ "__wchar_t", WCHAR_T },							// gcc specific
	{ "while", WHILE },

	{ "and", AND_AND },									// alternate operator names
	{ "and_eq", AND_ASSIGN },
	{ "bitand", '&' },
	{ "bitor", '|' },
	{ "compl", '~' },
	{ "not", '!' },
	{ "not_eq", NE },
	{ "or", OR_OR },
	{ "or_eq", OR_ASSIGN },
	{ "xor", '^' },
	{ "xor_eq", XOR_ASSIGN },

	// uC++

	{ "_Accept", ACCEPT },
	{ "_AcceptReturn", ACCEPTRETURN },
	{ "_AcceptWait", ACCEPTWAIT },
	{ "_Actor", ACTOR },
	{ "_At", AT },
	{ "_Catch", CATCH },
	{ "_CatchResume", CATCHRESUME },
	{ "_CorActor", CORACTOR },
	{ "_Coroutine", COROUTINE },
	{ "_Disable", DISABLE },
	{ "_Else", UELSE },
	{ "_Enable", ENABLE },
	{ "_Event", EXCEPTION },							// deprecated
	{ "_Exception", EXCEPTION },
	{ "_Finally", FINALLY },
	{ "_Mutex", MUTEX },
	{ "_Nomutex", NOMUTEX },
	{ "_PeriodicTask", PTASK },
	{ "_RealTimeTask", RTASK },
	{ "_Resume", RESUME },
	{ "_ResumeTop", RESUMETOP },
	{ "_Select", SELECT },
	{ "_SporadicTask", STASK },
	{ "_Task", TASK },
	{ "_Throw", UTHROW },
	{ "_Timeout", TIMEOUT },
	{ "_When", WHEN },
	{ "_With", WITH },
	{ nullptr, 0 },

	// C++11

	{ "alignas", ALIGNAS },								// C++11
	{ "alignof", ALIGNOF },								// C++11
	{ "_Atomic", ATOMIC },								// C11
	{ "auto", AUTO },									// C++11
	{ "constexpr", CONSTEXPR },							// C++11
	{ "decltype", DECLTYPE },							// C++11
	{ "__decltype", DECLTYPE },							// g++ builtin
	{ "final", FINAL },									// C++11
	{ "noexcept", NOEXCEPT },							// C++11
	{ "nullptr", nullptrPTR },							// C++11
	{ "override", OVERRIDE },							// C++11
	{ "static_assert", STATIC_ASSERT },					// C++11
	{ "thread_local", THREAD_LOCAL },					// C++11
	{ nullptr, 0 }
};

// Local Variables: //
// compile-command: "make install" //
// End: //
