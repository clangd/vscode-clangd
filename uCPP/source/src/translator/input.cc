//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// input.c --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:05:28 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Jan  6 17:24:31 2025
// Update Count     : 169
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

#include <cstdio>										// sprintf

#include "main.h"
#include "hash.h"
#include "token.h"
#include "key.h"
#include "input.h"

#include <iostream>
#include <cstdlib>										// exit

using std::cerr;
using std::endl;

#define BUFLEN (64 * 1024)

static char buffer[BUFLEN];
static char * bptr = buffer;
static char * eptr = buffer + BUFLEN;
static char * cptr;

static bool strliteral = false;

#define DELIMITER_MAX_LENGTH (16)						// C++11
static char delimiter_name[DELIMITER_MAX_LENGTH];
static unsigned int delimiter_lnth, delimiter_lnth_ctr;

static int get() {
	char c;
	*yyin >> c;
	if ( cptr == eptr ) {
		cerr << "get() : internal buffer space exhausted." << endl;
	} // if
	*cptr++ = c;
	return yyin->eof() ? EOF : c;
} // get

static void unget( int c ) {
	yyin->putback( c );
	cptr--;
} // unget

static void unwrap() {
	cptr = bptr;
} // unwrap

static inline void wrap() {
	*cptr++ = '\0';
} // wrap

typedef enum state_t {
	start,
	white,
	directive,
	identifier,
	character,
	character_escape,
	string,
	string_escape,
	wide_string,
	unicode_string,
	raw_string,
	start_delimiter,
	raw_character,
	end_delimiter,
	greater_than,
	right_shift,
	less_than,
	left_shift,
	plus,
	minus,
	arrow,
	multiply,
	divide,
	modulus,
	logical_not,
	bitwise_xor,
	bitwise_and,
	bitwise_or,
	assign,
	dot,
	dot_dot,
	colon,
	octal,
	hexadecimal,
	decimal,
	long_unsigned,
	fraction,
	hex_fraction,
	possible_exponent,
	possible_signed_exponent,
	exponent,
	float_long,
	float_size,											// g++-14, floating-point constant suffix
	divide_or_comment,
	new_comment,
	old_comment,
	possible_end_comment,
} state_t;

