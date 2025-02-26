//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// parse.c --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:10:34 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Jan  2 15:38:48 2025
// Update Count     : 5145
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

#include <cstdio>					// sprintf

#include "uassert.h"
#include "main.h"
#include "key.h"
#include "hash.h"
#include "attribute.h"
#include "symbol.h"
#include "structor.h"
#include "token.h"
#include "table.h"
#include "scan.h"
#include "parse.h"
#include "gen.h"

#include <climits>										// UINT_MAX
#include <cstring>										// strcat, strlen, strcmp
#include <set>

//#define __U_DEBUG_H__
#include "debug.h"

#include <iostream>
using namespace std;

uDEBUGPRT(
	static void print_focus_change( const char * name, table_t * from, table_t * to ) {
		cerr << " top:" << top << " (" << (top->tbl->symbol != nullptr ? top->tbl->symbol->hash->text : (top->tbl == root) ? "root" : "template/compound") << ") " <<
		    name << " change focus from:";
		if ( from != nullptr ) {
		    cerr << from << " (" << (from->symbol != nullptr ? from->symbol->hash->text : (from == root) ? "root" : "template/compound") << ")";
		} else {
		    cerr << "nullptr";
		} // if
		cerr << " to ";
		if ( to != nullptr ) {
		    cerr << to << " (" << (to->symbol != nullptr ? to->symbol->hash->text : (to == root) ? "root" : "template/compound") << ")";
		} else {
		    cerr << "nullptr";
		} // if
		cerr << endl;
	} // print_focus_change
)


#define LC '{'
#define RC '}'

#define LP '('
#define RP ')'

#define LB '['
#define RB ']'

#define LA '<'
#define RA '>'

#define ABSTRACT ((token_t *)1)


static void move_tokens( token_t * to, token_t * start, token_t * end ) {
	token_t * next;
	for ( token_t * p = start; p != end; p = next ) {	// move tokens
		next = p->next_parse_token();
		p->remove_token();
		p->add_token_before( *to );
	} // for
} // move_tokens


static void move_tokens_all( token_t * to, token_t * start, token_t * end ) {
	token_t * next;
	for ( token_t * p = start; p != end; p = next ) {	// move tokens
		next = p->fore;
		p->remove_token();
		p->add_token_before( *to );
	} // for
} // move_tokens


static void copy_tokens( token_t * to, token_t * start, token_t * end ) {
	token_t * next;
	for ( token_t * p = start; p != end; p = next ) {	// copy tokens
		next = p->next_parse_token();
		token_t * token = new token_t( *p );
		token->add_token_before( *to );
	} // for
} // copy_tokens


static const char * kind( symbol_t * symbol ) {
	if ( ( symbol->data->key == STRUCT || symbol->data->key == CLASS || symbol->data->key == UNION ) && ! symbol->data->attribute.Mutex ) {
		if ( symbol->data->key == STRUCT ) return "STRUCT";
		else if ( symbol->data->key == CLASS ) return "CLASS";
		else return "UNION";
	} else if ( ( symbol->data->key == STRUCT || symbol->data->key == CLASS ) && symbol->data->attribute.Mutex ) {
		return "MONITOR";
	} else if ( symbol->data->key == COROUTINE ) {
		if ( ! symbol->data->attribute.Mutex ) return "COROUTINE";
		else return "CORMONITOR";
	} else if ( symbol->data->key == TASK ) {
		return "TASK";
	} else if ( symbol->data->key == EXCEPTION ) {
		return "EXCEPTION";
	} else if ( symbol->data->key == ACTOR ) {
		return "ACTOR";
	} else if ( symbol->data->key == CORACTOR ) {
		return "CORACTOR";
	} else {
		return "*UNKNOWN*";
	} // if
} // kind


static bool eof() {
	return ahead->value == EOF;
} // eof


static bool check( int token ) {
	return token == ahead->value;
} // check


static bool match( int token ) {
	if ( check( token ) ) {
		scan();
		return true;
	} // if
	return false;
} // match


static token_t * type();

/*
Closing template delimiter: if the nearest opening bracket is a '<', then the token '>>' means the same as the two tokens '>' '>'.

Q<x<y,z>> // well-formed
Q<6>>1>   // ill-formed
Q<(6>>1)> // well-formed

Places match_closing is used; i.e., places where uC++ is uninterested in program source.

condition
	(...)  // if, switch, while, for, _When, _Timeout, _Select, member_initializer
static_assert_declaration
	static_assert(...)
event_handler
	_Enable< template<...> >
template_parameter
	template<template<...> class C>
	template<typename T = ...>
	template<int i = ...>
using
	using __rebind = typename _Tp::template rebind<_Ptr>;
task_parameter_list
	_Task<...>
mutex_parameter_list
	_Mutex<...> class
asm_clause
	void f() asm(...)
throw_clause
	void f() throw(...)
noexcept_clause
	void f() noexcept(...)
array_dimension
	int x[...]
more_declarator
	int (*fp)(...)
member_initializer
	ctor() : member{ ... }
initializer
	x = ... ;
	x = ... ,
	x(...)
	x{...}
bound_exception_declaration
	catch( ... )  // and then reparse catch argument
asm_declaration
	asm(...)
*/

static bool match_closing( char left, char right, bool semicolon = false, bool LessThan = false, bool Type = false ) {
	// assume starting character, "left", is already matched

	// ahead is one token after "left", so backup to the symbol before "left".
	token_t * prev2 = ahead->prev_parse_token()->prev_parse_token();

	// If "left" is not '<', continue looking for "right".
	// If "left" is '<', check kind of symbol before "left" to see if it binds with the '<':
	//    T<, template<, operator S<, _Mutex<, _Task T<, const_cast<, dynamic_cast<, reinterpret_cast<,
	//    static_cast<, id< where id is member/routine
	// For these cases, the '<' is a template delimiter not a less-than operator.
	if ( left != LA || prev2->value == TYPE || Type || prev2->value == TEMPLATE || prev2->prev_parse_token()->value == OPERATOR ||
		 prev2->value == MUTEX || prev2->prev_parse_token()->value == TASK ||
		 prev2->value == CONST_CAST || prev2->value == DYNAMIC_CAST || prev2->value == REINTERPRET_CAST || prev2->value == STATIC_CAST ||
		 ( prev2->value == IDENTIFIER && prev2->symbol != nullptr &&
		   // This check may generate false positives because the translator does not always find all declarations.
		   ( prev2->prev_parse_token()->value == TEMPLATE || prev2->symbol->data->key == TEMPLATEVAR || prev2->symbol->data->key == MEMBER || prev2->symbol->data->key == ROUTINE ) ) ) {
		for ( ;; ) {
		  if ( eof() ) break;
			if ( ! semicolon && check( ';' ) ) break;	// stop at end of statement (except "for" control clause and block/expression "{...)")
			if ( match( right ) ) {			// find correct closing delimiter ?
				return true;
				// If the next token is NOT the correct closing => mismatched closing => something is wrong with the nesting
				// of matching pairs.
			} else if ( match( RP ) || match( RC ) || match( RB ) ) {
				return left == LA ? LessThan : false;	// if '<' => less-than and not part of nesting
				// if token is '>' => greater-than
			} else if ( match( LP ) ) {					// check nested delimiters
				if ( ! match_closing( LP, RP ) ) break;
			} else if ( match( LC ) ) {
				if ( ! match_closing( LC, RC, true ) ) break;
			} else if ( match( LB ) ) {
				if ( ! match_closing( LB, RB ) ) break;
			} else if ( match( LA ) ) {
				if ( ! match_closing( LA, RA, false, true, Type ) ) break; // set LessThan to true
			} else if ( stdcpp11 && left == LA && check( RSH ) ) { // C++11 allows right-shift as closing delimiter for nested template
				token_t * curr = ahead;
				ahead = ahead->prev_parse_token();		// backup before >>
				curr->remove_token();					// remove >>
				delete curr;
				curr = new token_t( '>', hash_table->lookup( ">" ) ); // replace with two '>'
				curr->add_token_after( * ahead );
				curr = new token_t( '>', hash_table->lookup( ">" ) );
				curr->add_token_after( * ahead );
				ahead = ahead->next_parse_token();		// advance to LA
			} else {
				// must correctly parse type-names to allow the above check
				if ( left != LA || type() == nullptr ) {
					scan();								// always executed
				} // if
				// SKULLDUGGERY: logically make the identifier a type as it might have template parameters
				//	T1<typename T2::template identifier<int>>
				// and pass the logical type to next recursive call
				if ( ahead->value == IDENTIFIER && ahead->prev_parse_token()->value == TEMPLATE ) {
					Type = true;
				} // if
			} // if
		} // for
	} else {
		return LessThan;								// => '<' is less-than operator
	} // if
	return false;
} // match_closing


static bool template_key() {
	token_t * back = ahead;

	if ( check( LA ) ) {
		match( LA );
		if ( match_closing( LA, RA ) ) {
			return true;
		} // if
	} // if

	ahead = back; return false;
} // template_key


// nested-name-specifier:
//   class-or-namespace-name :: nested-name-specifier_opt
//   class-or-namespace-name :: "template" nested-name-specifier
//
// class-or-namespace-name:
//   class-name
//   namespace-name

static bool typeof_specifier();

bool templateSpecial = false;

static token_t * nested_name_specifier() {
	token_t * back = ahead;
	token_t * token = nullptr;
	token_t * prev = nullptr;

	for ( ;; ) {
	  if ( eof() ) break;
		if ( check( TYPE ) ) {
			// keep track of the previous "type" because the lookahead needs to see the last type but it is not part of
			// the nested-name-specifier.
			prev = token;
			token = ahead;
			// reset the focus of the scanner to the top table as the next symbol may not be in this focus.
			uDEBUGPRT( print_focus_change( "nested_name_specifier2", focus, top->tbl ); )
			focus = top->tbl;
			match( TYPE );
			// if the last item of a name is a template key => template specialization
			if ( template_key() ) templateSpecial = true;

			uassert( token->symbol != nullptr );
			uassert( token->symbol->data != nullptr );
			if ( check( COLON_COLON ) && ( token->symbol->typname || ( token->symbol->data != nullptr && token->symbol->data->key != 0 ) ) ) { // type must have substructure
				if ( token->symbol->data->key != 0 ) {
					uDEBUGPRT( print_focus_change( "nested_name_specifier3", focus, token->symbol->data->table ); )
					focus = token->symbol->data->table;
					match( COLON_COLON );
					match( TEMPLATE );
				} else {
					match( COLON_COLON );
				} // if
			} else {
				if ( prev == nullptr ) break;			// no "::" found
				// backup to the previous type, which is the last type that is part of the nested-name-specifier, i.e.,
				// the last type *before* the last "::".
				focus = prev->symbol->data->table;
				ahead = token;
				return prev;
			} // if
		} else {
			if ( token == nullptr ) break;				// handle 1st case of no type
			return token;								// otherwise return last type (not prev)
		} // if
	} // for

	//uDEBUGPRT( print_focus_change( "nested_name_specifier4", focus, top->tbl ); )
	focus = top->tbl;
	ahead = back; return nullptr;
} // nested_name_specifier


static token_t * type() {
	token_t * back = ahead;
	token_t * token;

	if ( check( COLON_COLON ) ) {						// start search at the root
		uDEBUGPRT( print_focus_change( "type1", focus, root ); )
		focus = root;
		match( COLON_COLON );
	} // if

	nested_name_specifier();

	if ( check( TYPE ) ) {
		token = ahead;
		uDEBUGPRT( print_focus_change( "type2", focus, top->tbl ); )
		focus = top->tbl;
		match( TYPE );
		template_key();
		return token;
	} // if

	//uDEBUGPRT( print_focus_change( "type4", focus, top->tbl ); )
	focus = top->tbl;
	ahead = back; return nullptr;
} // type


static token_t * epyt() {								// destructor
	token_t * back = ahead;
	token_t * token;

	if ( check( COLON_COLON ) ) {						// start search at the root
		uDEBUGPRT( print_focus_change( "epyt1", focus, root ); )
		focus = root;
		match( COLON_COLON );
	} // if

	nested_name_specifier();

	if ( match( '~' ) && check( TYPE ) ) {
		token = ahead;
		focus = top->tbl;
		match( TYPE );
		return token;
	} // if

	//uDEBUGPRT( print_focus_change( "epyt5", focus, top->tbl ); )
	focus = top->tbl;
	ahead = back; return nullptr;
} // epyt


static void make_type( token_t * token, symbol_t * & symbol, table_t * locn = focus );

static token_t * identifier() {
	token_t * back = ahead;
	token_t * token;

	if ( check( COLON_COLON ) ) {						// start search at the root
		uDEBUGPRT( print_focus_change( "identifier1", focus, root ); )
		focus = root;
		match( COLON_COLON );
	} // if

	nested_name_specifier();

	if ( check( IDENTIFIER ) ) {
		token = ahead;
		uDEBUGPRT( print_focus_change( "identifier2", focus, top->tbl ); )
		focus = top->tbl;
		match( IDENTIFIER );
		// if this identifier has no symbol table entry associated with it, make one
		uassert( token != nullptr );
		if ( token->symbol == nullptr ) {
			token->symbol = new symbol_t( token->value, token->hash );
		} // if
		if ( strcmp( token->hash->text, "__make_integer_seq" ) == 0 ) { // clang builtin template
			make_type( token, token->symbol );
		} // if
		if ( strcmp( token->hash->text, "__type_pack_element" ) == 0 ) { // g++ builtin template
			make_type( token, token->symbol );
		} // if
		template_key();									// optional
		return token;
	} // if

	//uDEBUGPRT( print_focus_change( "identifier4", focus, top->tbl ); )
	focus = top->tbl;
	ahead = back; return nullptr;
} // identifier


// operator: one of
//   new delete new[] delete[]
//   + - * / % ^ & | ~
//   ! = < > += -= *= /= %=
//   ^= &= |= << >> >>= <<= == !=
//   <= >= && || ++ -- , ->* ->
//   >? <?  g++
//   () []

static token_t * op() {
	token_t * back = ahead;

	// "operator new" and "operator new[]" are treated as "new", similarly for "delete".

	if ( match( NEW ) ) {
		if ( match( LB ) ) {
			if ( match( RB ) ) return back;
		} else return back;
	} // if
	if ( match( DELETE ) ) {
		if ( match( LB ) ) {
			if ( match( RB ) ) return back;
		} else return back;
	} // if
	if ( match( '+' ) ) return back;
	if ( match( '-' ) ) return back;
	if ( match( '*' ) ) return back;
	if ( match( '/' ) ) return back;
	if ( match( '%' ) ) return back;
	if ( match( '^' ) ) return back;
	if ( match( '&' ) ) return back;
	if ( match( '|' ) ) return back;
	if ( match( '~' ) ) return back;
	if ( match( '!' ) ) return back;
	if ( match( '=' ) ) return back;
	if ( match( '<' ) ) return back;
	if ( match( '>' ) ) return back;
	if ( match( PLUS_ASSIGN ) ) return back;
	if ( match( MINUS_ASSIGN ) ) return back;
	if ( match( MULTIPLY_ASSIGN ) ) return back;
	if ( match( DIVIDE_ASSIGN ) ) return back;
	if ( match( MODULUS_ASSIGN ) ) return back;
	if ( match( XOR_ASSIGN ) ) return back;
	if ( match( AND_ASSIGN ) ) return back;
	if ( match( OR_ASSIGN ) ) return back;
	if ( match( LSH ) ) return back;
	if ( match( RSH ) ) return back;
	if ( match( LSH_ASSIGN ) ) return back;
	if ( match( RSH_ASSIGN ) ) return back;
	if ( match( EQ ) ) return back;
	if ( match( NE ) ) return back;
	if ( match( LE ) ) return back;
	if ( match( GE ) ) return back;
	if ( match( AND_AND ) ) return back;
	if ( match( OR_OR ) ) return back;
	if ( match( PLUS_PLUS ) ) return back;
	if ( match( MINUS_MINUS ) ) return back;
	if ( match( ',' ) ) return back;
	if ( match( ARROW_STAR ) ) return back;
	if ( match( ARROW ) ) return back;
	if ( match( GMAX ) ) return back;					// g++
	if ( match( GMIN ) ) return back;					// g++
	if ( match( STRING_IDENTIFIER ) ) return back;		// C++14 user defined literals

	// because the following operators are denoted by multiple tokens, but only should be represented with a single
	// token, they are parsed as multiple tokens, but only the first token is passsed back to identify them later

	if ( match( LP ) && match( RP ) ) return back;
	if ( match( LB ) && match( RB ) ) return back;

	ahead = back; return nullptr;
} // op


static bool specifier_list( attribute_t & attribute );
static bool ptr_operator();
static bool cv_qualifier_list();
static bool attribute_clause();
static bool attribute_clause_list();
static bool asm_clause();
static bool exception_list();


// conversion-function-id:
//   "operator" conversion-type-id
//
// conversion-type-id:
//   type-specifier-seq conversion-declarator_opt
//
// conversion-declarator:
//   ptr-operator conversion-declarator_opt

static token_t * operater() {
	token_t * back = ahead;
	token_t * token;

	if ( check( COLON_COLON ) ) {						// start search at the root
		uDEBUGPRT( print_focus_change( "operater1", focus, root ); )
		focus = root;
		match( COLON_COLON );
	} // if

	nested_name_specifier();

	if ( check( OPERATOR ) ) {
		token_t * start = ahead;
		focus = top->tbl;
		match( OPERATOR );
		if ( ( token = op() ) != nullptr ) {
			template_key();

			// if this operator has no symbol table entry associated with it, make one

			uassert( token != nullptr );
			if ( token->symbol == nullptr ) {
				token->symbol = new symbol_t( OPERATOR, token->hash );
			} // if

			// check if this is a fundamental assignment operator so the translator does not generate a null, private
			// assignment operator.

			if ( '=' == token->value && focus != nullptr && focus->symbol != nullptr ) {
				token_t * save = ahead;

				if ( match( LP ) ) {
					cv_qualifier_list();				// optional pre qualifiers
					token_t * temp = type();
					if ( temp != nullptr && temp->symbol == focus->symbol ) {
						cv_qualifier_list();			// optional post qualifiers
						if ( match( '&' ) ) {
							match( IDENTIFIER );		// optional identifier
							if ( match( RP ) ) {
								focus->hasassnop = true;
							} // if
						} // if
					} // if
				} // if
				ahead = save;
			} // if

			uDEBUGPRT( print_focus_change( "operater2", focus, top->tbl ); )
			return token;
		} // if

		attribute_t attribute;							// dummy

		if ( specifier_list( attribute ) ) {			// specifier_list CHANGES FOCUS
			while ( ptr_operator() );					// optional

			uDEBUGPRT( print_focus_change( "operater3", focus, top->tbl ); );
			focus = top->tbl;

			char name[256] = "\0";
			for ( token_t * p = start; p != ahead; ) {	// build a name for the conversion operator
				strcat( name, p->hash->text );
				token_t * c = p;
				p = p->next_parse_token();
				if ( p != ahead ) {
					strcat( name, " " );
				} // if
				c->remove_token();						// remove tokens forming the name
				delete c;
			} // for
			strcat( name, " " );						// add a blank for name mangling

			token = gen_code( ahead, name, TYPE );		// insert new token into the token stream
			token->symbol = focus->search_table( token->hash ); // look up token
			if ( token->symbol == nullptr ) {
				// if the symbol is not defined in the symbol table, define it
				token->symbol = new symbol_t( OPERATOR, token->hash );
			} else {
				// if the symbol is already defined but not in this scope => make one for this scope
				
				if ( token->symbol->data->found != focus ) {
					token->symbol = new symbol_t( OPERATOR, token->hash );
					focus->insert_table( token->symbol );
				} // if
			} // if

			// reset the scanner's focus and return the current token

			uDEBUGPRT( print_focus_change( "operater4", focus, top->tbl ); );
			return token;
		} // if
		uDEBUGPRT( print_focus_change( "operater5", focus, top->tbl ); );

		focus = top->tbl;
		match( IDENTIFIER );
		template_key();

		// if this identifier has no symbol table entry associated with it, make one
		uassert( token != nullptr );
		if ( token->symbol == nullptr ) {
			token->symbol = new symbol_t( token->value, token->hash );
		} // if
		return token;
	} // if

	//uDEBUGPRT( print_focus_change( "operater6", focus, top->tbl ); )
	focus = top->tbl;
	ahead = back; return nullptr;
} // operater


static bool condition( bool semicolon = false ) {
	token_t * back = ahead;

	if ( match( LP ) ) {
		if ( match_closing( LP, RP, semicolon ) ) {		// semicolon allowed in "for" control clause
			return true;
		} // if
	} // if

	ahead = back; return false;
} // condition


// https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html#Attribute-Syntax
// In GNU C, an attribute specifier list may appear after the colon following a label, other than a case or default
// label. GNU C++ only permits attributes on labels if the attribute specifier is immediately followed by a semicolon
// (i.e., the label applies to an empty statement). If the semicolon is missing, C++ label attributes are ambiguous, as
// it is permissible for a declaration, which could begin with an attribute list, to be labelled in C++. Declarations
// cannot be labelled in C90 or C99, so the ambiguity does not arise there.

static void prefix_labels( token_t * before, token_t * after, token_t * label[], int cnt, bool contlabels ) {
	if ( cnt > 0 ) {									// prefix labels ?
		gen_code( before, "{" );
		char name[300];
		token_t * l;
		if ( contlabels ) {								// continue labels ?
			for ( int i = 0; i < cnt; i += 1 ) {
				l = label[i];							// optimization
				if ( l != nullptr && l->symbol != nullptr ) { // ignore duplicate and non-prefix labels
					sprintf( name, "_U_C_%.256s : __attribute__ (( unused )) ;", l->hash->text );
					gen_code( after, name );
				} // if
			} // for
		} // if
		gen_code( after, "}" );
		for ( int i = 0; i < cnt; i += 1 ) {
			l = label[i];				// optimization
			if ( l != nullptr && l->symbol != nullptr ) { // ignore duplicate and non-prefix labels
				sprintf( name, "_U_B_%.256s : __attribute__ (( unused )) ;", l->hash->text );
				gen_code( ahead, name );
			} // if
		} // for
	} // if
} // prefix_labels


static bool statement( symbol_t * symbol );
static bool null_statement();
static bool compound_statement( symbol_t * symbol, token_t * label[], int cnt );


// selection-statement:
//   "if" "(" condition ")" statement
//   "if" "(" condition ")" statement else statement
//   "switch" "(" condition ")" statement

static bool if_statement( symbol_t * symbol, token_t * label[], int cnt ) {
	token_t * back = ahead;

	if ( match( IF ) && condition() ) {
		if ( statement( symbol ) ) {
			if ( match( ELSE ) ) {
				if ( statement( symbol ) ) {
					prefix_labels( back, ahead, label, cnt, false );
					return true;
				} // if
			} else {
				prefix_labels( back, ahead, label, cnt, false );
				return true;
			} // if
		} // if
	} // if

	ahead = back; return false;
} // if_statement


