//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// key.h --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:39:05 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Oct  7 08:14:47 2023
// Update Count     : 104
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


#pragma once


struct keyword_t {
	const char *text;
	int value;
};

extern keyword_t key[];

enum key_value_t {
	// Operators

	EQ = 256,											// ==
	NE,													// !=
	LE,													// <=
	GE,													// >=

	PLUS_ASSIGN,										// +=
	MINUS_ASSIGN,										// -=
	LSH_ASSIGN,											// <<=
	RSH_ASSIGN,											// >>=
	AND_ASSIGN,											// &=
	XOR_ASSIGN,											// ^=
	OR_ASSIGN,											// |=
	MULTIPLY_ASSIGN,									// *=
	DIVIDE_ASSIGN,										// /=
	MODULUS_ASSIGN,										// %=

	AND_AND,											// &&
	OR_OR,												// ||
	PLUS_PLUS,											// ++
	MINUS_MINUS,										// --
	RSH,												// >>
	LSH,												// <<
	GMIN,												// <? (min) gcc specific, deprecated
	GMAX,												// >? (max) gcc specific, deprecated

	ARROW,												// ->
	ARROW_STAR,											// ->*
	DOT_STAR,											// ->.

	CHARACTER,											// 'a'
	STRING,												// "abc"
	NUMBER,												// integer (oct,dec,hex) and floating-point constants

	IDENTIFIER,											// variable names
	STRING_IDENTIFIER,									// variable names, "abc"abc
	LABEL,												// statement labels
	TYPE,												// builtin and user defined types

	DOT_DOT,											// meta, intermediate parsing state
	DOT_DOT_DOT,										// ...

	COLON_COLON,										// ::

	ERROR,												// meta, error mesage
	WARNING,											// meta, warning message
	CODE,												// meta, generated code

	// Keywords

	ASM = 512,
	ATTRIBUTE,											// gcc specific
	BOOL,
	BREAK,
	CASE,
	CATCH,
	CHAR,
	CHAR16_t,
	CHAR32_t,
	CLASS,
	COMPLEX,											// gcc/c99 specific
	CONST,
	CONST_CAST,
	CONTINUE,
	DEFAULT,
	DELETE,
	DO,
	DOUBLE,
	DYNAMIC_CAST,
	ELSE,
	ENUM,
	EXPLICIT,
	EXPORT,
	EXTENSION,											// gcc specific
	EXTERN,
	FALSE,
	FLOAT,
	FOR,
	FRIEND,
	GOTO,
	IF,
	INLINE,
	INT,
	LONG,
	MUTABLE,
	NAMESPACE,
	NEW,
	OPERATOR,
	OVERRIDE,
	PRIVATE,
	PROTECTED,
	PUBLIC,
	REGISTER,
	REINTERPRET_CAST,
	RESTRICT,
	RETURN,
	SHORT,
	SIGNED,
	SIZEOF,
	STATIC,
	STATIC_CAST,
	STRUCT,
	SWITCH,
	TEMPLATE,
	THIS,
	THREAD,
	THROW,
	TRUE,
	TRY,
	TYPEDEF,
	TYPEOF,												// gcc specific
	TYPEID,
	TYPENAME,
	UNDERLYING_TYPE,									// gcc specific
	UNION,
	UNSIGNED,
	USING,
	VIRTUAL,
	VOID,
	VOLATILE,
	WCHAR_T,
	WHILE,

	// uC++

	ACCEPT,
	ACCEPTRETURN,
	ACCEPTWAIT,
	ACTOR,
	AT,
	CATCHRESUME,
	CORACTOR,
	COROUTINE,
	DISABLE,
	UELSE,
	ENABLE,
	EXCEPTION,
	FINALLY,
	MUTEX,
	NOMUTEX,
	PTASK,
	RESUME,
	RESUMETOP,
	RTASK,
	SELECT,
	STASK,
	TASK,
	UTHROW,
	TIMEOUT,
	WITH,
	WHEN,

	// C++11

	ALIGNAS,											// C++11
	ALIGNOF,											// C++11
	ATOMIC,												// C11
	AUTO,												// C++11
	CONSTEXPR,											// C++11
	DECLTYPE,											// C++11
	FINAL,												// C++11
	NOEXCEPT,											// C++11
	nullptrPTR,											// C++11
	STATIC_ASSERT,										// C++11
	THREAD_LOCAL,										// C++11

	CONN_OR,											// pseudo values (i.e., no associated keywords) denotes kinds of definitions
	CONN_AND,
	SELECT_LP,
	SELECT_RP,
	MEMBER,
	ROUTINE,
	TEMPLATEVAR,
};


// Local Variables: //
// compile-command: "make install" //
// End: //