token_t * getinput() {
	state_t state = start;

	unwrap();

	for ( ;; ) {
		int c = get();
		switch ( state ) {
		  case start:
			switch ( c ) {
			  case EOF:
				return new token_t( EOF, nullptr );
			  case '\n':
			  case '\r':
				wrap();
				return new token_t( c, hash_table->lookup( buffer ) );
			  case ' ': case '\t': case '':
				state = white;
				break;
			  case '#':
				state = directive;
				break;
			  case '\'':
				state = character;
				break;
			  case '\"':
				state = string;
				break;
			  case 'L':
				state = wide_string;
				break;
			  case 'u': case 'U':
				state = unicode_string;
				break;
			  case 'R':
				state = raw_string;
				break;
			  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
			  case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
			  case 'v': case 'w': case 'x': case 'y': case 'z': case 'A': case 'B': case 'C': case 'D': case 'E':
			  case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'M': case 'N': case 'O': case 'P':
			  case 'Q': case 'S': case 'T': case 'V': case 'W': case 'X': case 'Y': case 'Z':
			  case '_':
				state = identifier;
				break;
			  case '>':
				state = greater_than;
				break;
			  case '<':
				state = less_than;
				break;
			  case '!':
				state = logical_not;
				break;
			  case '^':
				state = bitwise_xor;
				break;
			  case '&':
				state = bitwise_and;
				break;
			  case '|':
				state = bitwise_or;
				break;
			  case '*':
				state = multiply;
				break;
			  case '/':
				state = divide_or_comment;
				break;
			  case '%':
				state = modulus;
				break;
			  case '-':
				state = minus;
				break;
			  case '+':
				state = plus;
				break;
			  case '=':
				state = assign;
				break;
			  case ':':
				state = colon;
				break;
			  case '.':
				state = dot;
				break;
			  case '0':
				state = octal;
				break;
			  case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
				state = decimal;
				break;
			  default:
				wrap();
				return new token_t( c, hash_table->lookup( buffer ) );
			} // switch
			break;
		  case white:
			switch ( c ) {
			  case ' ': case '\t': case '':
				break;
			  default:
				unget( c );
				unwrap();
				state = start;
				break;
			} // switch
			break;
		  case directive:
			switch ( c ) {
			  case '\n':
			  case '\r':
				wrap();
				return new token_t( '#', hash_table->lookup( buffer ) );
				break;
			  default:
				break;
			} // switch
			break;
		  case divide_or_comment:
			switch ( c ) {
			  case '/':
				state = new_comment;
				break;
			  case '*':
				state = old_comment;
				break;
			  default:
				unget( c );
				state = divide;
				break;
			} // switch
			break;
		  case new_comment:
			switch ( c ) {
			  case '\n':
			  case '\r':
				unget( c );
				unwrap();
				state = start;
			  default:
				break;
			} // switch
			break;
		  case old_comment:
			switch ( c ) {
			  case '*':
				state = possible_end_comment;
				break;
			  default:
				break;
			} // switch
			break;
		  case possible_end_comment:
			switch ( c ) {
			  case '/':
				unwrap();
				state = start;
				break;
			  default:
				unget( c );
				state = old_comment;
				break;
			} // switch
			break;
		  case identifier:
		  identifier:
			switch ( c ) {
			  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
			  case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
			  case 'v': case 'w': case 'x': case 'y': case 'z': case 'A': case 'B': case 'C': case 'D': case 'E':
			  case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
			  case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
			  case '_':
			  case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
				break;
			  default:
				unget( c );
				wrap();
				if ( strliteral ) {
					strliteral = false;
					return new token_t( STRING_IDENTIFIER, hash_table->lookup( buffer ) );
				} else {
					return new token_t( IDENTIFIER, hash_table->lookup( buffer ) );
				} // if
			} // switch
			break;
		  case wide_string:
			switch ( c ) {
			  case '\'':
				state = character;
				break;
			  case '\"':
				state = string;
				break;
			  case 'R':
				state = raw_string;
				break;
			  default:
				goto identifier;
			} // switch
			break;
		  case unicode_string:
			switch ( c ) {
			  case '8':
				state = unicode_string;					// under constrain, allow multiple 8s, which are subsequently invalid
				break;
			  case 'R':
				state = raw_string;
				break;
			  case '\'':
				state = character;
				break;
			  case '\"':
				state = string;
				break;
			  default:
				goto identifier;
			} // switch
			break;
		  case raw_string:
			switch ( c ) {
			  case '\"':
				delimiter_lnth = delimiter_lnth_ctr = 0; // reset counters
				state = start_delimiter;
				break;
			  default:
				goto identifier;
			} // switch
			break;
		  case start_delimiter:
			if ( c  == '(' ) {
				state = raw_character;
			} else {
				// Do not care about characters composing delimiter.
				if ( delimiter_lnth < DELIMITER_MAX_LENGTH ) { // exceed max delimiter length
					delimiter_name[delimiter_lnth] = c;
					delimiter_lnth += 1;
				} else {								// syntax error, panic and look for end of string
					state = raw_character;
				} // if
			} // if
			break;
		  case raw_character:
			if ( c == ')' ) {
				state = end_delimiter;
			} // if
			break;
		  case end_delimiter:
			if ( c == '\"' ) {
				wrap();
				return new token_t( STRING, hash_table->lookup( buffer ) );
			} else if ( delimiter_lnth_ctr < delimiter_lnth && c == delimiter_name[delimiter_lnth_ctr] ) {
				delimiter_lnth_ctr += 1;
			} else {
				delimiter_lnth_ctr = 0;					// reset counter
				state = raw_character;					// not delimiter, continue scan
			} // if
			break;
		  case character:
			switch ( c ) {
			  case '\'':
				wrap();
				return new token_t( CHARACTER, hash_table->lookup( buffer ) );
			  case '\\':
				state = character_escape;
				break;
			  default:
				break;
			} // switch
			break;
		  case character_escape:
			state = character;
			break;
		  case string:
			switch ( c ) {
			  case '\"':
				{
					int c = get();						// temporary lookahead
					unget( c );
					if ( isalpha( c ) || c == '_' ) {	// identifier ?
						strliteral = true;				// global state
						state = identifier;				// now lex the identifier juxtaposed to string, "abc"abc
						break;
					} // if
				}
				wrap();
				return new token_t( STRING, hash_table->lookup( buffer ) );
				break;
			  case '\\':
				state = string_escape;
				break;
			  default:
				break;
			} // switch
			break;
		  case string_escape:
			state = string;
			break;
		  case greater_than:
			switch ( c ) {
			  case '>':
				state = right_shift;
				break;
			  case '=':
				wrap();
				return new token_t( GE, hash_table->lookup( buffer ) );
			  case '?':
				wrap();
				return new token_t( GMAX, hash_table->lookup( buffer ) ); // g++ >? maximum
			  default:
				unget( c );
				wrap();
				return new token_t( '>', hash_table->lookup( buffer ) );
			} // switch
			break;
		  case right_shift:
			switch ( c ) {
			  case '=':
				wrap();
				return new token_t( RSH_ASSIGN, hash_table->lookup( buffer ) );
			  default:
				unget( c );
				wrap();
				return new token_t( RSH, hash_table->lookup( buffer ) );
			} // switch
			break;
		  case less_than:
			switch ( c ) {
			  case '<':
				state = left_shift;
				break;
			  case '=':
				wrap();
				return new token_t( LE, hash_table->lookup( buffer ) );
			  case '?':
				wrap();
				return new token_t( GMIN, hash_table->lookup( buffer ) ); // g++ <? minimum
			  default:
				unget( c );
				wrap();
				return new token_t( '<', hash_table->lookup( buffer ) );
			} // switch
			break;
		  case left_shift:
			switch ( c ) {
			  case '=':
				wrap();
				return new token_t( LSH_ASSIGN, hash_table->lookup( buffer ) );
			  default:
				unget( c );
				wrap();
				return new token_t( LSH, hash_table->lookup( buffer ) );
			} // switch
			break;
		  case plus:
			switch ( c ) {
			  case '+':
				wrap();
				return new token_t( PLUS_PLUS, hash_table->lookup( buffer ) );
			  case '=':
				wrap();
				return new token_t( PLUS_ASSIGN, hash_table->lookup( buffer ) );
			  default:
				unget( c );
				wrap();
				return new token_t( '+', hash_table->lookup( buffer ) );
			} // switch
			break;
		  case minus:
			switch ( c ) {
			  case '-':
				wrap();
				return new token_t( MINUS_MINUS, hash_table->lookup( buffer ) );
			  case '=':
				wrap();
				return new token_t( MINUS_ASSIGN, hash_table->lookup( buffer ) );
			  case '>':
				state = arrow;
				break;
			  default:
				unget( c );
				wrap();
				return new token_t( '-', hash_table->lookup( buffer ) );
			} // switch
			break;
		  case arrow:
			switch ( c ) {
			  case '*':
				wrap();
				return new token_t( ARROW_STAR, hash_table->lookup( buffer ) );
			  default:
				unget( c );
				wrap();
				return new token_t( ARROW, hash_table->lookup( buffer ) );
			} // switch
			break;
		  case multiply:
			switch ( c ) {
			  case '=':
				wrap();
				return new token_t( MULTIPLY_ASSIGN, hash_table->lookup( buffer ) );
			  default:
				unget( c );
				wrap();
				return new token_t( '*', hash_table->lookup( buffer ) );
			} // switch
			break;
		  case divide:
			switch ( c ) {
			  case '=':
				wrap();
				return new token_t( DIVIDE_ASSIGN, hash_table->lookup( buffer ) );
			  default:
				unget( c );
				wrap();
				return new token_t( '/', hash_table->lookup( buffer ) );
			} // switch
			break;
		  case modulus:
			switch ( c ) {
			  case '=':
				wrap();
				return new token_t( MODULUS_ASSIGN, hash_table->lookup( buffer ) );
			  default:
				unget( c );
				wrap();
				return new token_t( '%', hash_table->lookup( buffer ) );
			} // switch
			break;
		  case logical_not:
			switch ( c ) {
			  case '=':
				wrap();
				return new token_t( NE, hash_table->lookup( buffer ) );
			  default:
				unget( c );
				wrap();
				return new token_t( '!', hash_table->lookup( buffer ) );
			} // switch
			break;
		  case bitwise_xor:
			switch ( c ) {
			  case '=':
				wrap();
				return new token_t( XOR_ASSIGN, hash_table->lookup( buffer ) );
			  default:
				unget( c );
				wrap();
				return new token_t( '^', hash_table->lookup( buffer ) );
			} // switch
			break;
		  case bitwise_and:
			switch ( c ) {
			  case '=':
				wrap();
				return new token_t( AND_ASSIGN, hash_table->lookup( buffer ) );
			  case '&':
				wrap();
				return new token_t( AND_AND, hash_table->lookup( buffer ) );
			  default:
				unget( c );
				wrap();
				return new token_t( '&', hash_table->lookup( buffer ) );
			} // switch
			break;
		  case bitwise_or:
			switch ( c ) {
			  case '=':
				wrap();
				return new token_t( OR_ASSIGN, hash_table->lookup( buffer ) );
			  case '|':
				wrap();
				return new token_t( OR_OR, hash_table->lookup( buffer ) );
			  default:
				unget( c );
				wrap();
				return new token_t( '|', hash_table->lookup( buffer ) );
			} // switch
			break;
		  case dot:
			switch ( c ) {
			  case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			  case '\'':
				state = fraction;
				break;
			  case '.':
				state = dot_dot;
				break;
			  case '*':
				wrap();
				return new token_t( DOT_STAR, hash_table->lookup( buffer ) );
			  default:
				unget( c );
				wrap();
				return new token_t( '.', hash_table->lookup( buffer ) );
			} // switch
			break;
		  case dot_dot:
			switch ( c ) {
			  case '.':
				wrap();
				return new token_t( DOT_DOT_DOT, hash_table->lookup( buffer ) );
			  default:
				unget( c );
				wrap();
				return new token_t( DOT_DOT, hash_table->lookup( buffer ) );
			} // switch
			break;
		  case colon:
			switch ( c ) {
			  case ':':
				wrap();
				return new token_t( COLON_COLON, hash_table->lookup( buffer ) );
			  default:
				unget( c );
				wrap();
				return new token_t( ':', hash_table->lookup( buffer ) );
			} // switch
			break;
		  case octal:
			switch ( c ) {
			  case 'x': case 'X':
				state = hexadecimal;
				break;
			  case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
				// handles floating-point constants 0123456789., which can start with 0!
			  case '8': case '9':
			  case '\'':
				break;
			  case 'l': case 'L': case 'u': case 'U':
				state = long_unsigned;
				break;
			  case '.':
				state = fraction;
				break;
			  case 'e': case 'E':
				state = possible_exponent;
				break;
			  default:
				unget( c );
				wrap();
				return new token_t( NUMBER, hash_table->lookup( buffer ) );
			} // switch
			break;
		  case hexadecimal:
			switch ( c ) {
			  case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
			  case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
			  case '\'':
				break;
			  case 'l': case 'L': case 'u': case 'U':
				state = long_unsigned;
				break;
			  case '.':
				state = hex_fraction;
				break;
			  case 'p': case 'P':
				state = possible_exponent;
				break;
			  default:
				unget( c );
				wrap();
				return new token_t( NUMBER, hash_table->lookup( buffer ) );
			} // switch
			break;
		  case decimal:
			switch ( c ) {
			  case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			  case '\'':
				break;
			  case 'l': case 'L': case 'u': case 'U':
				state = long_unsigned;
				break;
			  case '.':
				state = fraction;
				break;
			  case 'e': case 'E':
				state = possible_exponent;
				break;
			  default:
				unget( c );
				wrap();
				return new token_t( NUMBER, hash_table->lookup( buffer ) );
			} // switch
			break;
		  case long_unsigned:
			switch ( c ) {
			  case 'l': case 'L': case 'u': case 'U':
				break;
			  default:
				unget( c );
				wrap();
				return new token_t( NUMBER, hash_table->lookup( buffer ) );
			} // switch
			break;
		  case fraction:
			switch ( c ) {
			  case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			  case '\'':
				break;
			  case 'e': case 'E':
				state = possible_exponent;
				break;
			  case 'f': case 'F': case 'l': case 'L': //case 'b': case 'B':
				state = float_long;
				break;
			  default:
				unget( c );
				wrap();
				return new token_t( NUMBER, hash_table->lookup( buffer ) );
			} // switch
			break;
		  case hex_fraction:
			switch ( c ) {
			  case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
			  case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
			  case '\'':
				break;
			  case 'p': case 'P':
				state = possible_exponent;
				break;
			  default:
				unget( c );
				wrap();
				return new token_t( NUMBER, hash_table->lookup( buffer ) );
			} // switch
			break;
		  case possible_exponent:
			switch ( c ) {
			  case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
				state = exponent;
				break;
			  case '+': case '-':
				state = possible_signed_exponent;
				break;
			  default:
				unget( c );
				wrap();
				return new token_t( NUMBER, hash_table->lookup( buffer ) );
			} // switch
			break;
		  case possible_signed_exponent:
			switch ( c ) {
			  case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			  case '\'':
				state = exponent;
				break;
			  default:
				unget( c );
				wrap();
				return new token_t( NUMBER, hash_table->lookup( buffer ) );
			} // switch
			break;
		  case exponent:
			switch ( c ) {
			  case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			  case '\'':
				break;
			  case 'f': case 'F': case 'l': case 'L': //case 'b': case 'B':
				state = float_long;
				break;
			  default:
				unget( c );
				wrap();
				return new token_t( NUMBER, hash_table->lookup( buffer ) );
			} // switch
			break;
		  case float_long:
			switch ( c ) {
			  case 'f': case 'F': case 'l': case 'L': //case 'b': case 'B':
				break;
			  default:
				unget( c );								// put character back as it could start float size
			} // switch
			state = float_size;
			break;
		  case float_size:
			switch ( c ) {
			  case '1': case '2': case '3': case '4': case '6': case '8':
				break;
			  default:
				unget( c );
				wrap();
				return new token_t( NUMBER, hash_table->lookup( buffer ) );
			} // switch
			break;
		  case assign:
			switch ( c ) {
			  case '=':
				wrap();
				return new token_t( EQ, hash_table->lookup( buffer ) );
			  default:
				unget( c );
				wrap();
				return new token_t( '=', hash_table->lookup( buffer ) );
			} // switch
			break;
		  default:
			cerr << "uC++ Translator error: getinput(), internal error." << endl;
			exit( EXIT_FAILURE );
		} // switch
	} // for
} // getinput

void read_all_input() {
	for ( ;; ) {
		token_t * token = getinput();
		token_list->add_to_tail( *token );
		if ( token->value == EOF ) break;
	} // for
} // read_all_input

// Local Variables: //
// compile-command: "make install" //
// End: //