static bool switch_statement( symbol_t * symbol, token_t * label[], int cnt ) {
	token_t * back = ahead;

	if ( match( SWITCH ) && condition() ) {
		token_t * before = ahead;
		if ( statement( symbol ) ) {
			prefix_labels( before, ahead, label, cnt, false );
			return true;
		} // if
	} // if

	ahead = back; return false;
} // switch_statement


// iteration-statement:
//   "while" "(" condition ")" statement
//   "do" statement "while" "(" expression ")" ";"
//   "for" "(" for-init-statement condition_opt ; expression_opt ) statement
//
// for-init-statement:
//   expression-statement
//   simple-declaration

static bool while_statement( symbol_t * symbol, token_t * label[], int cnt ) {
	token_t * back = ahead;

	if ( match( WHILE ) && condition() ) {
		token_t * before = ahead;
		if ( statement( symbol ) ) {
			prefix_labels( before, ahead, label, cnt, true );
			return true;
		} // if
	} // if

	ahead = back; return false;
} // while_statement


static bool do_statement( symbol_t * symbol, token_t * label[], int cnt ) {
	token_t * back = ahead;

	if ( match( DO ) ) {
		token_t * before = ahead;
		if ( statement( symbol ) ) {
			token_t * after = ahead;
			if ( match( WHILE ) && condition() && match( ';' ) ) {
				prefix_labels( before, after, label, cnt, true );
				return true;
			} // if
		} // if
	} // if

	ahead = back; return false;
} // do_statement


static bool for_statement( symbol_t * symbol, token_t * label[], int cnt ) {
	token_t * back = ahead;

	if ( match( FOR ) && condition( true ) ) {			// parses all 3 expressions, separated by ';'
		token_t * before = ahead;
		if ( statement( symbol ) ) {
			prefix_labels( before, ahead, label, cnt, true );
			return true;
		} // if
	} // if

	ahead = back; return false;
} // for_statement


// jump-statement:
//   "break" label_opt ";"
//   "continue" label_opt ";"
//   "return" expression_opt ";"
//   "goto" identifier ";"

static bool break_statement() {
	token_t * back = ahead;
	token_t * blocn;

	blocn = ahead;										// remember start of break
	if ( match( BREAK ) ) {
		token_t * label = ahead;
		if ( match( IDENTIFIER ) ) {
			if ( label->symbol != nullptr ) {			// prefix label ?
				// cannot remove "break" token as there may be pointers to it
				blocn->hash = hash_table->lookup( "goto" ); // change to "goto" token
				blocn->value = blocn->hash->value;
				char name[300];				// generate new label
				sprintf( name, "_U_B_%.256s", label->hash->text );
				label->hash = hash_table->lookup( name ); // change label
			} else {
				gen_error( ahead, "label is not a prefix of a \"for\", \"while\", \"do\", \"switch\", \"if\", \"try\" or compound \"{}\" statement." );
			} // if
		} // if
		if ( match( ';' ) ) return true;				// must end with semi-colon
	} // if

	ahead = back; return false;
} // break_statement


// labelled continue


static bool continue_statement() {
	token_t * back = ahead;
	token_t * clocn;

	clocn = ahead;										// remember start of continue
	if ( match( CONTINUE ) ) {
		token_t * label = ahead;
		if ( match( IDENTIFIER ) ) {
			if ( label->symbol != nullptr && label->symbol->data->index == 1 ) { // prefix label ?
				// cannot remove "continue" token as there may be pointers to it
				clocn->hash = hash_table->lookup( "goto" ); // change to "goto" token
				clocn->value = clocn->hash->value;
				char name[300];				// generate new label
				sprintf( name, "_U_C_%.256s", label->hash->text );
				label->hash = hash_table->lookup( name ); // change label
			} else {
				gen_error( ahead, "label is not a prefix of a \"for\", \"while\" or \"do\" statement." );
			} // if
		} // if
		if ( match( ';' ) ) return true;				// must end with semi-colon
	} // if

	ahead = back; return false;
} // continue_statement


static bool static_assert_declaration() {
	token_t * back = ahead;

	if ( match( STATIC_ASSERT ) && match( LP ) && match_closing( LP, RP ) ) return true; // C++11

	ahead = back; return false;
} // static_assert_declaration


static bool wait_statement_body() {
	token_t * back = ahead;
	token_t * prefix, * suffix;

	prefix = ahead;										// remember where to put the prefix
	for ( ;; ) {										// look for the end of the statement
	  if ( eof() ) break;
		if ( match( WITH ) ) {
			suffix = ahead;
			gen_wait_prefix( prefix, suffix );
			prefix = ahead;
			for ( ;; ) {
			  if ( eof() ) break;
				if ( match( ';' ) ) {
					suffix = ahead->prev_parse_token();
					if ( prefix == suffix ) {
						gen_error( suffix, "integer value must follow _With." );
						goto fini;
					} // if
					gen_with( prefix, suffix );
					gen_wait_suffix( suffix );
					return true;
				} // if
				scan();
			} // for
		} // if
		if ( match( ';' ) ) {
			suffix = ahead->prev_parse_token();
			if ( prefix == suffix ) {
				gen_error( suffix, "condition value must follow _AcceptWait." );
				goto fini;
			} // if
			gen_wait_prefix( prefix, suffix );
			gen_wait_suffix( suffix );
			return true;
		} // if
		scan();
	} // for
  fini:
	ahead = back; return false;
} // wait_statement_body


// _Accept statement


static unsigned int accept_cnt = 0;						// count number of accept statements in translation unit

struct jump_t {
	unsigned int index;									// unique value of mutex member in class
	jump_t * link;
}; //jump_t


static void when_clause( token_t *& wstart, token_t *& wend ) {
	wstart = nullptr;									// null => no _When clause
	if ( match( WHEN ) ) {								// optional
		wstart = ahead;									// includes opening parenthesis but not _When
		if ( condition() ) {
			wend = ahead;								// includes closing parenthesis
		} else {
			// no reason to backtrack
			gen_error( wstart, "parenthesized conditional must follow _When." );
			wstart = nullptr;							// ignore ill-formed _When
		} // if
	} // if
} // when_clause


static void accept_start( token_t * before ) {
	gen_code( before, "{ UPP :: uSerial :: uProtectAcceptStmt uProtectAcceptStmtInstance ( this -> uSerialInstance" );
	// broken off from previous code fragment to allow subsequent insertion by timeout
	gen_code( before, ") ;" );
} // accept_start


static void accept_end( token_t * before, jump_t *& list, unsigned int & accept_no ) {
	char helpText[32];

	gen_code( before, "switch ( uProtectAcceptStmtInstance . mutexMaskPosn ) {" );
	for ( ; list != nullptr; ) {
		gen_code( before, "case" );
		gen_mask( before, list->index );
		sprintf( helpText, ": goto _U_A%04x_%04x ;", accept_no, list->index );
		gen_code( before, helpText );
		jump_t * curr = list;							// remove node
		list = list->link;
		delete curr;
	} // for
	gen_code( before, "}" );							// closing brace for switch
} // accept_end


static bool accept_mutex_list( jump_t *& list, symbol_t * symbol, const char * tryname = "acceptTry" ) {
	token_t * start;
	token_t * member;
	unsigned int index;

	start = ahead;
	gen_code( start, "this -> uSerialInstance ." );
	gen_code( start, tryname );
	gen_code( start, "(" );
	if ( ( member = identifier() ) != nullptr || ( member = operater() ) != nullptr || ( member = epyt() ) != nullptr ) {
		while ( start != ahead ) {			// remove the member name from the token stream as it is not printed
			token_t * dump = start;
			start = start->next_parse_token();
			dump->remove_token();
			delete dump;
		} // while

		symbol_t * msymbol = member->symbol;
		uassert( msymbol != nullptr );

		if ( msymbol->data->table != nullptr || msymbol->data->attribute.Mutex ) {
			if ( msymbol->data->table != nullptr ) {	// must be the destructor
				uassert( symbol->data->found != nullptr );
				// check destructor name is the same as the containing class
				if ( symbol->data->found->symbol != nullptr && // destructor may not be defined yet but its name can be used
					 symbol->data->found->symbol != msymbol ) {
					char msg[strlen(symbol->data->found->symbol->hash->text) + 256];

					sprintf( msg, "accepting an invalid destructor; destructor name must be the same as the containing class \"%s\".",
							 symbol->data->found->symbol->hash->text );
					gen_error( ahead, msg );
				} // if
				index = DESTRUCTORPOSN;					// kludge, if previous problem, prevent errors about exceeding maximum number of mutex members
			} else {									// members other than destructor
				index = msymbol->data->index;
			} // if

			// check if multiple accepts for this member in the accept statement

			for ( jump_t * p = list; p != nullptr; p = p->link ) {
				if ( index == p->index ) {
					char msg[strlen(msymbol->hash->text) + 256];
					sprintf( msg, "multiple accepts of mutex member \"%s\".", msymbol->hash->text );
					gen_error( ahead, msg );
					break;
				} // if
			} // for
		} else {
			index = DESTRUCTORPOSN;						// kludge to prevent errors about exceeding maximum number of mutex members
			char msg[strlen(msymbol->hash->text) + 256];
			sprintf( msg, "accept on a nomutex member \"%s\", possibly caused by accept statement appearing before mutex-member definition.", msymbol->hash->text );
			gen_error( ahead, msg );
		} // if

		jump_t * jump = new jump_t;						// create node
		jump->index = index;							// initialize node
		jump->link = list;								// add node to head of list
		list = jump;									// set top of list

		gen_code( ahead, "this ->" );					// must qualify with templates
		gen_entry( ahead, index );

		if ( check( ',' ) || check( OR_OR ) ) {			// separator ?
			if ( check( OR_OR ) ) { 					
				ahead->hash = hash_table->lookup( "," ); // change || to comma
				ahead->value = ',';
			} // if
			match( ',' );								// now match comma
			gen_mask( ahead, index );					// use existing comma to separate arguments for call to acceptTry
			gen_code( ahead, ") ||" );
			accept_mutex_list( list, symbol, tryname );
		} else if ( check( AND_AND ) ) {				// separator ?
			// no reason to backtrack
			gen_error( ahead, "unsupported && in _Accept clause." );
		} else {
			gen_code( ahead, "," );						// separate arguments for call to acceptTry
			gen_mask( ahead, index );

			if ( ! match( RP ) ) {						// end of mutex list ?
				// no reason to backtrack
				gen_error( ahead, "missing closing parenthesis for mutex-member list." );
			} // if
			gen_code( ahead, ") ) {" );					// brace starts the completion statement of the accept clause
		} // if
	} else {
		// no reason to backtrack
		gen_error( ahead, "mutex-member list must follow parenthesis." );
	} // if
	return true;
} // accept_mutex_list


static bool or_accept_clause( token_t * startposn, bool & uncondaccept, jump_t * & list, unsigned int & accept_no, symbol_t * symbol );

static int accept_clause( token_t * startposn, bool & uncondaccept, jump_t *& list, unsigned int & accept_no, symbol_t * symbol ) {
	token_t * back = ahead;
	token_t * start;
	token_t * wstart, * wend;

	when_clause( wstart, wend );						// optional

	if ( match( ACCEPT ) ) {
		start = ahead;
		if ( match( LP ) ) {							// start of mutex member list
			if ( list == nullptr ) {					// start of accept statement ?
				accept_cnt += 1;						// next accept statement
				accept_no = accept_cnt;					// retain value for this statement
				accept_start( startposn );
			} // if

			jump_t * end = list;						// save current position on mutex member stack
			if ( accept_mutex_list( list, symbol ) ) {
				if ( wstart != nullptr ) {				// _When ?
					gen_code( wstart, "if (" );			// insert before condition
					gen_code( start, "&&" );			// insert after condition
				} else {
					uncondaccept = true;
					gen_code( start, "if (" );			// insert before condition
				} // if

				// print labels for mutex-member list before completion statement
				char helpText[64];
				for ( jump_t * jump = list; jump != end; jump = jump->link ) {
					sprintf( helpText, "_U_A%04x_%04x : __attribute__ (( unused )) ;", accept_no, jump->index );
					gen_code( ahead, helpText );
				} // for

				gen_code( ahead, "{" );
				statement( symbol );
				gen_code( ahead, "} }" );				// closing brace for statement block and if block opened in accept_mutex_list
			} // if
		} else {
			// no reason to backtrack
			gen_error( ahead, "parenthesized mutex-member list must follow _Accept." );
		} // if
		return true;
	} // if

	ahead = back; return false;
} // accept_clause


static bool or_accept_clause( token_t * startposn, bool & uncondaccept, jump_t *& list, unsigned int & accept_no, symbol_t * symbol ) {
	token_t * back = ahead;
	token_t * separator;

	bool left = accept_clause( startposn, uncondaccept, list, accept_no, symbol );
	if ( left ) {
		token_t * posn = ahead;
		token_t * elsePosn = nullptr;

		// if ( check( ELSE ) ) {
		//	 elsePosn = ahead;
		//	 ahead->value = CONN_OR;					// change token kind
		// } // if
		if ( check( OR_OR ) && strcmp( ahead->hash->text, "or" ) == 0 ) {
			ahead->value = CONN_OR;						// change token kind
		} // if

		if ( match( CONN_OR ) ) {
			separator = ahead;							// remember where to put the separator
			bool right = or_accept_clause( startposn, uncondaccept, list, accept_no, symbol );
			if ( right ) {
				gen_code( separator, "else" );
				return true;
			} else {
				if ( elsePosn != nullptr ) {
					elsePosn->value = ELSE;				// change token kind back
				} // if
				// could be an error or a timeout
				ahead = posn;
				return left;
			} // if
		} else {
			return left;
		} // if
	} // if

	ahead = back; return false;
} // or_accept_clause


static bool accept_timeout_clause( token_t * start, token_t *& timeout, bool & uncondaccept, jump_t *& list, unsigned int & accept_no, symbol_t * symbol ) {
	token_t * back = ahead;
	token_t * wstart, * wend;
	token_t * elsePosn = nullptr;

	// if ( check( ELSE ) ) {
	// 	elsePosn = ahead;
	// 	ahead->value = CONN_OR;							// change token kind
	// } // if

	if ( match( CONN_OR ) ) {
		when_clause( wstart, wend );					// optional

		if ( match ( TIMEOUT ) ) {
			token_t * posn = ahead;

			if ( condition() ) {
				gen_code( start->prev_parse_token(), ", true" ); // add argument to uProtectAcceptStmtInstance
				const char * kind = uncondaccept ? "U" : "C";
				char helpText[64];
				sprintf( helpText, "else { if ( this -> uSerialInstance . execute%s (", kind );
				if ( wstart != nullptr ) {				// conditional timeout ?
					gen_code( wstart, helpText );
					gen_code( posn, "," );				// after _When condition
				} else {
					gen_code( posn, helpText );
					gen_code( posn, "true ," );			// _When value
				} // if
				gen_code( ahead, ") ) {" );				// after timeout
				timeout = ahead->prev_parse_token();
				accept_end( ahead, list, accept_no );
				gen_code( ahead, "{" );
				statement( symbol );
				gen_code( ahead, "}" );
			} else {
				// no reason to backtrack
				gen_error( ahead, "expression must follow _Timeout." );
			} // if
		} else {
			if ( elsePosn != nullptr ) {
				elsePosn->value = ELSE;					// change token kind back
				goto fini;
			} // if
			// no reason to backtrack
			gen_error( ahead, "_Accept or _Timeout must follow \"or\" connector." );
		} // if
		return true;
	} // if
  fini:
	ahead = back; return false;
} // accept_timeout_clause


static bool accept_else_clause( token_t * timeout, bool & uncondaccept, jump_t *& list, unsigned int & accept_no, symbol_t * symbol ) {
	token_t * back = ahead;
	token_t * wstart, * wend;
	token_t * posn;

	when_clause( wstart, wend );						// optional

	posn = ahead;
	if ( match( UELSE ) ) {								// end of accept clause list ?
		if ( wstart != nullptr ) {						// _When ?
			if ( timeout != nullptr ) {
				gen_code( timeout, "," );
				move_tokens( timeout, wstart, wend );
			} else {
				const char * kind = uncondaccept ? "U" : "C";
				char helpText[64];
				sprintf( helpText, "else { if ( this -> uSerialInstance . execute%s (", kind );
				gen_code( wstart, helpText );
				gen_code( posn, ") ) {" );
				accept_end( ahead, list, accept_no );
			} // if
			gen_code( ahead, "} else {" );
			statement( symbol );

			posn->remove_token();						// "else" in wrong place for reuse
			delete posn;
		} else {
			posn->hash = hash_table->lookup( "else" );	// reuse "else" by changing to "_Else" token
			posn->value = ELSE;							// change token kind back
			if ( timeout != nullptr ) {
				gen_warning( posn, "_Timeout preceding unconditional \"_Else\" is never executed." );
				gen_code( timeout, ", true" );			// force "else" clause with "true" _When value
				gen_code( posn, "}" );
				gen_code( ahead, "{" );
			} else {
				const char * kind = uncondaccept ? "U" : "C";
				char helpText[64];
				sprintf( helpText, "{ if ( ! this -> uSerialInstance . execute%s ( true ) ) {", kind );
				gen_code( ahead, helpText );
			} // if
			statement( symbol );
		} // if
		return true;
	} // if

	ahead = back; return false;
} // accept_else_clause


static bool accept_statement( symbol_t * symbol ) {
	token_t * back = ahead;
	token_t * start = ahead;
	jump_t * list = nullptr;
	unsigned int accept_no;
	token_t * timeout = nullptr;
	bool uncondaccept = false, hasTimeout, hasElse;

	bool root = or_accept_clause( start, uncondaccept, list, accept_no, symbol );
	if ( root ) {
		hasTimeout = accept_timeout_clause( start, timeout, uncondaccept, list, accept_no, symbol ); // optional
		hasElse = accept_else_clause( timeout, uncondaccept, list, accept_no, symbol ); // optional
		if ( ! hasTimeout && ! hasElse ) {				// no else or timeout
			if ( uncondaccept ) {
				gen_code( ahead, "else { { this -> uSerialInstance . executeU( ) ;" );
			} else {
				gen_code( ahead, "else { if ( this -> uSerialInstance . executeC( ) ) {" );
			} // if
			accept_end( ahead, list, accept_no );
		} // if
		gen_code( ahead, "} } }" );						// closing brace for if-block/else-block/accept-block
		return true;
	} else {
		token_t * wstart, * wend;

		when_clause( wstart, wend );					// optional

		if ( match ( TIMEOUT ) ) {
			start = ahead;
			if ( condition() ) {
				if ( wstart != nullptr ) {				// conditional timeout ?
					gen_code( wstart, "if (" );			// insert before condition
					gen_code( start, ")" );				// insert after condition
				} // if
				gen_code( start, "{ uBaseTask :: sleep (" ); // insert before timeout expression
				gen_code( ahead, ") ; {" );				// insert after timeout expression
				statement( symbol );
				gen_code( ahead, "} }" );				// closing brace for timeout-block
			} else {
				// no reason to backtrack
				gen_error( ahead, "expression must follow _Timeout." );
			} // if
			return true;
		} // if
	} // if

	ahead = back; return false;
} // accept_statement


static bool acceptreturn_statement( symbol_t * symbol ) {
	token_t * back = ahead;
	token_t * start = ahead;
	token_t * posn;
	jump_t * list = nullptr;

	if ( match( ACCEPTRETURN ) ) {
		posn = ahead;
		if ( match( LP ) ) {							// start of mutex member list
			if ( ! accept_mutex_list( list, symbol, "acceptTry2" ) ) {
				gen_error( ahead, "invalid mutex member list for _AcceptReturn." );
				// no reason to backtrack
				return true;
			} // if
			accept_start( start );
			gen_code( posn, "if (" );
			gen_code( ahead, "} else {" );				// end of accept clause list
			gen_code( ahead, "this -> uSerialInstance . acceptSetMask ( ) ;" );
			gen_code( ahead, "}" );						// closing brace for else
			gen_code( ahead, "return" );

			for ( ;; ) {								// optional expression
			  if ( eof() ) break;
			  if ( match( ';' ) ) break;
				scan();
			} // for

			for ( ; list != nullptr; ) {				// remove mutex member nodes allocated for member list
				jump_t * curr = list;
				list = list->link;
				delete curr;
			} // for

			gen_code( ahead, "}" );						// closing brace for accept statement
			return true;
		} else {
			// no reason to backtrack
			gen_error( ahead, "parenthesized mutex-member name-list must follow _AcceptReturn." );
			return true;
		} // if
	} // if

	ahead = back; return false;
} // acceptreturn_statement


static bool acceptwait_statement( symbol_t * symbol ) {
	token_t * back = ahead;
	token_t * start = ahead;
	token_t * posn;
	jump_t * list = nullptr;

	if ( match( ACCEPTWAIT ) ) {
		posn = ahead;
		if ( match( LP ) ) {							// start of mutex member list
			if ( ! accept_mutex_list( list, symbol, "acceptTry2" ) ) {
				gen_error( ahead, "invalid mutex member list for _AcceptWait." );
				// no reason to backtrack
				return true;
			} // if
			accept_start( start );
			gen_code( posn, "if (" );
			gen_code( ahead, "} else { this -> uSerialInstance . acceptSetMask ( ) ; }" ); // end of accept clause list

			for ( ; list != nullptr; ) {				// remove mutex member nodes allocated for member list
				jump_t * curr = list;
				list = list->link;
				delete curr;
			} // for

			if ( wait_statement_body() )  {
				gen_code( ahead, "}" );					// closing brace for accept statement
			// else
				// no reason to backtrack
			} // if
			return true;
		} else {
			// no reason to backtrack
			gen_error( ahead, "parenthesized mutex-member name-list must follow _AcceptWait." );
			return true;
		} // if
	} // if

	ahead = back; return false;
} // acceptwait_statement


// _Select statment


static unsigned int select_cnt = 0;						// count number of select statements in translation unit


static int or_select_clause( token_t * startposn, token_t *& wlocn, unsigned int & select_no, unsigned int & selector, unsigned int & leaf, symbol_t * symbol );

static int select_clause( token_t * startposn, token_t *& wlocn, unsigned int & select_no, unsigned int & selector, unsigned int & leaf, symbol_t * symbol ) {
	token_t * back = ahead;
	token_t * start;
	token_t * wstart, * wend;

	when_clause( wstart, wend );						// optional

	start = ahead;
	if ( match( LP ) || match( SELECT_LP ) ) {
		int select = or_select_clause( startposn, wlocn, select_no, selector, leaf, symbol );
		if ( select != -1 ) {
			if ( match( RP ) || match( SELECT_RP ) ) {
				start->value = SELECT_LP;				// change token kind
				ahead->prev_parse_token()->value = SELECT_RP; // change token kind
				if ( wstart != nullptr ) {				// _When ?
					move_tokens( wlocn, wstart, wend );
					gen_code( wlocn, "," );
				} // if
				return select;
			} // if
		} // if
	} else if ( match( SELECT ) ) {
		start = ahead;
		if ( condition() ) {							// selector expression
			char helpText[256];
			token_t * posn1, * posn2, * next;

			if ( selector == 0 ) {						// start of select statement ?
				select_cnt += 1;						// next select statement
				select_no = select_cnt;					// retain value for this statement
				gen_code( startposn, "{" );				// start select-block
			} // if

			sprintf( helpText, "UPP::UnarySelector < __typeof__" );
			gen_code( startposn, helpText );

			sprintf( helpText, " > uSelector%d (", selector );
			gen_code( startposn, helpText );
			posn1 = startposn->prev_parse_token();
			if ( wstart != nullptr ) {					// _When ?
				move_tokens( startposn, wstart, wend );
				gen_code( startposn, "," );
			} // if
			sprintf( helpText, ", %d ) ;", leaf );
			gen_code( startposn, helpText );
			posn2 = startposn->prev_parse_token();

			for ( token_t * p = start; p != ahead; p = next ) { // move selector-expression tokens
				//printf( "p:%s startposn:%s\n", p->hash->text, startposn->hash->text );
				next = p->next_parse_token();
				p->remove_token();
				p->add_token_before( *posn1 );
				token_t * temp = new token_t( *p );		// copy future tokens
				temp->add_token_before( * posn2 );
			} // for

			sprintf( helpText, "_U_S%04x_%04x : {", select_no, leaf );
			gen_code( ahead, helpText );
			statement( symbol );
			sprintf( helpText, "} goto _U_S%04x_%04x ;", select_no, 0 );
			gen_code( ahead, helpText );
			leaf += 1;
			return selector++;
		} else {
			// no reason to backtrack
			gen_error( ahead, "parenthesized selector expression must follow _Select." );
			return 0;
		} // if
	} // if

	ahead = back; return -1;
} // select_clause


static int and_select_clause( token_t * startposn, token_t *& wlocn, unsigned int & select_no, unsigned int & selector, unsigned int & leaf, symbol_t * symbol ) {
	token_t * back = ahead;

	int left = select_clause( startposn, wlocn, select_no, selector, leaf, symbol );
	if ( left != -1 ) {
		if ( check( AND_AND ) && strcmp( ahead->hash->text, "and" ) == 0 ) {
			ahead->value = CONN_AND;					// change token kind
		} // if

		if ( match( CONN_AND ) ) {
			int right = and_select_clause( startposn, wlocn, select_no, selector, leaf, symbol );
			if ( right != -1 ) {
				char helpText[256];
				sprintf( helpText, "UPP::BinarySelector < __typeof__ ( uSelector%d ), __typeof__ ( uSelector%d ) > uSelector%d (", left, right, selector );
				gen_code( startposn, helpText );
				sprintf( helpText, "uSelector%d, uSelector%d, UPP::Condition::And ) ;", left, right );
				gen_code( startposn, helpText );
				wlocn = startposn->prev_parse_token();	// location to place _When code if applicable
				return selector++;
			} else {
				// no reason to backtrack
				gen_error( ahead, "_Select clause must follow \"and\" connector." );
				return 0;
			} // if
		} else {
			return left;
		} // if
	} // if

	ahead = back; return -1;
} // and_select_clause


static int or_select_clause( token_t * startposn, token_t *& wlocn, unsigned int & select_no, unsigned int & selector, unsigned int & leaf, symbol_t * symbol ) {
	token_t * back = ahead;

	int left = and_select_clause( startposn, wlocn, select_no, selector, leaf, symbol );
	if ( left != -1 ) {
		token_t * posn = ahead;
		if ( check( OR_OR ) && strcmp( ahead->hash->text, "or" ) == 0 ) {
			ahead->value = CONN_OR;						// change token kind
		} // if

		if ( match( CONN_OR ) ) {
			int right = or_select_clause( startposn, wlocn, select_no, selector, leaf, symbol );
			if ( right != -1 ) {
				char helpText[256];
				sprintf( helpText, "UPP::BinarySelector < __typeof__ ( uSelector%d ), __typeof__ ( uSelector%d ) > uSelector%d (", left, right, selector );
				gen_code( startposn, helpText );
				sprintf( helpText, "uSelector%d, uSelector%d, UPP::Condition::Or ) ;", left, right );
				gen_code( startposn, helpText );
				wlocn = startposn->prev_parse_token();	// location to place _When code if applicable
				return selector++;
			} else {
				// could be an error or a timeout
				ahead = posn;
				return left;
			} // if
		} else {
			return left;
		} // if
	} // if

	ahead = back; return -1;
} // or_select_clause


static bool select_timeout_clause( token_t * start, unsigned int & select_no, symbol_t * symbol ) {
	token_t * back = ahead;
	token_t * wstart, * wend;
	token_t * posn;

	if ( match( CONN_OR ) ) {
		when_clause( wstart, wend );					// optional

		if ( match ( TIMEOUT ) ) {						// optional
			posn = ahead;
			if ( condition() ) {
				char label[32];

				gen_code( start, "," );
				if ( wstart != nullptr ) {				// _When ?
					move_tokens( start, wstart, wend );
				} else {
					gen_code( start, "true" );
				} // if
				gen_code( start, "," );

				move_tokens( start, posn, ahead );
				sprintf( label, "_U_S%04x_%04x : {", select_no, 2 );
				gen_code( ahead, label );
				statement( symbol );
				sprintf( label, "} goto _U_S%04x_%04xe ;", select_no, 0 );
				gen_code( ahead, label );
			} else {
				// no reason to backtrack
				gen_error( ahead, "expression must follow _Timeout." );
			} // if
		} else {
			// no reason to backtrack
			gen_error( ahead, "_Select or _Timeout must follow \"or\" connector." );
		} // if
		return true;
	} // if

	ahead = back; return false;
} // select_timeout_clause


static bool select_else_clause( token_t * start, unsigned int & select_no, symbol_t * symbol ) {
	token_t * back = ahead;
	token_t * wstart, * wend;
	token_t * posn;

	when_clause( wstart, wend );						// optional

	posn = ahead;
	if ( match ( UELSE ) ) {							// optional
		char label[32];

		gen_code( start, "," );
		if ( wstart != nullptr ) {
			move_tokens( start, wstart, wend );
		} else {
			gen_code( start, "true" );
		} // if

		sprintf( label, "_U_S%04x_%04x : {", select_no, 1 );
		gen_code( ahead, label );
		statement( symbol );
		sprintf( label, "} goto _U_S%04x_%04xe ;", select_no, 0 );
		gen_code( ahead, label );
		posn->remove_token();							// "_Else" not useful
		delete posn;

		return true;
	} // if

	ahead = back; return false;
} // select_else_clause


static bool select_statement( symbol_t * symbol ) {
	token_t * back = ahead;
	unsigned int select_no, selector = 0, leaf = 3;		// 0 => finished, 1 => else, 2 => timeout
	token_t * start = ahead, * wlocn = nullptr;

	int root = or_select_clause( start, wlocn, select_no, selector, leaf, symbol );
	if ( root != -1 ) {
		char helpText[256];
		bool hasElse, hasTimeout;

		sprintf( helpText, "UPP::Executor < __typeof__ ( uSelector%d ) > uExecutor_ ( uSelector%d", root, root );
		gen_code( start, helpText );

		hasTimeout = select_timeout_clause( start, select_no, symbol ); // optional
		hasElse = select_else_clause( start, select_no, symbol ); // optional

		sprintf( helpText, ") ; int _U_action ; _U_S%04x_%04x : ; _U_action = uExecutor_ . nextAction ( ) ; switch ( _U_action ) { case %d : goto _U_S%04x_%04xe ;", select_no, 0, 0, select_no, 0 );
		gen_code( start, helpText );
		if ( hasElse ) {
			sprintf( helpText, "case %d : goto _U_S%04x_%04x ;", 1, select_no, 1 );
			gen_code( start, helpText );
		} // if
		if ( hasTimeout ) {
			sprintf( helpText, "case %d : goto _U_S%04x_%04x ;", 2, select_no, 2 );
			gen_code( start, helpText );
		} // if
		for ( unsigned int i = 3; i < leaf; i += 1 ) {
			sprintf( helpText, "case %d : goto _U_S%04x_%04x ;", i, select_no, i );
			gen_code( start, helpText );
		} // for
		gen_code( start, "default : abort( \"internal error, _Select invalid nextAction value %x\", _U_action ); }" ); // terminate switch

		sprintf( helpText, "_U_S%04x_%04xe : ; }", select_no, 0 ); // terminate select-block
		gen_code( ahead, helpText );

		return true;
	} else if ( match( WHEN ) ) {						// _When ?
		// no reason to backtrack
		gen_error( ahead, "_Accept/_Select or _Timeout clause must follow _When." );
		return true;
	} // if

	ahead = back; return false;
} // select_statement


struct bound_t {
	token_t * oleft = nullptr, * oright = nullptr;
	token_t * exbegin = nullptr;
	symbol_t * extype = nullptr;
	token_t * idleft = nullptr, * idright = nullptr;
	bool pointer = false;
}; // bound_t


typedef std::set<symbol_t *> symbolset;
typedef std::list<token_t *> tokenlist;

static bool exception_declaration( attribute_t & attribute, bound_t & bound );
static bool bound_exception_declaration( bound_t & bound, int kind );

static bool catchResume_body( symbol_t * symbol, token_t * back, token_t * startTry, unsigned int handler, bound_t & bound, bool dotdotdot = false ) {
	char buffer[128];

	// line-number directive for the hoisted _CatchResume clause

	char linedir[32 + strlen( file )];
	sprintf( linedir, "# %d \"%s\"%s\n", line, file, flags[flag] );

	token_t * startblock = ahead;

  if ( ! compound_statement( symbol, nullptr, 0 ) ) return false; // body ?

	uassert( symbol->data->key == ROUTINE || symbol->data->key == MEMBER );
	uassert( symbol->data->attribute.startCR != nullptr );

	// resumption table entry

	if ( dotdotdot ) {
		gen_code( startTry, "uRoutineHandlerAny" );
	} else {
		gen_code( startTry, "uRoutineHandler <" );
		copy_tokens( startTry, bound.exbegin, bound.idleft );
		gen_code( startTry, ">" );
	} // if

	sprintf( buffer, "uHandler%d (", handler );			// begin handler constructor
	gen_code( startTry, buffer );

	if ( bound.oleft != nullptr ) {						// bound object ?
		gen_code( startTry, "& (" );
		move_tokens( startTry, bound.oleft, bound.oright ); // move bound object
		bound.oright->remove_token();					// remove the dot
		delete bound.oright;
		gen_code( startTry, ") ," );
	} // if

	// hoist _CatchResume clause into lambda

	gen_code( startTry, "[ & ]\n" );
	gen_code( startTry, linedir, '#' );
	gen_code( startblock, "-> uEHM :: FINALLY_CATCHRESUME_DISALLOW_RETURN {" );
	move_tokens_all( startTry, back, ahead );
	gen_code( startTry, "return uEHM :: FINALLY_CATCHRESUME_DISALLOW_RETURN :: NORETURN ; } ) ;" );

	return true;
} // catchResume_body


static bool catchResume_clause( symbol_t * symbol, token_t * startTry, unsigned int handler, unsigned int & catchany ) {
	token_t * back = ahead;

	if ( match( CATCHRESUME ) ) {
		if ( match( LP ) ) {
			attribute_t attribute;
			token_t * dotdotdot = ahead;
			bound_t bound;

			if ( exception_declaration( attribute, bound ) ) {
				return catchResume_body( symbol, back, startTry, handler, bound );
			} else if ( match( DOT_DOT_DOT ) && match( RP ) ) {
				if ( catchany == UINT_MAX ) catchany = handler;
				dotdotdot->remove_token();				// unnecessary in generated code
				return catchResume_body( symbol, back, startTry, handler, bound, true );
			} else if ( bound_exception_declaration( bound, CATCHRESUME ) ) {
				return catchResume_body( symbol, back, startTry, handler, bound );
			} // if
		} else {
			// no reason to backtrack
			gen_error( ahead, "missing exception declaration after _CatchResume." );
			return true;
		} // if
	} // if

	ahead = back; return false;
} // catchResume_clause


static bool catch_clause( symbol_t * symbol, bool & bflag, bool & optimized, hash_t *& para_name, symbol_t *& exception_type, tokenlist & try_catches, tokenlist & if_rethrow ) {
	token_t * back = ahead;
	token_t * prefix;
	bound_t bound;
	bflag = false;
	optimized = false;

	if ( match( CATCHRESUME ) ) {
		gen_error( ahead, "_CatchResume clause must precede any catch clauses." );
		goto fini;
	} // if

	if ( match( CATCH ) ) {
		if ( match( LP ) ) {
			attribute_t attribute;
			if ( exception_declaration( attribute, bound ) || ( match( DOT_DOT_DOT ) && match( RP ) ) ) {
				prefix = ahead;
				if ( compound_statement( symbol, nullptr, 0 ) ) {
					exception_type = attribute.typedef_base; // used to check whether splitting is necessary
					gen_code( prefix, "{" );
					gen_code( prefix, "if ( uOrigRethrow ) throw ;" );
					if_rethrow.push_front( prefix->prev_parse_token() ); // possible later deletion if this is the innermost try block
					gen_code( prefix, "try" );
					try_catches.push_front( prefix->prev_parse_token() ); // possible later deletion if the try block is not split
					gen_code( ahead, "catch( ... ) { uOrigRethrow = true; throw; }" );
					try_catches.push_front( ahead->prev_parse_token() ); // possible later deletion if the try block is not split
					gen_code( ahead, "}" );
					para_name = nullptr;				// prevent optimization
					return true;
				} // if
//		para_name = nullptr;			// prevent optimization
			} else if ( bound_exception_declaration( bound, CATCH ) ) {
				token_t * p, * next;
				token_t * start_catch;
				hash_t * local_param_name = nullptr;
				prefix = ahead;

				if ( compound_statement( symbol, nullptr, 0 ) ) {
					gen_code( prefix, "{ ");
					gen_code( prefix, "if ( uOrigRethrow ) throw ;");
					if_rethrow.push_front( prefix->prev_parse_token() ); // possible later deletion if this is the innermost try block
					gen_code( prefix, "const void * _U_bindingVal = (" );
					// look backwards for exception parameter name
					for ( p = bound.idright; p != bound.exbegin; p = p->prev_parse_token() ) {
						if ( p->value == IDENTIFIER || p->value == TYPE ) {
							gen_code( prefix, p->hash->text );
							local_param_name = p->hash;	// remember parameter name for optimization
							break;
						} // exit
					} // for
					gen_code( prefix, ")" );
					gen_code( prefix, bound.pointer ? "->" : "." );
					gen_code( prefix, "getRaiseObject ( ) ;" );
					token_t * realend = prefix->prev_parse_token(); // remember for optimization part
					gen_code( prefix, "if ( _U_bindingVal == & (" );
					// Move the tokens for the bound object expression from the catch clause argument to the if
					// statement within the catch body.
					move_tokens( prefix, bound.oleft, bound.oright );
					bound.oright->remove_token();		// remove the dot
					delete bound.oright;

					gen_code( prefix, ") )" );
					gen_code( prefix, "try ");
					try_catches.push_front( prefix->prev_parse_token() );

					gen_code( ahead, "catch( ... ) { uOrigRethrow = true; throw; }" );
					try_catches.push_front( ahead->prev_parse_token() );

					gen_code( ahead, "else { throw ; } }" );
					bflag = true;

					// Optimization part, remove catch clause, remove "if uOrigRethrow", remove "getRaiseObject()",
					// add "else".  It would be more efficient to not insert them in the first place, but this keeps
					// the optimization code separate from the normal case.

					if ( exception_type == bound.extype && local_param_name == para_name ) {
						optimized = true;

						// remove the catch clause and the } and all the other stuff before it
						for ( start_catch = back->prev_parse_token(); start_catch != realend; start_catch = next ) {
							next = start_catch->next_parse_token();
							start_catch->remove_token();
							delete start_catch;
						} // for
						if_rethrow.clear();				// just deleted what was stored here inside the loop, so scratch
						gen_code( start_catch, "else" );
						start_catch->remove_token();
						delete start_catch;
					} else {
						exception_type = bound.extype;	// return the type of OUR catch clause
					} // if
					para_name = local_param_name;		// return the name of OUR parameter
					return true;
				} // if
			} // if
		} else {
			// no reason to backtrack
			gen_error( ahead, "missing exception declaration after catch." );
			return true;
		} // if
	} // if
  fini:
	ahead = back; return false;
} // catch_clause


static bool split_try_statement( symbol_t * sym, symbolset & parents, symbolset & extypes ) {
	// given a symbol sym, and the two sets parents and extypes, decides whether a try-block splitting is necessary or
	// not, by checking if sym is in the parents, or any of its parents have catch clauses in the try block already

	if ( sym == nullptr ) {
		return ! extypes.empty();						// if catch_any split, if there were bound clauses before
	} //if

	// first check if any of sym's subtypes appeared
	if ( parents.count( sym ) != 0 ) {
		return true;
	} //if

	// then check if any of sym's base types appeared already
	// only single inheritance for exception types, and exception types on special base list
	for ( symbol_t * tmp = sym; tmp != 0; tmp = tmp->data->base ) {
		if ( extypes.count( tmp ) != 0 ) {
			return true;
		} //if
	} //for

	return false;
} // split_try_statement


static void register_extype( symbol_t * sym, symbolset & parents, symbolset & extypes ) {
	// register sym's ancestors in parents set, and sym in extypes set
	// only single inheritance for exception types, and exception types on special base list
	for ( symbol_t * tmp = sym->data->base; tmp != 0; tmp = tmp->data->base ) {
		parents.insert( tmp );
	} // for

	extypes.insert( sym );
} // register_extype


static bool finally_clause( symbol_t * symbol, token_t * start ) {
	token_t * back = ahead;

	if ( match( FINALLY ) ) {
		token_t * prev = ahead;

		// line-number directive for the hoisted finally clause

		char linedir[32 + strlen( file )];
		sprintf( linedir, "# %d \"%s%s\"\n", line, file, flags[flag] );

	  if ( ! compound_statement( symbol, nullptr, 0 ) ) return false; // parse finally body

		uassert( symbol->data->key == ROUTINE || symbol->data->key == MEMBER );
		uassert( symbol->data->attribute.startCR != nullptr );

		// hoist finally clause into lambda, return special type to prevent returns in finally body

		gen_code( start, "uEHM :: uFinallyHandler uFinallyHandler( [ & ] ( ) -> uEHM :: FINALLY_CATCHRESUME_DISALLOW_RETURN {\n" );
		gen_code( start, linedir, '#' );
		move_tokens_all( start, prev, ahead );
		gen_code( start, "return uEHM :: FINALLY_CATCHRESUME_DISALLOW_RETURN :: NORETURN ; } ) ;" );

		back->remove_token();							// remove the "_Finally" token
		return true;
	} // if

	ahead = back;
	return false;
} // finally_clause


static bool try_statement( symbol_t * symbol, token_t * label[], int cnt ) {
	token_t * back = ahead;
	token_t * start, * prefix;

	symbolset parents, extypes;
	symbol_t * exception_type = nullptr;

	start = ahead;
	if ( match( TRY ) ) {
		char linedir[32 + strlen( file )];
		unsigned int resume_clauses = 0, catch_clauses = 0, finally_clauses = 0;
		token_t * blockLC = gen_code( start, "{" );		// bracket try block

		// line-number directive for the try clause

		sprintf( linedir, "# %d \"%s\"%s\n", line, file, flags[flag] );
		prefix = gen_code( ahead, '\n' );
		gen_code( ahead, linedir, '#' );

		if ( compound_statement( symbol, nullptr, 0 ) ) {
			token_t * prev;
			bool bound;									// is catch clause bound ?
			prev = ahead;								// location before catch clause

			hash_t * para_name;
			bool optimized;
			bool split = false;
			tokenlist try_catches;
			tokenlist if_rethrow;
			token_t * t;

			if ( check( CATCHRESUME ) ) {
				char buffer[20];
				unsigned int catchany = UINT_MAX;
				token_t * prev = ahead;

				gen_code( prefix, "{" );
				for ( resume_clauses = 0; catchResume_clause( symbol, prefix, resume_clauses, catchany ); resume_clauses += 1 ) { // resumption handlers
					if ( resume_clauses > catchany ) {
						gen_error( prev, "'...' resumption handler must be the last handler of the _CatchResume blocks." );
					} // if
					prev = ahead;
				} // for

				// complete resumption table entry

				gen_code( prefix, "uEHM :: uHandlerBase * uHandlerTable [ ] = {" );
				for ( unsigned int i = 0; i < resume_clauses; i += 1 ) {
					sprintf( buffer, "& uHandler%d ,", i );
					gen_code( prefix, buffer );
				} // for
				gen_code( prefix, "} ; uEHM :: uResumptionHandlers uResumptionNode ( uHandlerTable ," );
				sprintf( buffer, "%d", resume_clauses );
				gen_code( prefix, buffer );
				gen_code( prefix, ") ;" );
				gen_code( ahead, "}" );

				// line-number directive for catch clauses after _CatchResume clauses in a try-block
				if ( check( CATCH ) ) {
					sprintf( linedir, "# %d \"%s\"%s\n", line, file, flags[flag] );
					gen_code( ahead, '\n' );
					gen_code( ahead, linedir, '#' );
				} // if
			} // if

			while ( catch_clause( symbol, bound, optimized, para_name, exception_type, try_catches, if_rethrow ) ) {
				catch_clauses += 1;
				if ( split_try_statement( exception_type, parents, extypes ) && ! optimized ) {
					split = true;					   // indicate following catch clauses are split off
					try_catches.clear();				// split => forget about removing the additional try/catch tokens
					parents.clear();
					extypes.clear();
					gen_code( start, "try {" );
					gen_code( prev, "}" );
				} // if
				if ( ! split ) {
					while ( ! if_rethrow.empty() ) {	// remove unneeded "if (uOrigRethrow) throw" tokens, i.e., the ones before the first split
						t = if_rethrow.front();
						t->remove_token();
						delete t;
						if_rethrow.pop_front();
					} // while
				} // if
				if ( bound ) {
					register_extype( exception_type, parents, extypes );
				} // if
				prev = ahead;
			} // while

			while ( ! try_catches.empty() ) {			// remove unneeded try/catch tokens, i.e., the ones after the last split
				t = try_catches.front();
				t->remove_token();
				delete t;
				try_catches.pop_front();
			} // while

			gen_code( ahead, "}" );

			if ( split ) {
				gen_code( blockLC->fore, "bool uOrigRethrow = false ;" ); // if splits, add flag declaration
			} // if

			if ( finally_clause( symbol, blockLC->fore ) ) { // insert at start of enclosing try block => error messages out of order
				finally_clauses = 1;
			} // if

			// line-number directive for code after try-block

			sprintf( linedir, "# %d \"%s\"%s\n", line, file, flags[flag] );
			gen_code( ahead, '\n' );
			gen_code( ahead, linedir, '#' );

			// If resume clauses but no catch clause for a try block, remove the "try" keyword before the compound
			// statement because a try block cannot exist without a catch clause.
			if ( catch_clauses == 0 && ( resume_clauses > 0 || finally_clauses > 0 ) ) {
				start->remove_token();					// remove "try"
				start = start->aft;						// backup to the inserted '{' before "try"
			} // if

			prefix_labels( start, ahead, label, cnt, false );
			return true;
		} // if
	} // if

	ahead = back; return false;
} // try_statement


static bool throw_expression() {
	token_t * back = ahead;

	if ( match( THROW ) ) {
		for ( ;; ) {									// look for the end of the statement
		  if ( eof() ) break;
		  if ( match( ';' ) ) {
				return true;
			} // if
			scan();
		} // for
	} // if

	ahead = back; return false;
} // throw_expression


static bool raise_expression( key_value_t kind, symbol_t * symbol ) {
	token_t * back = ahead;
	token_t * prefix, * suffix;
	char helpText[64];

	if ( match( kind ) ) {								// _Throw, _Resume or _ResumeTop ?
		const char * kindstr = kind == UTHROW ? "Throw" : "Resume";
		prefix = ahead;									// remember where to put the prefix
		for ( ;; ) {									// look for the end of the statement
		  if ( eof() ) break;
			if ( match( AT ) ) {
				if ( kind == UTHROW ) {
					gen_error( back, "keyword \"_Throw\" is not used for asynchronous raise.\nUse keyword \"_Resume\" with appropriate default resumption handler to achieve the same effect." );
					gen_code( back, "CHANGE_THROW_TO_RESUME" ); // force compilation error
				} // if

				gen_code( prefix, "uEHM :: ResumeAt (" );

				if ( ahead->prev_parse_token() != prefix ) { // async reraise ?
					gen_code( ahead, "," );				// separate exception and task/coroutine
				} // if

				prefix = ahead;
				for ( ;; ) {							// task/coroutine expression
				  if ( eof() ) break;
					if ( match( ';' ) ) {
						suffix = ahead->prev_parse_token();
						if ( prefix == suffix ) {
							gen_error( suffix, "expected task/coroutine value after _At." );
							goto fini;
						} // if
						gen_code( suffix, ")" );
						return true;
					} // if
					scan();
				} // for
			} // if
			if ( match( ';' ) ) {
				suffix = ahead->prev_parse_token();
				if ( prefix == suffix ) {				// no exception ? => reraise
					sprintf( helpText, "uEHM :: Re%s ( %s )", kindstr, kind == RESUMETOP ? "false" : "" );
					gen_code( prefix, helpText );
				} else {
					sprintf( helpText, "uEHM :: %s (", kindstr );
					gen_code( prefix, helpText );
					prefix = prefix->prev_parse_token();
					if ( symbol->data->found != nullptr && symbol->data->found->symbol && ! symbol->data->attribute.dclqual.qual.STATIC ) { // not static member?
						gen_code( suffix, ", this" );	// object for bound
					} else {
						gen_code( suffix, ", nullptr" ); // no object for bound
					} // if
					if ( kind == RESUMETOP ) {
						gen_code( suffix, ", false" );	// no consequence
					} // if
					gen_code( suffix, ")" );
				} // if
				return true;
			} // if
			scan();
		} // for
	} // if
  fini:
	ahead = back; return false;
} // raise_expression


static bool event_handler() {
	token_t * back = ahead;
	token_t * befparn;
	token_t * startE;

	befparn = ahead;
	if ( match( LA ) ) {
		gen_code( befparn, "& typeid" );
		befparn->value = LP;							// change '<' into '('
		befparn->hash = hash_table->lookup( "(" );
		startE = ahead;
		for ( ;; ) {
		  if ( eof() ) goto fini;
		  if ( check( ';' ) ) goto fini;
		  if ( check( LC ) ) goto fini;
		  if ( match( LA ) && ! match_closing( LA, RA ) ) goto fini;
		  if ( check( RA ) ) break;
			scan();
		} // for
		if ( startE == ahead ) goto fini;
		befparn = ahead;
		match( RA );									// match closing '>'
		befparn->value = RP;							// change '>' into ')'
		befparn->hash = hash_table->lookup( ")" );
		return true;
	  fini:
		gen_error( befparn, "incorrectly formed enable/disable clause." );
	} // if

	ahead = back; return false;
} // event_handler


static bool enable_statement( symbol_t * symbol ) {
	token_t * back = ahead;
	token_t * befEnable, * aftEnable;
	int event_clauses = 0;

	befEnable = ahead;
	if ( match( ENABLE ) ) {
		aftEnable = ahead;
		gen_code( aftEnable->prev_parse_token(), "{" );
		if ( event_handler() ) {
			gen_code( aftEnable->prev_parse_token(), "static const std::type_info * uEventTable [ ] = {" );
			do {
				event_clauses += 1;
				gen_code( ahead, "," );
			} while ( event_handler() );
			gen_code( ahead, "} ; uEHM :: uDeliverEStack uNoName ( true, uEventTable ," );
			char buffer[20];
			sprintf( buffer, "%d", event_clauses );
			gen_code( ahead, buffer );
			gen_code( ahead, ") ;" );
		} else {
			gen_code( ahead, "uEHM :: uDeliverEStack uNoName ( true ) ;" );
		} // if
		gen_code( ahead, "uEHM :: poll( ) ;" );			// poll on enter as there may be pending exceptions
		if ( null_statement() ) {						// optimize empty statement body
			gen_code( befEnable, "if ( uEHM :: pollCheck( ) ) {" );
			gen_code( ahead, "} }" );
			return true;
		} else if ( statement( symbol ) ) {
			gen_code( ahead, "uEHM :: poll( ) ;" );		// poll on exit as there may be pending exceptions
			gen_code( ahead, "}" );
			return true;
		} // if
	} // if

	ahead = back; return false;
} // enable_statement


static bool disable_statement( symbol_t * symbol ) {
	token_t * back = ahead;
	token_t * prefix;
	int event_clauses = 0;

	if ( match( DISABLE ) ) {
		prefix = ahead;
		gen_code( prefix->prev_parse_token(), "{" );
		if ( event_handler() ) {
			gen_code( prefix->prev_parse_token(), "static const std::type_info * uEventTable [ ] = {" );
			do {
				event_clauses += 1;
				gen_code( ahead, "," );
			} while ( event_handler() );
			gen_code( ahead, "} ; uEHM :: uDeliverEStack uNoName ( false , uEventTable ," );
			char buffer[20];
			sprintf( buffer, "%d", event_clauses );
			gen_code( ahead, buffer );
			gen_code( ahead, ") ;" );
		} else {
			gen_code( ahead, "uEHM :: uDeliverEStack uNoName ( false ) ;" );
		} // if
		if ( statement( symbol ) ) {
			gen_code( ahead, "uEHM :: poll( ) ;" );		// poll on exit as there may be pending exceptions
			gen_code( ahead, "}" );
			return true;
		} // if
	} // if

	ahead = back; return false;
} // disable_statement


// labeled-statement:
//   identifier ":" statement
//   "case" constant-expression ":" statement
//   "default" ":" statement

static bool labelled_statement( symbol_t * symbol ) {
	token_t * back = ahead;
	const int labprefix = 32;							// maximum number of labels prefixing a statement
	token_t * label[labprefix + 1];						// n+1 element holds the error case
	int cnt;

	for ( cnt = 0;; ) {
		label[cnt] = ahead;				// save current token
		if ( ! match( IDENTIFIER ) || ! match( ':' ) ) break;
		gen_code( ahead, "__attribute__ (( unused )) ;" );
		if ( cnt < labprefix ) {
			cnt += 1;
		} else {
			char msg[100];
			sprintf( msg, "more than %d labels prefixing a statement; label \"%s\" ignored.", labprefix, label[cnt]->hash->text );
			gen_error( ahead, msg );
		} // if
	} // for
	if ( cnt > 0 ) {									// any labels ?
		if ( check( FOR ) || check( WHILE ) || check( DO ) || check( SWITCH ) || check( IF ) || check( TRY ) || check( LC ) ) {
			int targetype;
			if ( check( SWITCH ) || check( IF ) || check( TRY ) || check( LC ) ) targetype = 2;
			else targetype = 1;
			token_t * l;
			for ( int i = 0; i < cnt; i += 1 ) {
				l = label[i];				// optimization
				// only need to create a symbol table for labels that need to be looked up by break and continue.
				l->symbol = focus->search_table( l->hash );
				if ( l->symbol == nullptr ) {			// not defined in this scope ?
					l->symbol = new symbol_t( l->value, l->hash );
					focus->insert_table( l->symbol );
					// SKULLDUGGERY: put label target kind into mutex index field
					l->symbol->data->index = targetype;
				} else {				// already defined ? => duplicate label
					label[i] = nullptr;
				} // if
			} // for
			if ( for_statement( symbol, label, cnt ) ) return true;
			if ( while_statement( symbol, label, cnt ) ) return true;
			if ( do_statement( symbol, label, cnt ) ) return true;
			if ( switch_statement( symbol, label, cnt ) ) return true;
			if ( if_statement( symbol, label, cnt ) ) return true;
			if ( try_statement( symbol, label, cnt ) ) return true;
			if ( compound_statement( symbol, label, cnt ) ) return true;
		} else if ( statement( symbol ) ) {
			return true;
		} // if
	} // if

	ahead = back; return false;
} // labelled_statement


static bool case_statement( symbol_t * symbol ) {
	token_t * back = ahead;

	if ( match( CASE ) ) {
		for ( ;; ) {
		  if ( eof() ) goto fini;
		  if ( match( ':' ) ) break;
			scan();
		} // for
		if ( statement( symbol ) ) {
			return true;
		} // if
	} // if
  fini:
	ahead = back; return false;
} // case_statement


static bool default_statement( symbol_t * symbol ) {
	token_t * back = ahead;

	if ( match( DEFAULT ) && match( ':' ) && statement( symbol ) ) {
		return true;
	} // if

	ahead = back; return false;
} // default_statement


// compound-statement:
//   "{" statement-seq_opt "}"

static bool compound_statement( symbol_t * symbol, token_t * label[], int cnt ) {
	token_t * back = ahead;

	// check for a {, don't match because a symbol table has to be built before scanning the next token, which may be an
	// identifier

	if ( check( LC ) ) {
		table_t * table = new table_t( nullptr );	// no symbol to connect to
		uassert( focus == top->tbl );
		table->lexical = top->tbl;
		table->push_table();

		match( LC );									// now scan passed the '{' for the next token

		token_t * before = ahead;
		while ( statement( symbol ) );
		prefix_labels( before, ahead, label, cnt, false );
		delete pop_table();
		if ( match( RC ) ) return true;
	} // if

	ahead = back; return false;
} // compound_statement


static bool null_statement() {
	token_t * back = ahead;

	if ( match( ';' ) ) return true;
	if ( match( LC ) && match( RC ) ) return true;

	ahead = back; return false;
} // null_statement


static bool object_declaration();

// statement:
//   selection-statement
//   iteration-statement
//   jump-statement
//   try-block
//   labeled-statement
//   compound-statement
//   declaration-statement
//   expression-statement

static bool statement( symbol_t * symbol ) {
	token_t * back = ahead;

	for ( ;; ) {
		if ( if_statement( symbol, nullptr, 0 ) ) return true;
		if ( switch_statement( symbol, nullptr, 0 ) ) return true;

		if ( while_statement( symbol, nullptr, 0 ) ) return true;
		if ( do_statement( symbol, nullptr, 0 ) ) return true;
		if ( for_statement( symbol, nullptr, 0 ) ) return true;

		if ( break_statement() ) return true;
		if ( continue_statement() ) return true;

		if ( accept_statement( symbol ) ) return true;
		if ( acceptreturn_statement( symbol ) ) return true;
		if ( acceptwait_statement( symbol ) ) return true;

		if ( select_statement( symbol ) ) return true;	// MUST APPEAR AFTER ACCEPT STATEMENT, which handles simple timeout

		if ( try_statement( symbol, nullptr, 0 ) ) return true;
		if ( throw_expression() ) return true;
		if ( raise_expression( UTHROW, symbol ) ) return true;
		if ( raise_expression( RESUME, symbol ) ) return true;
		if ( raise_expression( RESUMETOP, symbol ) ) return true;

		if ( enable_statement( symbol ) ) return true;
		if ( disable_statement( symbol ) ) return true;

		if ( labelled_statement( symbol ) ) return true;
		if ( case_statement( symbol ) ) return true;
		if ( default_statement( symbol ) ) return true;

		if ( compound_statement( symbol, nullptr, 0 ) ) return true;
		if ( object_declaration() ) return true;

		// handles statements currently not parsed: goto, return, expression.

		for ( ;; ) {
		  if ( eof() ) goto fini;
		  if ( match( ';' ) ) return true;
		  if ( compound_statement( symbol, nullptr, 0 ) ) return true; // gcc compound in expression
		  if ( check( RC ) ) return false;				// don't backtrack
			// Need to rewrite name of program main. Does not handle "main" nested in "main" without global qualifier "::".
			if ( strcmp( ahead->hash->text, "main" ) == 0 && strcmp( ahead->prev_parse_token()->hash->text, "::" ) == 0 ) {
				ahead->hash = hash_table->lookup( "uCpp_main" ); // change name
				ahead->value = ahead->hash->value;
			} // if
			scan();
		  if ( ahead->value >= ASM ) break;				// check again if keyword
		} // for
	} // for
  fini:
	ahead = back; return false;
} // statement


// access-specifier:
//   "private"
//   "protected"
//   "public"

static unsigned int access_specifier() {
	token_t * back = ahead;

	if ( match( PRIVATE ) ) return PRIVATE;
	if ( match( PROTECTED ) ) return PROTECTED;
	if ( match( PUBLIC ) ) return PUBLIC;

	ahead = back; return 0;
} // access_specifier


// base-specifier:
//   "::"_opt nested-name-specifier_opt class-name
//   virtual access-specifier_opt "::"_opt nested-name-specifier_opt class-name
//   access-specifier virtual_opt "::"_opt nested-name-specifier_opt class-name

static token_t * base_specifier( unsigned int & access ) {
	token_t * back = ahead;
	token_t * base;

	if ( ( base = type() ) != nullptr ) {
		base->left = back;
		base->right = ahead->prev_parse_token();
		access = PRIVATE;				// default
		return base;
	} else if ( match( VIRTUAL ) ) {
		access = access_specifier();
		token_t * temp = ahead;
		if ( ( base = type() ) != nullptr ) {
			base->left = temp;
			base->right = ahead->prev_parse_token();
			return base;
		} // if
	} else if ( ( access = access_specifier() ) != 0 ) {
		match( VIRTUAL );
		token_t * temp = ahead;
		if ( ( base = type() ) != nullptr ) {
			base->left = temp;
			base->right = ahead->prev_parse_token();
			return base;
		} // if
	} // if

	ahead = back; return nullptr;
} // base_specifier


// base-specifier-list:
//   base-specifier
//   base-specifier-list "," base-specifier

static bool base_specifier_list( symbol_t * symbol ) {
	token_t * back = ahead;
	token_t * token;
	symbol_t * base;
	unsigned int access;

	uassert( symbol != nullptr );

	if ( ( token = base_specifier( access ) ) != nullptr && symbol != token->symbol ) { // prevent inheriting from oneself, e.g., class x : public x
		base = token->symbol;
		uassert( base != nullptr );
#if 0
		printf( "base_specifier_list, symbol:%s, symbol->data->key:%s (%d), symbol->data->Mutex:%d, symbol->data->index:%d, "
				"base:%s, base->key:%s (%d), base->Mutex:%d, base->index:%d\n",
				symbol->hash->text, kind( symbol ), symbol->data->key, symbol->data->attribute.Mutex, symbol->data->index,
				base->hash->text, kind( base ), base->data->key, base->data->attribute.Mutex, base->data->index );
#endif
		if ( base->data->key == COROUTINE || base->data->key == TASK || base->data->attribute.Mutex || base->data->key == EXCEPTION || base->data->key == ACTOR || base->data->key == CORACTOR ) {

			// copy the left and right template delimiters from the scanned token

			symbol->data->left = token->left;
			symbol->data->right = token->right;

			if ( ( ( symbol->data->key == EXCEPTION || base->data->key == EXCEPTION ) && ( symbol->data->key != base->data->key || access != PUBLIC ) )
				 || symbol->data->base != nullptr ) {
				// This section only handles EXCEPTION and multiple inheritance problems. The error checking for coroutine,
				// monitor, comonitor and task is in class_specifier, after the type's members are parsed because mutex
				// members affect a type's kind. A side effect of separating the checking is for error messages to
				// appear out of order.
				if ( symbol->data->base != nullptr ) {
					char msg[strlen(symbol->data->base->hash->text) + strlen(base->hash->text) + 256];
					sprintf( msg, "multiple inheritance disallowed between base type \"%s\" of kind \"%s\" and base type \"%s\" of kind \"%s\"; inheritance ignored.",
							 symbol->data->base->hash->text, kind( symbol->data->base ), base->hash->text, kind( base ) );
					gen_error( ahead, msg );
				} else {
					char msg[strlen(symbol->hash->text) + strlen(base->hash->text) + 256];
					if ( symbol->data->key != base->data->key ) {
						sprintf( msg, "derived type \"%s\" of kind \"%s\" is incompatible with the base type \"%s\" of kind \"%s\"; inheritance ignored.",
								 symbol->hash->text, kind( symbol ), base->hash->text, kind( base ) );
					} else {
						sprintf( msg, "non-public inheritance disallowed between the derived type \"%s\" of kind \"%s\" and the base type \"%s\" of kind \"%s\"; inheritance ignored.",
								 symbol->hash->text, kind( symbol ), base->hash->text, kind( base ) );
					} // if
					gen_error( ahead, msg );
				} // if
			} else {
				symbol->data->base = base;				// now set the base

				// inherit the number of mutex members defined in the base class so that new mutex members in the
				// derived class are numbered correctly

				symbol->data->index = base->data->index;
			} // if
		} // if

		if ( symbol->data->attribute.startE != nullptr || symbol->data->attribute.startP != nullptr ) {
			gen_error( ahead, "base kind has already specified mutex/task data-structure arguments." );
		} // if

		symbol->data->base_list.push_back( base );
	} // if
#if 0
	printf( "base_specifier_list, symbol:%s, symbol->data->key:%s (%d), symbol->data->Mutex:%d, symbol->data->index:%d, "
			"base:%s, base->key:%s (%d), base->Mutex:%d, base->index:%d\n",
			symbol->hash->text, kind( symbol ), symbol->data->key, symbol->data->attribute.Mutex, symbol->data->index,
			base->hash->text, kind( base ), base->data->key, base->data->attribute.Mutex, base->data->index );
#endif
	if ( match( ',' ) ) {
		if ( base_specifier_list( symbol ) ) {
			return true;
		} // if
	} else {
		return true;
	} // if

	ahead = back; return false;
} // base_specifier_list


// base-clause:
//   ":" base-specifier-list

static bool base_clause( symbol_t * symbol ) {
	token_t * back = ahead;

	if ( match( ':' ) ) {
		base_specifier_list( symbol );

		// make sure base_specifier_list terminates with one of ';', '{', or EOF before continuing parse. Without this
		// check, many spurious paring errors can result.

		while ( ! check( ';' ) && ! check( LC ) && ! eof() ) {
			scan();
		} // while
		return true;
	} // if

	ahead = back; return false;
} // base_clause


static bool task_parameter_list( attribute_t & attribute );


// class-key:
//   class
//   struct
//   union
//   _Coroutine
//   _Task
//   _PeriodicTask
//   _RealTimeTask
//   _SporadicTask
//   _Event

static int class_key( attribute_t & attribute ) {
	token_t * back = ahead;

	if ( match( CLASS ) ) return CLASS;
	if ( match( STRUCT ) ) return STRUCT;
	if ( match( UNION ) ) return UNION;

	if ( match( COROUTINE ) ) {
		gen_code( ahead, "class" );
		return COROUTINE;
	} // if

	if ( match( TASK ) ) {
		gen_code( ahead, "class" );
		if ( check( LA ) && ! task_parameter_list( attribute ) ) goto fini;
		return TASK;
	} // if

	if ( match( RTASK ) ) {
		gen_code( ahead, "class" );
		attribute.rttskkind.kind.APERIODIC = true;
		if ( check( LA ) && ! task_parameter_list( attribute ) ) goto fini;
		return TASK;
	} // if

	if ( match( PTASK ) ) {
		gen_code( ahead, "class" );
		attribute.rttskkind.kind.PERIODIC = true;
		if ( check( LA ) && ! task_parameter_list( attribute ) ) goto fini;
		return TASK;
	} // if

	if ( match( STASK ) ) {
		gen_code( ahead, "class" );
		attribute.rttskkind.kind.SPORADIC = true;
		if ( check( LA ) && ! task_parameter_list( attribute ) ) goto fini;
		return TASK;
	} // if

	if ( match( EXCEPTION ) ) {
		gen_code( ahead, "class" );
		if ( strcmp( back->hash->text, "_Event" ) == 0 ) {
			gen_warning( ahead, "_Event keyword deprecated and replaced with _Exception." );
		} // if
		return EXCEPTION;
	} // if

	if ( match( ACTOR ) ) {
		gen_code( ahead, "class" );
		if ( check( LA ) && ! task_parameter_list( attribute ) ) goto fini;
		return ACTOR;
	} // if

	if ( match( CORACTOR ) ) {
		gen_code( ahead, "class" );
		if ( check( LA ) && ! task_parameter_list( attribute ) ) goto fini;
		return CORACTOR;
	} // if

  fini:
	ahead = back; return 0;
} // class_key


static void make_type( token_t * token, symbol_t *& symbol, table_t * locn ) { // symbol is in/out parameter because it may be changed
	uassert( token != nullptr );
	uassert( symbol != nullptr );

	if ( symbol->data->found == focus ) {
		// if the symbol already exists in the current symbol table as an identifier, make it a type

		token->value = symbol->value = TYPE;
	} else {
		if ( symbol->data->found != nullptr ) {
			// if the symbol exists in some other symbol table, make a copy of the symbol

			token->symbol = symbol = new symbol_t( symbol->value, symbol->hash );
		} // if

		// change the token and symbol value to type

		token->value = symbol->value = TYPE;

		// insert the symbol into the current symbol table

		locn->insert_table( symbol );
	} // if
} // make_type


static bool template_qualifier( attribute_t & attribute );
static token_t * declarator( attribute_t & attribute );

// template-parameter:
//	type-parameter
//	parameter-declaration

// type-parameter:
//	"class" identifier_opt
//	"class" identifier_opt "=" type-id
//	"typename" identifier_opt
//	"typename" identifier_opt "=" type-id
//	"template" "<" template-parameter-list ">" "class" identifier_opt
//	"template" "<" template-parameter-list ">" "class" identifier_opt = id-expression

static bool template_parameter( attribute_t & attribute ) {
	token_t * back = ahead;
	unsigned int ckey;
	token_t * token;
	symbol_t * symbol;

  again:
	if ( match( CLASS ) || match( TYPENAME ) ) {
		ckey = CLASS;

		match( DOT_DOT_DOT );							// optional

		// if a class key is found

		if ( ( token = identifier() ) != nullptr ) {
			symbol = token->symbol;

			// change the token's and symbol's value to TYPE

			if ( symbol->data->found == focus ) {
				// if the symbol already exists in the current symbol table as an identifier, make it a type

				token->value = symbol->value = TYPE;
			} else {
				// template parameter declarations have no parent (yet) and so the symbol table created during scan
				// lookup is used
				if ( symbol->data->found != nullptr /* && symbol->data->found != focus */ ) {
					// if the symbol exists in some other symbol table, make a copy of the symbol

					token->symbol = symbol = new symbol_t( symbol->value, symbol->hash );
				} // if

				token->value = symbol->value = TYPE;	// change the token and symbol value to type
				attribute.plate->insert_table( symbol ); // insert the symbol into the current symbol table
			} // if

			// set the symbol's key to class key

			symbol->data->key = ckey;
		} else if ( ( token = type() ) != nullptr ) {
			// if a type token is found, grab the symbol associated with the token and prepare to do some analysis

			symbol = token->symbol;
			uassert( symbol != nullptr );

			if ( symbol->data && symbol->data->found == attribute.plate ) {
				// if the symbol already exists in the current symbol table as a type, verify that its kind is the same
				// as the previous definition

				if ( symbol->data->key != ckey ) {
					// struct and class are equivalent for prototype
					if ( ! ( ( symbol->data->key == STRUCT && ckey == CLASS ) || ( symbol->data->key == CLASS && ckey == STRUCT ) ) ) {
						char msg[strlen(symbol->hash->text) + 128];
						sprintf( msg, "\"%s\" redeclared with different kind.", symbol->hash->text );
						gen_error( ahead, msg );
					} else {
						symbol->data->key = ckey;
					} // if
				} // if
			} else {
				//if ( symbol->data->found != nullptr /* && symbol->data->found != focus */ ) {
				// if the symbol exists in some other symbol table make a new symbol for the current symbol table

				token->symbol = symbol = new symbol_t( token->value, token->hash );
				//} // if

				symbol->data->key = ckey;				// set the symbol's class to the class key
				attribute.plate->insert_table( symbol ); // insert the symbol into the template symbol
			} // if
		} // if

		if ( match( '=' ) ) {							// scan off optional initialization value
			for ( ;; ) {
			  if ( eof() ) break;
			  if ( match( LP ) ) {
					if ( match_closing( LP, RP ) ) continue;
					goto fini;
			  } else if ( match( LA ) ) {
					if ( match_closing( LA, RA ) ) goto again;
					goto fini;
			  } else if ( match( LC ) ) {
					if ( match_closing( LC, RC ) ) continue;
					goto fini;
			  } else if ( match( LB ) ) {
					if ( match_closing( LB, RB ) ) continue;
					goto fini;
			  } // if
			  if ( check( ',' ) ) break;
			  if ( check( RA ) ) break;
				// must correctly parse type-names to allow the above match_closing check
				if ( type() == nullptr ) {
					scan();
				} // if
			} // for
		} // if

		return true;
	} else if ( match( TEMPLATE ) ) {
		// internal template parameters can be ignored
		for ( ;; ) {
		  if ( eof() ) break;
		  if ( match( LP ) ) {
				if ( match_closing( LP, RP ) ) continue;
				goto fini;
		  } else if ( match( LA ) ) {
				if ( match_closing( LA, RA ) ) goto again;
				goto fini;
		  } else if ( match( LC ) ) {
				if ( match_closing( LC, RC ) ) continue;
				goto fini;
		  } else if ( match( LB ) ) {
				if ( match_closing( LB, RB ) ) continue;
				goto fini;
		  } // if
		  if ( check( ',' ) ) break;
		  if ( check( RA ) ) break;
			// must correctly parse type-names to allow the above match_closing check
			if ( type() == nullptr ) {
				scan();
			} // if
		} // for

		return true;
	} else if ( specifier_list( attribute ) ) {
		match( DOT_DOT_DOT );							// optional

		// does not handle unnamed routine pointer: template<double (*)() > struct P {};
		declarator( attribute );
		if ( attribute.startI != nullptr ) {			// named parameter ?
			uassert( attribute.startI->symbol != nullptr );
			if ( attribute.startI->symbol->data != nullptr && attribute.startI->symbol->data->found != focus ) {
				attribute.plate->insert_table( attribute.startI->symbol ); // insert the symbol into the template symbol
			} // if
		} // if

		if ( match( '=' ) ) {							// scan off optional initialization value
			for ( ;; ) {
			  if ( eof() ) break;
			  if ( match( LP ) && ! match_closing( LP, RP ) ) goto fini;
				// first match without closing is less than operator; otherwise template parameters
			  if ( match( LA ) && match_closing( LA, RA ) ) continue;
			  if ( check( ',' ) ) break;
			  if ( check( RA ) ) break;
				// must correctly parse type-names to allow the above match_closing check
				if ( type() == nullptr ) {
					scan();
				} // if
			} // for
		} // if
		return true;
	} // if
  fini: ;
	ahead = back; return false;
} // template_parameter


// template-parameter-list:
//	template-parameter
//	template-parameter-list "," template-parameter

static bool template_parameter_list( attribute_t & attribute ) {
	local_t * prev = attribute.plate->local;

	// template parameter may be empty so always return true (no backup)

	if ( template_parameter( attribute ) ) {
		// only reset if this is the first parse (constructor/destructor/routine) and the last template of a list of
		// templates for a routine
		if ( attribute.plate->local != prev ) attribute.plate->startT = attribute.plate->local;
		while ( match( ',' ) ) {
			template_parameter( attribute );
		} // while
		if ( attribute.plate->local != prev ) attribute.plate->endT = attribute.plate->local;
	} // if

	return true;
} // template_parameter_list


static bool task_parameter_list( attribute_t & attribute ) {
	token_t * back = ahead;
	token_t * startP;
	token_t * endP;

	if ( match( LA ) ) {
		startP = ahead;
		for ( ;; ) {
		  if ( eof() ) goto fini;
		  if ( check( ';' ) ) goto fini;
		  if ( check( LC ) ) goto fini;
		  if ( match( LA ) && ! match_closing( LA, RA ) ) goto fini;
		  if ( check( RA ) ) break;
		  if ( check( ',' ) ) break;
			scan();
		} // for
		if ( startP == ahead ) goto fini;
		endP = ahead;
		if ( ! match( RA ) ) goto fini;					// match closing '>'

		attribute.startP = startP;
		attribute.endP = endP;
		return true;
	  fini:
		ahead = startP;									// attempt local error recovery
		match_closing( LA, RA );
		if ( startP->value != ERROR ) {					// only one error message
			gen_error( startP , "incorrectly formed task argument, argument list ignored." );
		} // if
		return true;
	} // if

	ahead = back; return false;
} // task_parameter_list


static bool mutex_parameter_list( attribute_t & attribute ) {
	token_t * back = ahead;
	token_t * startE;
	token_t * startM;
	token_t * endM;

	if ( match( LA ) ) {
		startE = ahead;
		for ( ;; ) {
		  if ( eof() ) goto fini;
		  if ( check( ';' ) ) goto fini;
		  if ( check( LC ) ) goto fini;
		  if ( match( LA ) && ! match_closing( LA, RA ) ) goto fini;
		  if ( check( RA ) ) break;
		  if ( check( ',' ) ) break;
			scan();
		} // for
		if ( startE == ahead ) goto fini;
		if ( match( ',' ) ) {
			startM = ahead;
			for ( ;; ) {
			  if ( eof() ) goto fini;
			  if ( check( ';' ) ) goto fini;
			  if ( check( LC ) ) goto fini;
			  if ( match( LA ) && ! match_closing( LA, RA ) ) goto fini;
			  if ( check( RA ) ) break;
			  if ( check( ',' ) ) break;
				scan();
			} // for
			if ( startM == ahead ) goto fini;
			endM = ahead;
			if ( ! match( RA ) ) goto fini;				// match closing '>'

			if ( attribute.startE != nullptr && attribute.startE != startE ) {
				gen_error( startE, "multiple mutex qualifiers specified, only one allowed." );
			} // if

			attribute.startE = startE;
			attribute.startM = startM;
			attribute.endM = endM;
			return true;
		} // if
	  fini:
		ahead = startE;									// attempt local error recovery
		match_closing( LA, RA );
		if ( startE->value != ERROR ) {					// only one error message
			gen_error( startE, "incorrectly formed mutex argument list, argument list ignored." );
		} // if
		return true;
	} // if

	ahead = back; return false;
} // mutex_parameter_list


static bool type_qualifier( attribute_t & attribute ) {
	token_t * back = ahead;

	if ( match( STATIC ) ) { attribute.dclqual.qual.STATIC = true; return true; }
	if ( match( EXTERN ) ) { attribute.dclqual.qual.EXTERN = true; return true; }
	if ( match( MUTABLE ) ) { attribute.dclqual.qual.MUTABLE = true; return true; }
	if ( match( THREAD ) ) { attribute.dclqual.qual.THREAD = true; return true; }
	if ( match( THREAD_LOCAL ) ) { attribute.dclqual.qual.THREAD = true; return true; } // C++11

	if ( match( EXTENSION ) ) { attribute.dclqual.qual.EXTENSION = true; return true; }; // gcc specific

	if ( match( INLINE ) ) { attribute.dclqual.qual.INLINE = true; return true; }
	if ( match( VIRTUAL ) ) { attribute.dclqual.qual.VIRTUAL = true; return true; }
	if ( match( EXPLICIT ) ) { attribute.dclqual.qual.EXPLICIT = true; return true; }

	if ( match( CONST ) ) { attribute.dclqual.qual.CONST = true; return true; }
	if ( match( VOLATILE ) ) { attribute.dclqual.qual.VOLATILE = true; return true; }
	if ( match( RESTRICT ) ) { attribute.dclqual.qual.RESTRICT = true; return true; }
	if ( match( REGISTER ) ) { attribute.dclqual.qual.REGISTER = true; return true; }
	if ( match( ATOMIC ) ) { attribute.dclqual.qual.ATOMIC = true; return true; } // C11

	if ( match( TYPEDEF ) ) { attribute.dclkind.kind.TYPEDEF = true; return true; }
	if ( match( FRIEND ) ) { attribute.dclkind.kind.FRIEND = true; return true; }
	if ( match( CONSTEXPR ) ) { attribute.dclkind.kind.CONSTEXPR = true; return true; }

	if ( match( MUTEX ) ) {
		attribute.dclmutex.qual.mutex = true;
		if ( check( LA ) && ! mutex_parameter_list( attribute ) ) goto fini;
		attribute.dclmutex.qual.MUTEX = true;
		return true;
	} // if
	if ( match( NOMUTEX ) ) {
		attribute.dclmutex.qual.nomutex = true;
		if ( check( LA ) && ! mutex_parameter_list( attribute ) ) goto fini;
		attribute.dclmutex.qual.NOMUTEX = true;
		return true;
	} // if

  fini:
	ahead = back; return false;
} // type_qualifier


// template-qualifier:
//	"export"_opt "template" "<" template-parameter-list ">"

static bool template_qualifier( attribute_t & attribute ) {
	token_t * back = ahead;

	match( EXPORT );									// optional export clause

	token_t * start = ahead;
	if ( match( TEMPLATE ) ) {
		if ( match( LA ) ) {
			if ( attribute.plate == nullptr ) {
				attribute.plate = new table_t( nullptr );
			} // if

			// must push this table on the lookup stack as types in the table can be used immediately.
			// e.g., template<typename T, T m, bool> struct Mod;
			if ( focus != attribute.plate ) {
				attribute.focus = focus;				// save current focus
				attribute.plate->lexical = focus;		// temporarily connect to current lexical scope
				attribute.plate->push_table();
			} // if
			template_parameter_list( attribute );		// can be empty
			if ( match( RA ) ) {
				attribute.startT = start;
				attribute.endT = ahead;
				return true;
			} // if
		} else {
			// no reason to backtrack
			return true;
		} // if
	} // if

	ahead = back; return false;
} // template_qualifier


static token_t * class_head( attribute_t & attribute ) {
	token_t * back = ahead;
	token_t * token;
	symbol_t * symbol;
	unsigned int key;

	if ( ( key = class_key( attribute ) ) != 0 ) {		// class key found ?
		attribute_clause_list();						// optional

		if ( attribute.dclkind.kind.FRIEND ) {			// friend declaration ?
			if ( ( token = identifier() ) != nullptr ) { // undefined ?
				// Undefined friend declarations are moved to the root scope.
				symbol = token->symbol;
				uassert( symbol != nullptr );
				make_type( token, symbol, root );
				symbol->data->key = key;				// set the symbol's class key to the class key
			} else if ( ( token = type() ) != nullptr ) {
				symbol = token->symbol;
				uassert( symbol != nullptr );
				// verify that its kind is the same as the previous definition
				if ( symbol->data->key != key ) {
					// struct and class are equivalent for prototype
					if ( ! ( ( symbol->data->key == STRUCT && key == CLASS ) || ( symbol->data->key == CLASS && key == STRUCT ) ) ) {
						char msg[strlen(symbol->hash->text) + 256];
						sprintf( msg, "\"%s\" redeclared with different kind.", symbol->hash->text );
						gen_error( ahead, msg );
					} else {
						symbol->data->key = key;
					} // if
				} // if
			} // if
			return token;
		} // if

		if ( ( token = identifier() ) != nullptr ) {
			match( FINAL );								// optional

			symbol = token->symbol;
			uassert( symbol != nullptr );
			// if the top of the lexical stack is a template, add this type to the actual table below it.
			make_type( token, symbol, top->tbl == attribute.plate ? top->link->tbl : top->tbl );
			symbol->data->key = key;					// set the symbol's class key to the class key
		} else if ( ( token = type() ) != nullptr ) {
			match( FINAL );				// optional
			// if a type token is found, grab the symbol associated with the token and do some analysis

			symbol = token->symbol;
			uassert( symbol != nullptr );

			// look ahead for old style C struct declarations.
			if ( ( symbol->data->key == STRUCT || symbol->data->key == UNION ) && ( ! check( ':' ) && ! check( LC ) && ! check( ';' ) ) ) return token;

			uassert( symbol->data->found != nullptr );

			table_t * containing = focus == attribute.plate ? focus->lexical : focus;
			symbol_t * checksym;

			// the scope that is (lexically) outside this declaration (for double lookup)
			table_t * outside = top->tbl == attribute.plate ? top->link->tbl : top->tbl;

			// lookup the type name again to determine if it was specified using a nested name (qualified).  (this could
			// have been accomplished by creating a flag in attribute to indicate a nested name.)
			if ( outside != symbol->data->found &&
				 ( checksym = outside->search_table( symbol->hash ) ) != nullptr &&
				 ( checksym->data->found == symbol->data->found ) ) {

				// if found in the same previous table => no nested naming => hiding => create a new entry
				token->symbol = symbol = new symbol_t( token->value, token->hash );
				containing->insert_table( symbol );		// insert the symbol into the current symbol table
				symbol->data->key = key;				// set the symbol's class to the class key
			} else {
				// if not found in search => nested naming => augmenting existing type definition if found in a
				// different table => augmenting existing type definition

				// verify that its kind is the same as the previous definition
				if ( symbol->data->key != key ) {
					// struct and class are equivalent for prototype
					if ( ! ( ( symbol->data->key == STRUCT && key == CLASS ) || ( symbol->data->key == CLASS && key == STRUCT ) ) ) {
						char msg[strlen(symbol->hash->text) + 256];
						sprintf( msg, "\"%s\" redeclared with different kind.", symbol->hash->text );
						gen_error( ahead, msg );
					} else {
						symbol->data->key = key;
					} // if
				} // if
			} // if
		} else {
			// Cannot generate anonymous types for coroutine or task because constructors and destructors are needed.
			// If there is no name for the type, unlike the compiler, we cannot generated one because the order of
			// includes may cause different names to be generated for different compilations.

			token = gen_code( ahead, "", TYPE );		// insert token into the token stream

			// create a symbol that must accompany this token as a type

			token->symbol = symbol = new symbol_t( token->value, token->hash );

			// insert the symbol into the current symbol table

			focus->insert_table( symbol );

			symbol->data->key = key;

			if ( key == COROUTINE || key == TASK || key == EXCEPTION ) { // monitor is checked elsewhere
				symbol->data->key = key;
				char msg[256];
				sprintf( msg, "cannot create anonymous %s because of the need for constructors and destructors.", kind( symbol ) );
				gen_error( ahead, msg );
				// By making this type look like a class, it will not generate additional bogus tokens that might cause
				// problems later on.
				//symbol->data->key = CLASS;
			} // if
		} // if

		// save a pointer to the next token so that if we have to insert a base class definition later, it be done

		symbol->data->base_token = ahead;

		return token;
	} // if

	ahead = back; return nullptr;
} // class_head


static bool access_declaration() {
	token_t * back = ahead;
	unsigned int access;

	if ( ( access = access_specifier() ) != 0 ) {
		if ( match( ':' ) ) {
			// modify the access privileges of member functions added to the symbol table after this point

			uassert( top != nullptr );
			focus->access = access;

			return true;
		} // if
	} // if

	ahead = back; return false;
} // access_declaration


static bool member_module() {
	token_t * back = ahead;

	if ( access_declaration() ) return true;
	if ( object_declaration() ) return true;

	ahead = back; return false;
} // member_module


static void make_mutex_class( symbol_t * clss );


static bool class_specifier( attribute_t & attribute ) {
	token_t * back = ahead;
	token_t * clss;
	table_t * table;
	symbol_t * symbol;

	if ( ( clss = class_head( attribute ) ) != nullptr ) {
		// specialized template: "template<...> class T<...>" => create new symbol table
		if ( templateSpecial ) {			// specialization ?
			templateSpecial = false;
			// SKULLDUGGERY: Naming specializations is difficult. Instead, assume the template specialization is
			// completely inlined, which is reasonable because templates cannot be separately compiled. The next
			// specialization simply drops all local declarations from the previous one and starts again.  This hack
			// does not handle non-inline definitions of specialization members, as uC++ does not track the
			// specialization name that prefixes the non-inline definition.
			clss->symbol->data->attribute.plate = nullptr; // free old storage ?
			clss->symbol->data->index = 1;				// reset mutex member counter
		} // if

		symbol = clss->symbol;
		uassert( symbol != nullptr );

		attribute.typedef_base = symbol;

		// copy these fields to complete symbol attribute

		if ( attribute.startE != nullptr ) {
			symbol->data->attribute.startE = attribute.startE;
			symbol->data->attribute.startM = attribute.startM;
			symbol->data->attribute.endM = attribute.endM;
		} // if
		if ( attribute.startP != nullptr ) {
			symbol->data->attribute.startP = attribute.startP;
			symbol->data->attribute.endP = attribute.endP;
		} // if

		symbol->data->attribute.rttskkind = attribute.rttskkind;

		base_clause( symbol );

		// Ignore the case where a class is used in a declaration.  When a class is defined/forward, check the mutex
		// attributes.

		if ( check( LC ) || check( ';' ) ) {
			// If this is a complete (not forward) declaration, treat the default attributes as if they had been
			// explicitly specified.  This prevents an attribute specification on a forward declaration from applying to
			// the actual declaration, which is probably an error.

			if ( check( LC ) && ! attribute.dclmutex.qual.MUTEX && ! attribute.dclmutex.qual.NOMUTEX ) {
				if ( symbol->data->key == TASK ) {
					attribute.dclmutex.qual.MUTEX = true;
				} else {
					attribute.dclmutex.qual.NOMUTEX = true;
				} // if
			} // if

			// check that only one of the mutex attributes is specified for symbol and current definition

			if ( ( symbol->data->attribute.dclmutex.qual.MUTEX | attribute.dclmutex.qual.MUTEX ) &&
				 ( symbol->data->attribute.dclmutex.qual.NOMUTEX | attribute.dclmutex.qual.NOMUTEX ) ) {
				if ( check( LC ) ) {
					// this declaration is complete declaration -- ignore previous attributes
					gen_error( ahead, "may not specify both mutex and nomutex attributes for a class. Ignoring previous declaration." );
					symbol->data->attribute.dclmutex.value = 0;
				} else if ( symbol->data->table != nullptr ) {
					// previous declaration was complete declaration -- ignore current attributes
					gen_error( ahead, "may not specify both mutex and nomutex attributes for a class. Ignoring this declaration." );
					attribute.dclmutex.value = 0;
				} else {
					// duelling forward declarations -- ignore both sets of attributes
					gen_error( ahead, "may not specify both mutex and nomutex attributes for a class. Assuming default attribute." );
					symbol->data->attribute.dclmutex.value = 0;
					attribute.dclmutex.value = 0;
				} // if
			} // if

			// compute the attribute of this class based on explicit specification and default rules

			symbol->data->attribute.dclmutex.value |= attribute.dclmutex.value;

			// if mutex attribute specified or task => this is a mutex type so destructor is mutex

			if ( symbol->data->attribute.dclmutex.qual.MUTEX || symbol->data->key == TASK ) {
				symbol->data->attribute.Mutex = true;
			} // if
		} // if

		// check for a {, don't match because a symbol table has to be built before scanning the next token, which may
		// be an identifier

		if ( check( LC ) ) {
			// copy these fields to complete symbol attribute

			if ( attribute.plate != nullptr && symbol->data->attribute.plate == nullptr ) {
				symbol->data->attribute.startT = attribute.startT;
				symbol->data->attribute.endT = attribute.endT;
				symbol->data->attribute.plate = attribute.plate;
				symbol->data->attribute.plate->startT = attribute.plate->startT;
				symbol->data->attribute.plate->endT = attribute.plate->endT;
			} // if

			// define the base class

			gen_base_clause( symbol->data->base_token, symbol );

			if ( symbol->data->attribute.plate != nullptr ) {
				// use the template table as the symbol table for this class

//		uassert( symbol->data->table == nullptr );
				symbol->data->table = table = symbol->data->attribute.plate; // already pushed

				// link the symbol and table together because it was not originally done when the template table was
				// created because the class symbol was not yet known.

				table->symbol = symbol;
			} else {
				// create a new symbol table

//		uassert( symbol->data->table == nullptr );
				symbol->data->table = table = new table_t( symbol );
				table->push_table();
			} // if

			// set the search path to the current lexical context (table).

			table->lexical = symbol->data->found;

			table->access = PRIVATE;
			table->defined = false;

			match( LC );								// now scan passed the '{' for the next token

			symbol->data->attribute.startCR = attribute.startCR;
			symbol->data->attribute.fileCR = attribute.fileCR;
			symbol->data->attribute.lineCR = attribute.lineCR;

			// generate the class prefix

			gen_class_prefix( ahead, symbol );

			// if type is a TASK, generate PIQ instance

			if ( symbol->data->key == TASK ) {
				gen_PIQ( table->protected_area, symbol );
			} // if

			// if type is mutex, generate uSerial instance

			if ( symbol->data->attribute.Mutex ) {
				make_mutex_class( symbol );
			} // if

			while ( member_module() );

			table->defined = true;
			table->access = PRIVATE;

			if ( symbol->data->base != nullptr ) {		// check inheritance matching
				symbol_t * base = symbol->data->base;

				// check that derived kind only inherits from appropriate base kind

				bool err = false;
				if ( ( symbol->data->key == STRUCT || symbol->data->key == CLASS ) && ! symbol->data->attribute.Mutex ) {
					if ( ! ( ( base->data->key == STRUCT || base->data->key == CLASS ) && ! base->data->attribute.Mutex ) ) err = true; // only struct/class
				} else if ( symbol->data->key == COROUTINE && ! symbol->data->attribute.Mutex ) { // coroutine
					if ( ! ( base->data->key == COROUTINE && ! base->data->attribute.Mutex ) ) err = true; // only coroutine
				} else if ( ( symbol->data->key == STRUCT || symbol->data->key == CLASS ) && symbol->data->attribute.Mutex ) { // monitor
					if ( ! ( base->data->key == STRUCT || base->data->key == CLASS ) ) err = true; // only struct/class/monitor
				} else if ( symbol->data->key == COROUTINE && symbol->data->attribute.Mutex ) { // comonitor
					if ( ! ( base->data->key == COROUTINE || ( ( base->data->key == STRUCT || base->data->key == CLASS ) && base->data->attribute.Mutex ) ) ) err = true; // only coroutine/monitor/comonitor
				} else if ( symbol->data->key == TASK ) {
					if ( ! ( ( ( base->data->key == STRUCT || base->data->key == CLASS ) && base->data->attribute.Mutex ) || base->data->key == TASK ) ) err = true; // only monitor/task
				} // if

				if ( err ) {
					char msg[strlen(symbol->hash->text) + strlen(base->hash->text) + 256];
					sprintf( msg, "derived type \"%s\" of kind \"%s\" is incompatible with the base type \"%s\" of kind \"%s\"; inheritance ignored.",
							 symbol->hash->text, kind( symbol ), base->hash->text, kind( base ) );
					gen_error( ahead, msg );
					symbol->data->base = nullptr;
				} // if
			} // if

			if ( symbol->data->attribute.Mutex && strcmp( symbol->hash->text, "" ) == 0 ) { // anonymous monitor type
				char msg[256];
				sprintf( msg, "cannot create anonymous %s because of the need for constructors and destructors.", kind( symbol ) );
				gen_error( ahead, msg );
			} // if

			gen_class_suffix( symbol );

			match( RC );

			attribute_clause_list();					// optional

			pop_table();								// pop class table
			attribute.plate = nullptr;
		} else {										// ";"
			if ( attribute.plate != nullptr ) {
				delete pop_table();
				attribute.plate = nullptr;
			} // if
		} // if

		return true;
	} // if

	ahead = back; return false;
} // class_specifier


static bool enumerator() {
	token_t * back = ahead;

	if ( identifier() != nullptr || type() != nullptr ) {
		if ( match( '=' ) ) {							// scan off optional enumerator initializer
			for ( ;; ) {
			  if ( eof() ) break;
			  if ( match( LP ) ) {
					if ( match_closing( LP, RP ) ) continue;
					goto fini;
			  } else if ( match( LA ) ) {
					if ( match_closing( LA, RA ) ) continue;
					goto fini;
			  } else if ( match( LC ) ) {
					if ( match_closing( LC, RC ) ) continue;
					goto fini;
			  } else if ( match( LB ) ) {
					if ( match_closing( LB, RB ) ) continue;
					goto fini;
			  } // if
			  if ( check( ',' ) ) break;
			  if ( check( RC ) ) break;
				// must correctly parse type-names to allow the above match_closing check
				if ( type() == nullptr ) {
					scan();
				} // if
			} // for
		} // if
		return true;
	} // if
  fini: ;
	ahead = back; return false;
} // enumerator


static bool enumerator_list() {
	token_t * back = ahead;

	if ( enumerator() ) {
		if ( match( ',' ) ) {
			if ( enumerator_list() ) {
				return true;
			} else {
				return true;
			} // if
		} else {
			return true;
		} // if
	} // if

	ahead = back; return false;
} // enumerator_list


static bool enumerator_specifier() {
	token_t * back = ahead;
	token_t * token;

	if ( match( ENUM ) ) {
		match( STRUCT ) || match( CLASS );
		nested_name_specifier();						// optional

		if ( ( token = identifier() ) != nullptr || ( token = type() ) != nullptr ) { // optional
			make_type( token, token->symbol );
		} // if

		if ( match( ':' ) ) {
			attribute_t attribute;						// dummy
			specifier_list( attribute );				// optional
		} // if

		if ( match( LC ) ) {							// optional
			enumerator_list();
			if ( match( RC ) ) {
				return true;
			} // if
		} else {
			return true;
		} // if

	} // if

	ahead = back; return false;
} // enumerator_specifier


static bool specifier( attribute_t & attribute, bool & rt );


static bool formal_parameter_empty( attribute_t & attribute ) {
	token_t * back = ahead;

	if ( match( RP ) || ( match( VOID ) && match( RP ) ) ) {
		attribute.emptyparms = true;
		return true;
	} // if

	ahead = back; return false;
} // formal_parameter_empty


static bool formal_parameter( bool ctordtor, token_t * token, attribute_t & attribute ) {
	token_t * back = ahead;
	bool pushflag = false;
	symbol_t * symbol;

	// check for a (, don't match because the focus has to be set before scanning the next token, which may be a type

	if ( check( LP ) ) {
		if ( token != nullptr ) {
			symbol = token->symbol;
			if ( ctordtor ) {							// constructor/destructor routine ?
				uassert( symbol->value == TYPE );
				if ( symbol != focus->symbol && ( focus->lexical == nullptr || symbol != focus->lexical->symbol ) ) { // not inlined definition ?
					if ( attribute.plate == nullptr ) {	// non-template constructor ?
						pushflag = true;
						// place class containing constructor above current lexical context
						symbol->data->table->push_table();
					} else {
						// place class containing constructor below template lexical context
						attribute.plate->lexical = symbol->data->table;
					} // if
				} // if
			} else if ( symbol->data->found != nullptr ) { // not inlined definition ?
				// if a template function, use the enclosing scope to store the symbol.
				table_t * containing = focus == attribute.plate ? focus->lexical : focus;

				// lookup the function name again to determine if it was specified using a nested name (qualified).  (this
				// could have been accomplished by creating a flag in attribute to indicate a nested name.)
				symbol_t * checksym = containing->search_table( symbol->hash );

				// if not found or found in a different table => nested naming => push table identifier found in
				if ( checksym == nullptr || checksym->data->found != symbol->data->found ) {
					attribute.nestedqual = true;
					if ( symbol->data->found != focus && symbol->data->found != focus->lexical ) { // not inlined definition ?
						if ( attribute.plate == nullptr ) { // non-template routine ?
							pushflag = true;
							// place class/namespace containing routine above current lexical context
							symbol->data->found->push_table();
						} else {
							// place class/namespace containing routine below template lexical context
							attribute.plate->lexical = symbol->data->found;
						} // if
					} // if
				} // if
			} // if
		} // if

		attribute.startParms = ahead->next_parse_token();
		match( LP );
		// special case to detect default constructor
		if ( formal_parameter_empty( attribute ) ) {
			if ( pushflag ) {
				pop_table();
				//cerr << "formal_parameter pop2:" << endl;
			} // if
			attribute.endParms = ahead->prev_parse_token();
			return true;
		} // if

		// Quick check to determine if this is a formal parameter list by looking at the first token. If the first token
		// forms any part of a parameter declaration, this must be a formal declaration context. If the first token is a
		// type name nested in a class containing this member routine, the symbol table focus has to be correct. It
		// should have been set when the function name was looked up.

		bool ts = false;								// dummy variables for this check
		attribute_t attr;
		if ( specifier( attr, ts ) || match( DOT_DOT_DOT ) ) {
			if ( pushflag ) {
				pop_table();
				//cerr << "formal_parameter pop2:" << endl;
			} // if

			// Having determined that that this construct is a formal argument list, simply scan for the closing
			// parentheses.

			if ( match_closing( LP, RP ) ) {
				attribute.endParms = ahead->prev_parse_token();
				return true;
			} // if
		} else {
			if ( pushflag ) {
				pop_table();
				//cerr << "formal_parameter pop3:" << endl;
			} // if
		} // if
	} // if

	ahead = back; return false;
} // formal_parameter


static bool ptr_operator() {
	token_t * back = ahead;

	if ( match( '*' ) || match( '&' ) || match( AND_AND ) ) {
		cv_qualifier_list();							// optional
		return true;
	} else {
		if ( check( COLON_COLON ) ) {					// start search at the root
			uDEBUGPRT( print_focus_change( "pointer", focus, root ); )
			focus = root;
			match( COLON_COLON );
		} // if
		if ( nested_name_specifier() != nullptr ) {		// must be type names
			// nested_name_specifier leaves the focus set to the specified class
			uDEBUGPRT( print_focus_change( "pointer", focus, top->tbl ); )
			focus = top->tbl;
			if ( match( '*' ) ) {
				cv_qualifier_list();					// optional
				return true;
			} // if
		} // if
	} // if

	ahead = back; return false;
} // ptr_operator


static bool typeof_specifier() {						// gcc typeof specifier
	token_t * back = ahead;

	if ( (match( TYPEOF ) || match( DECLTYPE )) && match( LP ) && match_closing( LP, RP ) ) return true;

	ahead = back; return false;
} // typeof_specifier


static bool underlying_type_specifier() {				// gcc underlying_type specifier
	token_t * back = ahead;

	if ( match( UNDERLYING_TYPE ) && match( LP ) && match_closing( LP, RP ) ) return true;

	ahead = back; return false;
} // underlying_type_specifier


static bool attribute_clause() {						// gcc/C++11 attribute clause
	token_t * back = ahead;

	if ( match( ATTRIBUTE ) && match( LP ) && match_closing( LP, RP ) )	return true;
	if ( match( ALIGNAS ) && match( LP ) && match_closing( LP, RP ) ) return true;
	if ( match( LB ) && match( LB ) && match_closing( LB, RB ) && match( RB ) ) return true;

	ahead = back; return false;
} // attribute_clause


static bool attribute_clause_list() {
	while ( attribute_clause() );
	return true;
} // attribute_clause_list


static bool asm_clause() {								// gcc asm clause
	token_t * back = ahead;

	if ( match( ASM ) && match( LP ) && match_closing( LP, RP ) ) return true;

	ahead = back; return false;
} // asm_clause


static bool cv_qualifier_list() {
	// over constrained to handle multiple contexts
	while ( match( CONST ) || match( VOLATILE ) || match( RESTRICT ) ||
			match( ATOMIC ) || match( FINAL ) || match( OVERRIDE ) ||
			attribute_clause() || asm_clause() || exception_list() ||
			match( '&' ) || match( AND_AND )			// reference qualifier
		);
	return true;
} // cv_qualifier_list


static bool throw_clause() {
	token_t * back = ahead;

	if ( match( THROW ) && match( LP ) && match_closing( LP, RP ) ) return true;

	ahead = back; return false;
} // throw_clause


static bool noexcept_clause() {							// C++11
	token_t * back = ahead;

	if ( match( NOEXCEPT ) ) {
		if ( ! match( LP ) ) return true;				// optional condition
		if ( match_closing( LP, RP ) ) return true;		// condition
	} // if

	ahead = back; return false;
} // noexcept_clause


static bool exception_list() {
	token_t * back = ahead;

	if ( throw_clause() ) return true;
	if ( noexcept_clause() ) return true;				// C++11

	ahead = back; return false;
} // exception_list


static bool array_dimension() {
	token_t * back = ahead;

	if ( match( LB ) && match_closing( LB, RB ) ) {
		array_dimension();								// 1 or more
		return true;
	} // if

	ahead = back; return false;
} // array_dimension


static void more_declarator( attribute_t & attribute ) {
	if ( array_dimension() ) {							// array dimension
		more_declarator( attribute );
	} else if ( match( LP ) ) {							// parameter list
		if ( match_closing( LP, RP ) ) {
			more_declarator( attribute );
		} // if
	} else if ( match( ':' ) ) {						// bit slice declarator
		// constant-expression cannot contain a ',', i.e., no comma expression or function call
		while ( ! check( ',' ) && ! check( ';' ) && ! check( '}' ) ) {
			scan();
		} // while
	} // if
	cv_qualifier_list();								// optional
} // more_declarator


static token_t * declarator( attribute_t & attribute ) {
	token_t * token;

	attribute_clause_list();							// optional

	if ( match( LP ) ) {
		if ( ( token = declarator( attribute ) ) != nullptr ) {
			if ( match( RP ) ) {
				more_declarator( attribute );
				return token;
			} // if
		} // if
	} else if ( ptr_operator() ) {
		if ( check( ')' ) || check( ';' ) ) {			// abstract declaration
			more_declarator( attribute );
			return ABSTRACT;
		} // if
		if ( ( token = declarator( attribute ) ) != nullptr ) {
			more_declarator( attribute );
			return token;
		} // if
	} else if ( array_dimension() ) {					// array dimension
		more_declarator( attribute );
		return ABSTRACT;
	} else if ( ( token = identifier() ) != nullptr || ( token = operater() ) != nullptr || ( token = type() ) != nullptr ) {
		attribute.startI = token;
		more_declarator( attribute );
		return token;
	} else if ( match( ':' ) ) {						// anonymous bit slice declarator
		// constant-expression cannot contain a ',', i.e., no comma expression or function call
		while ( ! check( ',' ) && ! check( ';' ) && ! check( '}' ) ) {
			scan();
		} // while
		return nullptr;
	} // if

	return nullptr;
} // declarator


static token_t * paren_identifier( attribute_t & attribute ) {
	token_t * back = ahead;
	token_t * token;

	if ( ( token = identifier() ) != nullptr || ( token = operater() ) != nullptr || ( token = type() ) != nullptr ) {
		return token;
	} else if ( match( '(' ) && ( token = paren_identifier( attribute ) ) && match( ')' ) ) return token; // redundant parenthesis

	ahead = back; return nullptr;
} // paren_identifier


static token_t * function_ptr( attribute_t & attribute );
static token_t * function_no_ptr( attribute_t & attribute );
static token_t * function_array( attribute_t & attribute );

static token_t * function_declarator( attribute_t & attribute ) {
	token_t * back = ahead;
	token_t * token;

	attribute_clause_list();							// optional

	// reset the focus of the scanner to the top table, and return the token that points to the type.

	if ( ( token = function_no_ptr( attribute ) ) || ( token = function_array( attribute ) ) || ( token = function_ptr( attribute ) ) ) {
		return token;
	} // if

	ahead = back; return nullptr;
} // function_declarator


static token_t * function_no_ptr( attribute_t & attribute ) {
	token_t * back = ahead;
	token_t * token;

	if ( ( token = paren_identifier( attribute ) ) && formal_parameter( false, token, attribute ) ) return token;
	else {
		ahead = back;
		if ( match( '(' ) ) {
			token_t * back = ahead;
			if ( ptr_operator() && match( ')' ) && formal_parameter( false, token, attribute ) ) return ABSTRACT;
			else if ( ( token = function_ptr( attribute ) ) && match( ')' ) && formal_parameter( false, token, attribute ) ) return token;
			else {
				ahead = back;
				if ( ( token = function_no_ptr( attribute ) ) && match( ')' ) ) return token; // redundant parenthesis
			} // if
		} // if
	} // if

	ahead = back; return nullptr;
} // function_no_ptr


static token_t * function_ptr( attribute_t & attribute ) {
	token_t * back = ahead;
	token_t * token;

	if ( ptr_operator() ) {
		if ( ( token = function_declarator( attribute ) ) ) return token;
	} else {
		ahead = back;
		if ( match( '(' ) && ( token = function_ptr( attribute ) ) && match( ')' ) ) return token;
	} // if

	ahead = back; return nullptr;
} // function_ptr

static token_t * function_array( attribute_t & attribute ) {
	token_t * back = ahead;
	token_t * token;

	if ( match( '(' ) ) {
		token_t * back = ahead;
		if ( ( token = function_ptr( attribute ) ) && match( ')' ) && array_dimension() ) return token;
		else {
			ahead = back;
			if ( ( token = function_array( attribute ) ) && match( ')' ) ) { // redundant parenthesis
				array_dimension();						// optional
				return token;
			} // if
		} // if
	} // if

	ahead = back; return nullptr;
} // function_array


static bool copy_constructor( const bool explict, const token_t * token, attribute_t & attribute ) {
	token_t * back = ahead;
	symbol_t * symbol;
	bool pushflag = false;

	symbol = token->symbol;
	if ( symbol == nullptr || symbol->data == nullptr || symbol->data->table == nullptr ) return false;

	if ( check( LP ) ) {
		if ( symbol != focus->symbol && ( focus->lexical == nullptr || symbol != focus->lexical->symbol ) ) { // not inlined definition ?
			if ( attribute.plate == nullptr ) {			// non-template constructor ?
				pushflag = true;
				// place class containing constructor above current lexical context
				symbol->data->table->push_table();
			} else {
				// place class containing constructor below template lexical context
				attribute.plate->lexical = symbol->data->table;
			} // if
		} // if

		match( LP );

		cv_qualifier_list();							// optional
		token_t * temp = type();
		if ( temp != nullptr && temp->symbol == token->symbol ) {
			if ( (match( CONST ) && match( '&' )) || match( '&' ) || match( AND_AND ) ) { // looks good so far
				int commas = 0;							// if a copy constructor, every comma has a corresponding '=' after it
				int level = 1;
				match( IDENTIFIER );					// optional identifier
				for ( ;; ) {
				  if ( eof() ) goto fini;
					if ( match( ',' ) ) {
						commas += 1;
					} else if ( match( '=' ) ) {
						// ignore an '=' in something like X(X & a = b)
						if ( commas > 0 ) {
							commas -= 1;
						} // if
					} else if ( match( LP ) ) {			// match parentheses
						level += 1;
					} else if ( match( RP ) ) {
						level -= 1;
						if ( level == 0 ) break;
					} else
						scan();							// ignore everything else
				} // for
				if ( commas == 0 ) {
					if ( ! explict ) token->symbol->data->table->hascopy = true;
					if ( pushflag ) pop_table();
					return true;
				} // if
			} // if
		} // if
	} // if
  fini:
	if ( pushflag ) pop_table();
	ahead = back; return false;
} // copy_constructor


static bool function_return_type( attribute_t & attribute );

static token_t * constructor_declarator( token_t *& rp, attribute_t & attribute ) {
	token_t * back = ahead;
	token_t * token;

	if ( ( token = type() ) != nullptr ) {
		// constructors and destructors are not put in the symbol table.  the class symbol table is the one set for
		// further name look ups.

		if ( copy_constructor( attribute.dclqual.qual.EXPLICIT, token, attribute ) || formal_parameter( true, token, attribute ) ) {
			// remember where the right parenthese of the argument list of the constructor is
			rp = ahead->prev_parse_token();
			uassert( rp->value == ')' );
			cv_qualifier_list();						// optional post qualifiers
			function_return_type( attribute );			// optional user-defined deduction guide
			return token;
		} // if
	} // if

	ahead = back; return nullptr;
} // constructor_declarator


static token_t * destructor_declarator( attribute_t & attribute ) {
	token_t * back = ahead;
	token_t * token;

	if ( ( token = epyt() ) != nullptr ) {
		if ( formal_parameter( true, token, attribute ) ) { // should be empty

			cv_qualifier_list();						// optional post qualifiers
			return token;
		} // if
	} // if

	ahead = back; return nullptr;
} // destructor_declarator


static bool body( token_t * function, attribute_t & attribute, symbol_t * symbol ) {
	token_t * back = ahead;

	// check for a {, don't match because a symbol table has to be built before scanning the next token, which may be an
	// identifier

	if ( check( LC ) ) {
		uassert( symbol != nullptr );
		table_t * table;

		if ( attribute.plate != nullptr ) {
			table = attribute.plate;					// already allocated & pushed
			uassert( table->symbol == nullptr );
			table->symbol = symbol;						// connect routine table temporarily to routine symbol
		} else {
			table = new table_t( nullptr );
			table->symbol = symbol;						// connect routine table temporarily to routine symbol
			table->push_table();
		} // if

		if ( function ) {
			table->lexical = symbol->data->found;
		} else {
			// Constructor/destructor do not have symbol-table entries in the class (unlike member routines), so use
			// class symbol-table.
			table->lexical = symbol->data->table;
		} // if

		match( LC );									// now scan passed the '{' for the next token

		symbol->data->attribute.startCR = attribute.startCR;
		symbol->data->attribute.fileCR = attribute.fileCR;
		symbol->data->attribute.lineCR = attribute.lineCR;

		while ( statement( symbol ) );					// declarations/statements in routine body

		// Must remove the current function scope *before* looking up the next
		// identifier after the RC, as in:
		//
		//   foo f() {} bar b() {}
		//			  ^- look up bar in outer scope
		//
		// otherwise the serach starts in the function scope.

		delete pop_table();
		attribute.plate = nullptr;

		match( RC );
		return true;
	} // if

	ahead = back; return false;
} // body


static bool pure_specific() {
	token_t * back = ahead;

	if ( match( '=' ) ) {
		if ( match( NUMBER ) ) {
			return true;
		} // if
	} // if

	ahead = back; return false;
} // pure_specific


static bool delete_default_specific() {
	token_t * back = ahead;

	if ( match( '=' ) ) {
		if ( match( DELETE ) ) {
			return true;
		} // if
		if ( match( DEFAULT ) ) {
			return true;
		} // if
	} // if

	ahead = back; return false;
} // delete_default_specific


static void make_mutex_class( symbol_t * clss ) {
	uassert( clss != nullptr );

	// If there are no mutex members, make the type mutex (should be checked by caller). Notice, a derived type may find
	// existing mutex members from the base type, and hence, does not recreate declarations that appear in the root base
	// type of the inheritance chain.

	if ( clss->data->base == nullptr || ! clss->data->base->data->attribute.Mutex ) {
		table_t * table = clss->data->table;
		uassert( table != nullptr );

		gen_mutex( table->protected_area, clss );
		gen_mutex_entry( table->protected_area, clss );
		clss->data->index += 1;							// advance to next bit in the mutex mask for mutex type (for mutex destructor)
	} // if
} // make_mutex_class


static void make_mutex_member( symbol_t * symbol ) {
	table_t * table;
	symbol_t * clss;

	uassert( symbol != nullptr );
	table = symbol->data->found;
	uassert( table != nullptr );
	clss = table->symbol;
	uassert( clss != nullptr );

	// if the containing type is not mutex, make it so now to ensure the destructor mutex is numbered DESTRUCTORPOSN

	if ( ! clss->data->attribute.Mutex ) {
		clss->data->attribute.Mutex = true;
		make_mutex_class( clss );
	} // if

	symbol->data->attribute.Mutex = true;				// mark routine as mutex member
	symbol->data->index = clss->data->index;			// copy current position in mutex bit mask
	gen_mutex_entry( table->protected_area, symbol );
	clss->data->index += 1;								// advance to next bit in the mutex mask for mutex type
} // make_mutex_member


// Constructs a string corresponding to the attribute.

static const char * attr_string( attribute_t & attribute ) {
	if ( attribute.dclkind.kind.TYPEDEF ) return "typedef";
	if ( attribute.dclkind.kind.FRIEND ) return "friend";
	if ( attribute.dclmutex.qual.MUTEX ) return "mutex";
	if ( attribute.dclmutex.qual.NOMUTEX ) return "nomutex";
	return "(none)";
} // attr_string


static bool function_return_type( attribute_t & attribute ) {
	token_t * back = ahead;

	if ( match( ARROW ) ) {
		if ( specifier_list( attribute ) ) {
			declarator( attribute );					// optional abstract
			return true;
		} // if
	} // if 

	ahead = back; return false;
} // function_return_type


static bool function_declaration( bool explict, attribute_t & attribute ) {
	token_t * back = ahead;
	token_t * function;
	symbol_t * symbol;
	table_t * table;
	token_t * prefix, * suffix;
	bool nested_specifier = false;

	// template qualifier removed by specifier

	// if the declarator is a function declarator with an explicit type specifier (return type) or an operator routine
	// (conversion routines do not have explicit return types).

	uassert( top != nullptr );

	function = function_declarator( attribute );
	if ( function != ABSTRACT && ( ( function != nullptr && explict ) || ( function != nullptr && function->symbol->value == OPERATOR ) ) ) {
		cv_qualifier_list();							// optional post qualifiers
		match( '&' );									// optional reference qualifier
		match( AND_AND );
		while ( attribute_clause() || asm_clause() || exception_list() ); // more optional post qualifiers
		pure_specific();								// optional pure specifier for virtual functions

		symbol = function->symbol;						// grab the symbol associated with the token
		uassert( symbol != nullptr );

		// if a template function, use the class/namespace scope to store the symbol.
		table_t * containing = focus == attribute.plate ? focus->lexical : focus;

		// the scope that is (lexically) outside this declaration (for double lookup)
		table_t * outside = top->tbl == attribute.plate ? top->link->tbl : top->tbl;

		if ( symbol->data->found == nullptr ) {
			// the symbol is not defined in the symbol table so define it
			if ( attribute.dclkind.kind.FRIEND ) {		// friend routines are placed into the root namespace
				root->insert_table( symbol );
			} else {
				containing->insert_table( symbol );
			} // if
			nested_specifier = false;
		} else if ( outside != symbol->data->found ) {
			if ( ! attribute.nestedqual ) {
				symbol = new symbol_t( IDENTIFIER, symbol->hash );
				containing->insert_table( symbol );
				nested_specifier = false;
			} else {
				// if found in a different table => nested naming => augmenting existing function definition
				nested_specifier = true;
			} // if
		} // if

		if ( symbol->value != TYPE ) {					// already defined as a struct/class so ignore this definition
			if ( symbol->data->found->symbol != nullptr ) { // member routine ?
				symbol->data->key = MEMBER;
			} else {
				symbol->data->key = ROUTINE;
			} // if
		} // if

		// copy these fields to complete symbol attribute

		symbol->data->attribute.dclqual.value |= attribute.dclqual.value;
		symbol->data->attribute.nestedqual = attribute.nestedqual;
		if ( attribute.plate != nullptr ) {
			symbol->data->attribute.startT = attribute.startT;
			symbol->data->attribute.endT = attribute.endT;
			symbol->data->attribute.plate = attribute.plate;
			symbol->data->attribute.plate->startT = attribute.plate->startT;
			symbol->data->attribute.plate->endT = attribute.plate->endT;
		} // if

		table = symbol->data->found;					// grab the table in which this symbol is found
		uassert( table != nullptr );

		// spend some time determining what attributes are attached to this function.  attributes can be explicitly
		// specified, inherited from the class in which a member function is defined, or specified by the default rules
		// of the language.

		if ( table->symbol != nullptr ) {
			// if this symbol is found in a table that has a symbol associated with it, this table must be a class
			// table.  follow the default rules for other members, constructors and destructors

			if ( nested_specifier && ! attribute.dclmutex.qual.MUTEX && ! attribute.dclmutex.qual.NOMUTEX ) {
				attribute.dclmutex.qual.MUTEX = symbol->data->attribute.dclmutex.qual.MUTEX;
				attribute.dclmutex.qual.NOMUTEX = symbol->data->attribute.dclmutex.qual.NOMUTEX;
			} // if

			if ( ! attribute.dclmutex.qual.MUTEX && ! attribute.dclmutex.qual.NOMUTEX ) { // still don't know ?
				// if no attribute is specified explicitly, infer the attribute of this symbol

				if ( table->access == PUBLIC && strcmp( symbol->hash->text, "new" ) != 0 && strcmp( symbol->hash->text, "delete" ) != 0 ) {
					// if in the public area of a class, use the attribute associated with the class except for the new
					// or delete operators

					uassert( table->symbol != nullptr );
					attribute.dclmutex.qual.MUTEX = table->symbol->data->attribute.dclmutex.qual.MUTEX;
					attribute.dclmutex.qual.NOMUTEX = table->symbol->data->attribute.dclmutex.qual.NOMUTEX;
				} else {
					// if in the private or protected area of a class, use the no mutex attribute

					attribute.dclmutex.qual.NOMUTEX = true;
				} // if
			} else if ( attribute.dclmutex.qual.MUTEX ) {
				if ( strcmp( symbol->hash->text, "new" ) == 0 || strcmp( symbol->hash->text, "delete" ) == 0 ) { // new and delete operators can't be mutex
					char msg[strlen(symbol->hash->text) + 256];
					sprintf( msg, "\"%s\" operator must be nomutex, mutex attribute ignored.", symbol->hash->text );
					gen_error( ahead, msg );
					attribute.dclmutex.qual.MUTEX = false;
				} // if
				if ( attribute.startE != nullptr ) {
					char msg[strlen(symbol->hash->text) + 256];
					sprintf( msg, "mutex qualifier on member \"%s\" can only specify queue types when modifying a class or task, queue types ignored.",
							 symbol->hash->text );
					gen_error( ahead, msg );
				} // if
			} // if

			if ( symbol->data->attribute.dclqual.qual.STATIC && attribute.dclmutex.qual.MUTEX ) {
				char msg[strlen(symbol->hash->text) + 256];
				sprintf( msg, "static member \"%s\" must be nomutex, mutex attribute ignored.", symbol->hash->text );
				gen_error( ahead, msg );
				attribute.dclmutex.qual.MUTEX = false;
			} // if

			// once an attribute is determined, assign this attribute to the symbol if it does not have an attribute
			// yet, else check that the attributes match

			if ( ! symbol->data->attribute.dclmutex.qual.MUTEX && ! symbol->data->attribute.dclmutex.qual.NOMUTEX ) {
				symbol->data->attribute.dclmutex.qual.MUTEX = attribute.dclmutex.qual.MUTEX;
				symbol->data->attribute.dclmutex.qual.NOMUTEX = attribute.dclmutex.qual.NOMUTEX;
			} else if ( symbol->data->attribute.dclmutex.qual.MUTEX != attribute.dclmutex.qual.MUTEX &&
						symbol->data->attribute.dclmutex.qual.NOMUTEX != attribute.dclmutex.qual.NOMUTEX ) {
				// conflicting attributes
				char msg[strlen(attr_string(attribute)) + strlen(table->symbol->hash->text) + strlen(symbol->hash->text) +
						 strlen(attr_string(symbol->data->attribute)) + 256];
				sprintf( msg, "%s attribute of \"%s::%s\" conflicts with previously declared",
						 attr_string(attribute), table->symbol->hash->text, symbol->hash->text );
				sprintf( &msg[strlen(msg)], " %s attribute.",
						 attr_string(symbol->data->attribute) );
				gen_error( ahead, msg );
			} // if

			// if the symbol has a mutex attribute, make it a mutex member unless it has already been defined in the
			// inheritance hierarchy as mutex, as any mutex redefinition inherits the same mutex bit from the original
			// definition.

			if ( symbol->data->attribute.dclmutex.qual.MUTEX && ! symbol->data->attribute.Mutex ) {
				make_mutex_member( symbol );
			} // if
		} else if ( attribute.dclmutex.qual.MUTEX || attribute.dclmutex.qual.NOMUTEX ) {
			// assigning a (no)Mutex attribute to a function that's not a class member

			char msg[strlen(symbol->hash->text) + 256];
			sprintf( msg, "routine \"%s\" not a class member, %smutex attribute ignored.",
					 symbol->hash->text, attribute.dclmutex.qual.MUTEX ? "" : "no" );
			gen_error( ahead, msg );
			symbol->data->attribute.dclmutex.qual.MUTEX = false;
			symbol->data->attribute.dclmutex.qual.NOMUTEX = false;
		} // if

		prefix = ahead;

		function_return_type( attribute );				// optional

		if ( body( function, attribute, symbol ) ) {
			suffix = ahead;
			uassert( table != nullptr );
			uassert( function != nullptr );
			uassert( function->hash != nullptr );
			// routines/members named "main" require analysis
			if ( strcmp( function->hash->text, "main" ) == 0 ) {
				if ( table->symbol != nullptr ) {		// member ?
					if ( ( table->symbol->data->key == COROUTINE || table->symbol->data->key == TASK ) ) {
						if ( strcmp( table->symbol->hash->text, "uMain" ) == 0 ) {
							gen_error( ahead, "explicit declaration of uMain::main deprecated and replaced by program main: int main(...)." );
						} // if
						if ( table->access == PUBLIC ) { // public member ?
							gen_code( prefix->next_parse_token(), "static_assert( false, \"main member of coroutine or task cannot have public access.\" );" );
						} else {
							gen_main_prefix( prefix, symbol );
							gen_main_suffix( suffix, symbol );
						} // if
					} // if
				} else {				// routine
					// Convert main routine into another named routine called by uMain::main, so creating uMain::main is unnecessary.

					// return type must be int
					if ( attribute.startRet->next_parse_token() == attribute.endRet && strcmp( attribute.startRet->hash->text, "int" ) == 0 ) {
						// empty parameters or first type must be int types (misses const after int)
						int commas = 0, stars1 = 0, stars2 = 0, openbracket = 0, closebracket = 0;
						// scan parameters for correct types and number
						token_t * p, * argcstart = nullptr, * argcend = nullptr, * argvstart = nullptr, * argvend = nullptr, * envstart = nullptr, * envend = nullptr;
						bool foundint = false, foundchar = false;

						if ( ! attribute.emptyparms ) {
							// handle argc
							argcstart = attribute.startParms;
							for ( p = attribute.startParms; p != attribute.endParms && strcmp( p->hash->text, "," ) != 0; p = p->next_parse_token() ) {
								if ( strcmp( p->hash->text, "int" ) == 0 ) foundint = true;
							} // for
							if ( strcmp( p->hash->text, "," ) != 0 && ! foundint ) goto fini;
							argcend = p;
							commas += 1;
							// handle argv
							argvstart = p->next_parse_token();
							for ( p = p->next_parse_token(); p != attribute.endParms && strcmp( p->hash->text, "," ) != 0; p = p->next_parse_token() ) {
								if ( strcmp( p->hash->text, "char" ) == 0 ) foundchar = true;
								if ( strcmp( p->hash->text, "*" ) == 0 ) stars1 += 1;
								if ( strcmp( p->hash->text, "[" ) == 0 ) openbracket += 1;
								if ( strcmp( p->hash->text, "]" ) == 0 ) closebracket += 1;
							} // for
							if ( ! ( foundchar && ( stars1 == 2 || ( stars1 == 1 && openbracket == 1 && closebracket == 1 ) ) ) ) goto fini;
							argvend = p;
							if ( strcmp( p->hash->text, "," ) == 0 ) { // has env ?
								foundchar = false;
								openbracket = closebracket = 0;
								commas += 1;
								// handle env
								envstart = p->next_parse_token();
								for ( p = p->next_parse_token(); p != attribute.endParms; p = p->next_parse_token() ) {
									if ( strcmp( p->hash->text, "char" ) == 0 ) foundchar = true;
									if ( strcmp( p->hash->text, "*" ) == 0 ) stars2 += 1;
									if ( strcmp( p->hash->text, "[" ) == 0 ) openbracket += 1;
									if ( strcmp( p->hash->text, "]" ) == 0 ) closebracket += 1;
								} // for
								if ( ! ( foundchar && ( stars2 == 2 || ( stars2 == 1 && openbracket == 1 && closebracket == 1 ) ) ) ) goto fini;
								envend = p;
							} // if
						} // if
						gen_code( suffix->prev_parse_token(), "return 0 ;" ); // add return to prevent warning
						function->hash = hash_table->lookup( "uCpp_main" ); // change routine name
						gen_code( suffix, "void uMain :: main ( ) { try { uRetCode = uCpp_main (" ); // generate uMain::main

						if ( commas >= 1 ) {			// has argc, argv ?
							gen_code( suffix, "(" );
							copy_tokens( suffix, argcstart, argcend->prev_parse_token() ); // copy type for cast
							gen_code( suffix, ") argc ," );
							gen_code( suffix, "(" );
							// backup over "name [ ]" tokens
							if ( stars1 == 1 ) argvend = argvend->prev_parse_token()->prev_parse_token();
							copy_tokens( suffix, argvstart, argvend->prev_parse_token() ); // copy type for cast
							if ( stars1 == 1 ) gen_code( suffix, "*" ); // change [] to *
							gen_code( suffix, ") argv" );
							if ( commas == 2 ) {		// has env ?
								gen_code( suffix, ", (" );
								// backup over "name [ ]" tokens
								if ( stars2 == 1 ) envend = envend->prev_parse_token()->prev_parse_token();
								copy_tokens( suffix, envstart, envend->prev_parse_token() ); // copy type for cast
								if ( stars2 == 1 ) gen_code( suffix, "*" ); // change [] to *
								gen_code( suffix, ") env" );
							} // if
						} // if
						gen_code( suffix, ") ; } catch( uBaseCoroutine::UnhandledException & ex ) { ex.defaultTerminate(); } catch( ... ) { if ( ! cancelInProgress() ) cleanup( *this ); } }" );
					} // if
				  fini: ;
				} // if
			} else if ( strcmp( function->hash->text, "uCpp_real_main" ) == 0 && table->symbol == nullptr ) { // routine ?
				function->hash = hash_table->lookup( "main" );
			} else if ( symbol->data->attribute.Mutex ) {
				gen_member_prefix( prefix, symbol );
				gen_member_suffix( suffix );
			} // if
			return true;
		} else if ( match( ';' ) ) {
			if ( strcmp( function->hash->text, "main" ) == 0 ) {
				if ( table->symbol != nullptr ) {		// member ?
					if ( ( table->symbol->data->key == COROUTINE || table->symbol->data->key == TASK ) && table->access == PUBLIC ) { // public member ?
						gen_error( prefix, "main member of coroutine or task cannot have \"public\" access." );
					} // if
				} else {								// routine
					function->hash = hash_table->lookup( "uCpp_main" );
				} // if
			} // if
			if ( attribute.plate != nullptr ) {
				delete pop_table();
				attribute.plate = nullptr;
			} // if
			if ( attribute.dclkind.kind.TYPEDEF ) {
				make_type( function, symbol );
			} // if
			return true;
		} // if
	} // if

	ahead = back; return false;
} // function_declaration


static bool member_initializer( symbol_t * symbol, token_t *& start ) {
	token_t * back = ahead;
	token_t * token;
	token_t * rp;

	if ( ( token = type() ) != nullptr ) {
		if ( condition() || ( match( LC ) && match_closing( LC, RC ) ) ) { // argument list
			uassert( symbol != nullptr );
			symbol_t * base = token->symbol;
			uassert( base != nullptr );
			if ( symbol->data->base == base ) {
				base->data->used = true;
				if ( base->data->key == COROUTINE || base->data->key == TASK || base->data->attribute.Mutex || base->data->key == ACTOR || base->data->key == CORACTOR ) {
					rp = ahead->prev_parse_token();
					if ( rp->prev_parse_token()->value == LP || rp->prev_parse_token()->value == LC ) {
						gen_code( rp, "UPP :: uNo" );
					} else {
						gen_code( rp, ", UPP :: uNo" );
					} // if
				} // if
			} // if
			return true;
		} // if
	} else if ( ( token = identifier() ) != nullptr ) {
		if ( condition() || ( match( LC ) && match_closing( LC, RC ) ) ) { // argument list
			// first non-base-class constructor
			if ( start == nullptr ) start = token;
			return true;
		} // if
	} // if

	ahead = back; return false;
} // member_initializer


static bool member_initializer_list( symbol_t * symbol, token_t *& start ) {
	token_t * back = ahead;

	if ( member_initializer( symbol, start ) ) {
		if ( match( ',' ) ) {
			if ( member_initializer_list( symbol, start ) ) {
				return true;
			} // if
		} else {
			return true;
		} // if
	} // if

	ahead = back; return false;
} // member_initializer_list


static bool constructor_initializer( symbol_t * symbol, token_t *& start ) {
	token_t * back = ahead;

	if ( match( ':' ) ) {
		uassert( symbol != nullptr );

		// if a base class exists, clear the used flag for now

		if ( symbol->data->base != nullptr ) {
			symbol->data->base->data->used = false;
		} // if

		if ( member_initializer_list( symbol, start ) ) {
			return true;
		} // if
	} // if

	ahead = back; return false;
} // constructor_initializer


static bool constructor_declaration( attribute_t & attribute ) {
	token_t * back = ahead;
	token_t * constructor;
	symbol_t * symbol;
	table_t * table;
	token_t * prefix, * suffix;
	token_t * start = nullptr;
	token_t * rp;

	bool initializer;

	while ( type_qualifier( attribute ) || template_qualifier( attribute ) || attribute_clause() );

	prefix = ahead;
	if ( ( constructor = constructor_declarator( rp, attribute ) ) != nullptr ) {
		symbol = constructor->symbol;
		uassert( symbol != nullptr );
//	  if ( symbol != focus->symbol ) goto fini;		// type must be the same as containing class
		table = symbol->data->table;

		if ( attribute.dclmutex.qual.NOMUTEX && symbol->data->attribute.Mutex ) { // can only be MUTEX
			gen_error( prefix, "constructor must be mutex, nomutex attribute ignored." );
		} // if

		if ( table != nullptr ) {						// must be complete type
			if ( attribute.emptyparms ) {				// default constructor ?
				symbol->data->table->hasdefault = true;
			} // if

			if ( delete_default_specific() ) {			// optional delete/default
				if ( ahead->aft->value == DEFAULT && attribute.emptyparms && symbol->data->attribute.Mutex ) {
					for ( unsigned int i = 0; i < 5; i += 1 ) { // remove tokens: T ( ) = default
						ahead->aft->remove_token();
					} // for
					return true;
				} // if
			} // if

			if ( match( TRY ) ) {						// exceptions for constructor body ?
				gen_error( ahead, "try block for constructor body not supported." );
			} // if

			// eat the constructor initializers.  remember if we saw one

			initializer = constructor_initializer( symbol, start );
			prefix = ahead;

			// there is no reason to assign the mutex qualifier to this symbol because it is always NO_ATTRIBUTE.

			if ( body( nullptr, attribute, symbol ) ) {
				bool dummy;
				bool d1;
				hash_t * d2;
				symbol_t * d3;
				tokenlist d4,d5;
				catch_clause( symbol, dummy, d1, d2, d3, d4, d5 ); // exception for initializers

				suffix = ahead->prev_parse_token();

				if ( table->defined ) {
					if ( ! initializer ) {
						// if there was no initializer, may have some work to do to initialize the class

						gen_initializer( prefix, symbol, ':', false );
					} else {
						// if a base class exists, and the used flags is still cleared, it means the user has not called
						// its constructor explicitly.  that means the translator has to add a call to its constructor.

						if ( symbol->data->base != nullptr && ! symbol->data->base->data->used ) {
							if ( start == nullptr ) {
								gen_initializer( prefix, symbol, ',', false );
							} else {
								gen_initializer( start, symbol, ',', true );
							} // if
						} // if
					} // if
					gen_constructor_parameter( rp, symbol, false );
					gen_serial_initializer( rp, prefix, start, symbol );
					gen_constructor_prefix( prefix, symbol );
					gen_constructor_suffix( suffix, symbol );
				} else {
					// inline definition, do not know if mutex
					structor_t * structor = new structor_t;
					uassert( structor != nullptr );
					if ( ! initializer ) {
						structor->separator = ':';
					} else if ( symbol->data->base != nullptr && ! symbol->data->base->data->used ) {
						structor->separator = ',';
					} else {
						structor->separator = '\0';
					} // if
					structor->rp = rp;
					structor->defarg = false;
					structor->start = start;
					structor->prefix = prefix;
					structor->suffix = suffix;
					table->constructor.add_structor( structor );
				} // if
				return true;
			} else if ( match( ';' ) ) {
				if ( attribute.plate != nullptr ) {
					delete pop_table();
					attribute.plate = nullptr;
				} // if

				uassert( table != nullptr );

				if ( ! table->defined ) {
					// inline definition, do not know if mutex
					structor_t * structor = new structor_t;
					uassert( structor != nullptr );
					structor->rp = rp;
					structor->defarg = true;
					structor->prefix = nullptr;
					structor->suffix = nullptr;
					table->constructor.add_structor( structor );
				} // if

				return true;
			} // if
		} // if
	} // if
//  fini:
	unscan( back ); return false;
} // constructor_declaration


static bool destructor_declaration( attribute_t & attribute ) {
	token_t * back = ahead;
	token_t * destructor;
	symbol_t * symbol;
	table_t * table;
	token_t * prefix, * suffix;

	while ( type_qualifier( attribute ) || template_qualifier( attribute ) || attribute_clause() );

	if ( ( destructor = destructor_declarator( attribute ) ) != nullptr ) {
		symbol = destructor->symbol;
		uassert( symbol != nullptr );
		uassert( focus == top->tbl );
//	if ( symbol != focus->symbol ) goto fini;			// type must be the same as containing class
		table = symbol->data->table;

		if ( table != nullptr ) {						// must be complete type
			if ( delete_default_specific() ) {			// optional delete/default
				if ( ahead->aft->value == DEFAULT && symbol->data->attribute.Mutex ) {
					for ( unsigned int i = 0; i < 6; i += 1 ) { // remove tokens: ~ T ( ) = default
						ahead->aft->remove_token();
					} // for
					return true;
				} // if
			} // if

			prefix = ahead;
			if ( attribute.dclmutex.qual.MUTEX ) {
				if ( ! symbol->data->attribute.Mutex ) {
					symbol->data->attribute.Mutex = true;
					make_mutex_class( symbol );
				} // if
			} // if

			// check for incorrect mutex qualifier of the destructor is performed during destructor code generation
			// because a (nomutex) class may become mutex if it has a mutex member.

			if ( body( nullptr, attribute, symbol ) ) {
				suffix = ahead->prev_parse_token();

				if ( table->defined ) {
					gen_destructor_prefix( prefix, symbol );
					gen_destructor_suffix( suffix, symbol );
				} else {
					structor_t * structor = new structor_t();

					uassert( structor != nullptr );
					structor->prefix = prefix;
					structor->suffix = suffix;
					structor->dclmutex.value = attribute.dclmutex.value;
					table->destructor.add_structor( structor );
				} // if
				return true;
			} else if ( match( ';' ) ) {
				if ( attribute.plate != nullptr ) {
					delete pop_table();
					attribute.plate = nullptr;
				} // if

				uassert( table != nullptr );

				if ( ! table->defined ) {
					structor_t * structor = new structor_t();
					uassert( structor != nullptr );

					structor->prefix = nullptr;
					structor->suffix = nullptr;
					structor->dclmutex.value = attribute.dclmutex.value;
					table->destructor.add_structor( structor );
				} // if

				return true;
			} // if
		} // if
	} // if
//  fini:
	ahead = back; return false;
} // destructor_declaration


static bool string_literal() {
	token_t * back = ahead;

	if ( match( STRING ) ) {
		while ( match( STRING ) );
		return true;
	} // if

	ahead = back; return false;
} // string_literal


static bool simple_type_specifier() {
	token_t * back = ahead;

	if ( match( INT ) ) return true;
	if ( match( CHAR ) ) return true;
	if ( match( SHORT ) ) return true;
	if ( match( LONG ) ) return true;
	if ( match( SIGNED ) ) return true;
	if ( match( UNSIGNED ) ) return true;
	if ( match( VOID ) ) return true;
	if ( match( BOOL ) ) return true;
	if ( match( FLOAT ) ) return true;
	if ( match( DOUBLE ) ) return true;
	if ( match( COMPLEX ) ) return true;				// gcc specific
	if ( match( CHAR16_t ) ) return true;				// C++11
	if ( match( CHAR32_t ) ) return true;				// C++11
	if ( match( WCHAR_T ) ) return true;
	if ( match( AUTO ) ) return true;					// C++11
	if ( typeof_specifier() ) return true;				// gcc specific
	if ( underlying_type_specifier() ) return true;		// gcc specific

	ahead = back; return false;
} // simple_type_specifier


static bool elaborated_type_specifier( attribute_t & attribute ) {
	token_t * back = ahead;
	token_t * token;

	if ( class_key( attribute ) || match( ENUM ) || match( TYPENAME ) ) {
		attribute_clause_list();						// optional

		if ( check( COLON_COLON ) ) {					// start search at the root
			uDEBUGPRT( print_focus_change( "elaborated_type_specifier1", focus, root ); )
			focus = root;
			match( COLON_COLON );
		} // if
		for ( ;; ) {
		  if ( eof() ) break;
			token = ahead;
			if ( check( IDENTIFIER ) || check( TYPE ) ) {
				// reset the focus of the scanner to the top table as the next symbol may not be in this focus.
				uDEBUGPRT( print_focus_change( "type2", focus, top->tbl ); )
				focus = top->tbl;
				scan();									// handles IDENTIFIER or TYPE
				template_key();

				if ( check( COLON_COLON ) ) {
					// for typename, the qualification list is not always defined
					if ( token->symbol != nullptr && token->symbol->data->table != nullptr ) {
						uDEBUGPRT( print_focus_change( "elaborated_type_specifier2", focus, token->symbol->data->table ); )
						focus = token->symbol->data->table;
					} // if
					match( COLON_COLON );
				} else {
					return true;
				} // if
			} else {
				if ( match( TEMPLATE ) ) {
					if ( check( IDENTIFIER ) ) {
						ahead->value = TYPE;
					} // if
					continue;							// restart for the template name
				} // if
				break;
			} // if
		} // for
	} // if

	//uDEBUGPRT( print_focus_change( "elaborated_type_specifier3", focus, top->tbl ); )
	focus = top->tbl;
	ahead = back; return false;
} // elaborated_type_specifier


static bool type_specifier( attribute_t & attribute, bool & ts ) {
	token_t * back = ahead;
	token_t * name;

	if ( simple_type_specifier() ) { ts = true; return true; }
	if ( ! ts && ( name = type() ) != nullptr ) { attribute.typedef_base = name->symbol; ts = true; return true; }
	if ( ! ts && enumerator_specifier() ) { ts = true; return true; }
	if ( ! ts && class_specifier( attribute ) ) { ts = true; return true; }
	if ( ! ts && elaborated_type_specifier( attribute ) ) { ts = true; return true; }
	if ( attribute_clause() ) return true;				// gcc specific
	// handle builtin g++ types
//	if ( ! ts && strncmp( ahead->hash->text, "__builtin_", 10 ) == 0 ) { scan(); ts = true; return true; }
	if ( ! ts && ( name = identifier() ) != nullptr ) { ts = true; return true; }

	ahead = back; return false;
} // type_specifier


static bool specifier( attribute_t & attribute, bool & ts ) {
	token_t * back = ahead;

	if ( template_qualifier( attribute ) ) return true;
	if ( type_qualifier( attribute ) ) return true;
	if ( type_specifier( attribute, ts ) ) return true;

	ahead = back; return false;
} // specifier


static bool specifier_list( attribute_t & attribute ) {
	token_t * back = ahead;
	bool ts = false;									// indicates when a type specifier is found

	if ( specifier( attribute, ts ) ) {
		attribute.startRet = back;
		while ( specifier( attribute, ts ) );
		attribute.endRet = ahead;
		return ts;
	} // if

	ahead = back; return ts;
} // specifier_list


static bool lambda() {
	token_t * back = ahead;

	if ( stdcpp11 && match( LB ) && match_closing( LB, RB ) ) {	// C++11 lambda
		if ( match( LP ) && ! match_closing( LP, RP ) ) goto fini; // optional
		for ( ;; ) {									// scan off return type
		  if ( eof() ) break;
		  if ( check( LC ) )  break;
			scan();
		} // for
		symbol_t symbol( IDENTIFIER, hash_table->lookup( "" ) ); // fake symbol
		if ( compound_statement( &symbol, nullptr, 0 ) ) return true;
	} // if
  fini:
	ahead = back; return false;
} // lambda


static bool initexpr() {
	token_t * back = ahead;

	lambda();
	for ( ;; ) {										// scan off the expression
	  if ( eof() ) break;
		if ( match( LP ) ) {
			if ( match_closing( LP, RP ) ) continue;
			goto fini;
		} else if ( match( LA ) ) {
			if ( match_closing( LA, RA, true, true ) ) continue; // true => ignre less than in expression
			goto fini;
		} else if ( match( LC ) ) {
			if ( match_closing( LC, RC, true ) ) continue;
			goto fini;
		} // if
		if ( check( ',' ) ) return true;				// separator
		if ( check( ';' ) ) return true;				// terminator
		if ( check( RC ) ) return false;				// terminator
		// must correctly parse type-names to allow the above match_closing check
		if ( type() == nullptr ) {
			scan();
		} // if
	} // for
  fini: ;
	ahead = back; return false;
} // initexpr


// initializer:
//	= initializerclause
//	( expressionlist )
//	{ expressionlist }

static bool initializer() {
	token_t * back = ahead;

	if ( match( '=' ) ) {
		if ( match( LC ) ) {
			while ( initexpr() && match( ',' ) );
			if ( match( RC ) ) return true;
		} else if ( initexpr() ) return true;
	} else if ( match( LP ) && match_closing( LP, RP ) ) { // constructor argument
		return true;
	} else if ( stdcpp11 && match( LC ) ) {				// C++11 uniform initialization
		while ( initexpr() && match( ',' ) );
		if ( match( RC ) ) return true;
	} // if
	ahead = back; return false;
} // initializer


static void declarator_list( attribute_t & attribute ) {
	token_t * token;
	symbol_t * symbol;

	token = declarator( attribute );
	if ( token != ABSTRACT && token != nullptr ) {
		symbol = token->symbol;
		uassert( symbol );
		// There is no separate namespace for structures so if target symbol of the typedef is already defined, assume
		// it is a type, which is sufficient to parse declarations.
		if ( attribute.dclkind.kind.TYPEDEF && ( symbol->data->key == 0 || symbol->data->found != nullptr ) ) {
			make_type( token, symbol );

			if ( attribute.typedef_base != nullptr ) {	// null => base type without substructure (e.g., "int")
				// deleting does not work because "struct"s are not is separate name space, so storage is leaked.

				//delete symbol->data;
				symbol->data = attribute.typedef_base->data;
				symbol->typname = symbol->copied = true;
			} // if
		} // if
	} // if

	attribute_clause_list();							// optional
	initializer();										// optional

	if ( match( ',' ) ) {
		declarator_list( attribute );
	} // if
} // declarator_list


static bool exception_declaration( attribute_t & attribute, bound_t & bound ) {
	token_t * back = ahead;

	if ( specifier_list( attribute ) ) {
		bound.oleft = bound.oright = nullptr;
		bound.exbegin = back;
		bound.idleft = ahead;
		declarator( attribute );
		bound.idright = ahead;
		if ( match( RP ) ) return true;
	} // if

	ahead = back; return false;
} // exception_declaration


static bool bound_exception_declaration( bound_t & bound, int kind ) {
	token_t * back = ahead;
	token_t * prefix;

	prefix = ahead;
	if ( match_closing( LP, RP ) ) {					// match entire catch argument
		ahead = ahead->prev_parse_token();				// backup so closing RP is not matched
		token_t * p;
		// Looking for T1 in: catch( d.T1::T2 e ) by scanning backwards from the closing ')' of the catch clause.
		for ( p = ahead; ; p = p->prev_parse_token() ) {
		  if ( p == prefix ) goto fini;					// at start so not a bound exception
		  if ( p->prev_parse_token()->value == '.' && ( p->value == TYPE || p->value == NAMESPACE || p->value == COLON_COLON ) ) break;
			// SKULLDUGGERY: When parsing statements, identifiers are just scanned. Scanning looks up the tokens without
			// handling type qualification (e.g., T1::T2). Therefore, tokens in the statement stream may have incorrect
			// symbol pointers. To get correct lookup for the bound exception type, the "type" tokens are reset to
			// "identifier" with null symbols, and looked up again correctly in exception_declaration.
		  if ( p->value == TYPE ) { p->value = IDENTIFIER; p->symbol = nullptr; }
		} // for
		bound.idright = ahead;							// end of exception parameter (')')
		ahead = prefix;									// backup to the start of the exception declaration, excluding '('
		attribute_t attribute;
		while ( type_qualifier( attribute ) );			// scan off initial type qualifiers
		bound.oleft = ahead;							// start of bound object
		bound.oright = p->prev_parse_token();			// end of bound object, including dot
		bound.exbegin = p;								// start of exception type
		ahead = bound.exbegin;							// backup to the start of the exception type
		token_t * typ = type();							// properly parse the exception type (could contain '::')
	  if ( typ == nullptr ) goto fini;					// bad type ?
		bound.extype = typ->symbol;						// set the type of the exception parameter
		if ( match( '*' ) ) bound.pointer = true;		// scan off pointer/reference
		else match( '&' );
		bound.idleft = ahead;							// start of exception parameter
		if ( bound.idleft == bound.idright && kind == CATCH ) {	// no exception parameter in catch clause ?
			// add a dummy exception parameter name to access the thrown object
			bound.idleft = gen_code( ahead, "_U_boundParm", IDENTIFIER );
		} // if
		ahead = bound.idright->next_parse_token();		// reset the parse location to after ')'
		return true;
	} // if
  fini:
	ahead = back; return false;
} // bound_exception_declaration


static bool using_definition();
static bool using_directive();
static bool using_alias( attribute_t & attribute );

static bool object_declaration() {
	token_t * back = ahead;
	attribute_t attribute;

	while ( type_qualifier( attribute ) );			// scan off initial type qualifiers

	if ( using_definition() ) return true;
	if ( using_directive() ) return true;
	if ( using_alias( attribute ) ) return true;
	if ( static_assert_declaration() ) return true;		// C++11
	if ( constructor_declaration( attribute ) ) return true;
	if ( destructor_declaration( attribute ) ) return true;

	attribute.startCR = ahead;
	attribute.fileCR = file_token;
	attribute.lineCR = line;

	bool explict = specifier_list( attribute );
	if ( function_declaration( explict, attribute ) ) return true;

	if ( ahead->value == IDENTIFIER && ahead->symbol && attribute.plate ) { // template variable ?
		// SKULLDUGGERY: function_declaration (above) created a symbol table for this identifier, so use it.
		// Template variable must be put in the symbol table to handle parsing of the template closing delimiter.
		ahead->symbol->data->key = TEMPLATEVAR;
		focus->lexical->insert_table( ahead->symbol );
	} // if

	if ( attribute.typedef_base == nullptr ) {			// => not aggregate specifier, e.g., struct, class, etc.
		if ( attribute.dclmutex.qual.mutex ) gen_error( ahead, "_Mutex must qualify a class or member routine." );
		if ( attribute.dclmutex.qual.nomutex ) gen_error( ahead, "_Nomutex must qualify a class or member routine." );
	} // if

	declarator_list( attribute );
	if ( match( ';' ) ) {
		if ( attribute.plate != nullptr ) {
			delete pop_table();
		} // if
		return true;
	} // if

	ahead = back; return false;
} // object_declaration


static bool asm_declaration() {
	token_t * back = ahead;

	match( EXTENSION );									// optional
	if ( match( ASM ) ) {
		while ( match( VOLATILE ) | match( INLINE ) | match( GOTO ) ); // optional
		if ( match( LP ) && match_closing( LP, RP ) && match( ';' ) ) return true;
	} // if

	ahead = back; return false;
} // asm_declaration


static bool declaration();


// namespace-definition:
//	named-namespace-definition unnamed-namespace-definition

static bool namespace_definition() {
	token_t * back = ahead;
	token_t * token;
	symbol_t * symbol;

	bool inlne = match( INLINE );						// optional

	if ( match( NAMESPACE ) ) {
		token = identifier();
		if ( token == nullptr ) token = type();			// optional name
		else {
			// nested namespace definition: namespace A::B::C { ... } is equivalent to namespace A { namespace B { namespace C { ... } } }.
			// namespace A::B::inline C { ... } is equivalent to namespace A::B { inline namespace C { ... } }.
			// namespace A::inline B::C {} is equivalent to namespace A { inline namespace B { namespace C {} } }.
			// inline may appear in front of every namespace name except the first.
			// Does not handle inline at intermediate levels.
			while ( match( COLON_COLON ) ) {
				inlne = match( INLINE );				// optional
				token = identifier();
			} // while
		} // if

		attribute_clause_list();						// optional

		// check for a {, don't match because a symbol table has to be built before scanning the next token, which may
		// be a type
		if ( check( LC ) ) {
			if ( token != nullptr ) {					// named ?
				symbol = token->symbol;
				if ( symbol->data->key != NAMESPACE ) {
					make_type( token, symbol );
					symbol->data->key = NAMESPACE;
					if ( symbol->data->table == nullptr ) {
						symbol->data->table = new table_t( symbol ); // create a new symbol table
						symbol->data->attribute.dclqual.qual.INLINE = inlne;
					} // if
					symbol->data->table->lexical = focus; // connect lexical chain
				} // if
				symbol->data->table->push_table();
			} // if
			match( LC );								// now scan passed the '{' for the next token
			while ( declaration() );
			if ( token != nullptr ) pop_table();
			if ( match( RC ) ) {
				if ( token != nullptr && symbol->data->attribute.dclqual.qual.INLINE ) {
					local_t * use = new local_t;
					use->useing = true;
					use->tblsym = true;
					use->kind.tbl = symbol->data->table;
					use->link = focus->local;
					//cerr << "adding using table:" << use << " (" << ns->hash->text << ") to:" << focus
					//	 << " (" << (focus->symbol != nullptr ? focus->symbol->hash->text : (focus == root) ? "root" : "template/compound") << ")" << endl;
					focus->useing = true;				// using entries in this list
					focus->local = use;
				} // if
				return true;
			} // if
		} // if
	} // if

	ahead = back; return false;
} // namespace_definition


// namespace-alias-definition:
//	"namespace" identifier "=" qualified-namespace-specifier ";"

static bool namespace_alias_definition() {
	token_t * back = ahead;
	token_t * lhs, * rhs;
	symbol_t * symbol;

	if ( match( NAMESPACE ) && ( lhs = identifier() ) != nullptr && match( '=' ) && ( rhs = type() ) != nullptr && match( ';' ) ) {
		symbol = lhs->symbol;
		make_type( lhs, symbol );
		symbol->data->key = NAMESPACE;
		if ( symbol->data->table == nullptr ) {
			delete symbol->data;
			symbol->data = rhs->symbol->data;			// set to exisitng symbol table
		} // if
		return true;
	} // if

	ahead = back; return false;
} // namespace_alias_definition


// using-declaration:
//	"using" typename_opt "::"_opt nested-name-specifier unqualified-id ";"
//	"using" "::" unqualified-id ";"

static bool using_definition() {
	token_t * back = ahead;
	token_t * token;

	if ( match( USING ) ) {
		bool typname = match( TYPENAME );				// optional
		if ( ( token = identifier() ) != nullptr || ( token = operater() ) != nullptr || ( token = type() ) != nullptr ) {
			match( DOT_DOT_DOT );						// optional
			attribute_clause_list();					// optional
			if ( check( ';' ) ) {						// don't scan ahead yet
				uassert( token->symbol != nullptr );
				symbol_t * symbol = token->symbol;
				if ( typname ) {
					make_type( token, token->symbol );
				} else if ( symbol != nullptr ) {
					local_t * use = new local_t;
					use->useing = true;
					use->tblsym = false;
					use->kind.sym = symbol;
					use->link = focus->local;
					//cerr << "adding using symbol:" << use << " (" << symbol->hash->text << ") to:" << focus << " ("
					//	 << (focus->symbol != nullptr ? focus->symbol->hash->text : (focus == root) ? "root" : "template/compound") << ")" << endl;
					focus->useing = true;				// using entries in this list
					focus->local = use;
				} // if
				match( ';' );
				return true;
			} // if
		} // if
	} // if

	ahead = back; return false;
} // using_definition


// using-directive:
//	"using" "namespace" "::"_opt nested-name-specifier_opt namespace-name ";"

static bool using_directive() {
	token_t * back = ahead;
	token_t * token;

	if ( match( USING ) && match( NAMESPACE ) && ( ( token = identifier() ) != nullptr || ( token = operater() ) != nullptr || ( token = type() ) != nullptr ) ) {
		attribute_clause_list();						// optional
		if ( check( ';' ) ) {							// don't scan ahead yet
			symbol_t * symbol = token->symbol;
			if ( symbol != nullptr && symbol->data->table != nullptr ) {
				local_t * use = new local_t;
				use->useing = true;
				use->tblsym = true;
				use->kind.tbl = symbol->data->table;
				use->link = focus->local;
				//cerr << "adding using table:" << use << " (" << symbol->hash->text << ") to:" << focus
				//	 << " (" << (focus->symbol != nullptr ? focus->symbol->hash->text : (focus == root) ? "root" : "template/compound") << ")" << endl;
				focus->useing = true;					// using entries in this list
				focus->local = use;
			} // if
			match( ';' );
			return true;
		} // if
	} // if

	ahead = back; return false;
} // using_directive


// using-alias:
//	"using" identifier attr(optional) "=" typename_opt type-id ";"
//	"template" "<" template-parameter-list ">" "using" identifier attr(optional) "=" typename_opt type-id ";"


static bool using_alias( attribute_t & attribute ) {
	token_t * back = ahead;
	token_t * token;
	symbol_t * symbol;

	if ( ( match( USING ) || ( template_qualifier( attribute ) && match( USING ) ) ) &&
		 ( ( token = identifier() ) != nullptr || ( token = type() ) != nullptr ) ) {
		uassert( token != nullptr );
		symbol = token->symbol;
		// place alias in current lexical context if there is a template qualifier
		table_t * lexical = focus == attribute.plate ? focus->lexical : focus;
		make_type( token, symbol, lexical );
		attribute_clause_list();						// optional
		if ( match( '=' ) ) {
			bool typname = match( TYPENAME );			// optional
			specifier_list( attribute );
			uassert( token->symbol != nullptr );
			token->symbol->typname = typname;
			if ( formal_parameter( false, token, attribute ) ) return true; // function prototype ?

			if ( attribute.typedef_base == nullptr ) {	// => not aggregate specifier, e.g., struct, class, etc.
				if ( attribute.dclmutex.qual.mutex ) gen_error( ahead, "_Mutex must qualify a class or member routine." );
				if ( attribute.dclmutex.qual.nomutex ) gen_error( ahead, "_Nomutex must qualify a class or member routine." );
			} // if

			declarator( attribute );					// abstract
			match( '&' ) || match( AND_AND );			// optional

			if ( attribute.typedef_base != nullptr ) {	// null => base type without substructure (e.g., "int")
				symbol->data = attribute.typedef_base->data;
				symbol->copied = true;
			} // if
			if ( match( ';' ) ) {
				if ( attribute.plate != nullptr ) {
					delete pop_table();
				} // if
				return true;
			} // if
		} // if
	} // if

	ahead = back; return false;
} // using_alias


// linkage-specification:
//	"extern" string-literal "{" declaration-seq_opt "}"
//	"extern" string-literal declaration

static bool linkage_specification() {
	token_t * back = ahead;

	if ( match( EXTERN ) && string_literal() ) {		// should be "C" or "C++"
		if ( match( LC ) ) {
			while ( declaration() );
			if ( match( RC ) ) return true;
		} else {
			if ( declaration() ) return true;
		} // if
	} // if

	ahead = back; return false;
} // linkage_specification


static bool declaration() {
  if ( eof() ) return false;

  if ( asm_declaration() ) return true;
  if ( linkage_specification() ) return true;
  if ( namespace_alias_definition() ) return true;		// check before namespace_definition as ns names must be defined
  if ( namespace_definition() ) return true;			// define ns names, possibly nested
  if ( object_declaration() ) return true;

	return false;
} // declaration


// translation-unit:
//	declaration-seq_opt
//
// declaration-seq:
//	declaration
//	declaration-seq declaration

void translation_unit() {
	static bool skipping = false;

	ahead = token_list->get_head();

	scan();

	while ( ! eof() ) {
		if ( ! declaration() ) {
			// Did not recognize something.  It is probably an error or some construct added to the language that was
			// not anticipated.  In any case, simply scan for the next synchronizing token and continue parsing from
			// there.

			if ( ! skipping ) {							// only print once
				gen_warning( ahead, "unrecognizable text, possible uC++ syntax problem, skipping text to allow the C++ compiler to print an appropriate error." );
				skipping = true;
			} // if

			// printf( "\n\"" );
			while ( ! ( eof() || match( ';' ) || match( LC ) || match( RC ) ) ) {
				// printf( "%s ", ahead->hash->text );
				scan();
			} // while
			// printf( "\"\n" );
		} // if
	} // while
} // translation_unit


// Local Variables: //
// compile-command: "make install" //
// End: //
