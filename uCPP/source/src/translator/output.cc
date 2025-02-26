//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// output.c --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:09:30 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Oct  7 08:13:59 2023
// Update Count     : 255
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

#include <cstdio>										// EOF

#include "uassert.h"
#include "main.h"
#include "key.h"
#include "hash.h"
#include "token.h"
#include "output.h"

#include <cstring>										// strcpy, strlen

#define xstr(s) str(s)
#define str(s) #s

token_t * file_token = nullptr;
char file[FILENAME_MAX];
unsigned int line = 1, flag;
const char * flags[] = { "", " 3", " 4", " 3 4" };

void context() {
	token_t * p;
	int i;

	cerr << "=====>\"";
	// backup up to 10 tokens
	for ( i = 0, p = ahead; i < 10 && p->aft != nullptr; i += 1, p = p->aft );
	// print up to 10 tokens before problem area
	for ( ; p != ahead; p = p->fore ) {
		if ( p->hash == nullptr ) continue;
		cerr << p->hash->text << " ";
	} // for
	cerr << " @ " << (ahead->hash ? ahead->hash->text : "EOF") << " @ ";
	// print up to 10 tokens after problem area
	for ( i = 0, p = ahead->fore; i < 10 && p != nullptr; i += 1, p = p->fore ) {
		if ( p->hash == nullptr ) continue;
		cerr << p->hash->text << " ";
	} // for
	cerr << "\"" << endl;
} // context

void sigSegvBusHandler( int ) {
	cerr << "uC++ Translator error: fatal problem during parsing." << endl <<
		"Probable cause is mismatched braces, missing terminating quote, or use of an undeclared type name." << endl <<
		"Possible area where problem occurred:" << endl;
	context();
	exit( EXIT_SUCCESS );
} // sigSegvBusHandler

void parse_directive( char * text, char file[], unsigned int & line, unsigned int & flag ) {
	char d;
	unsigned int flag1, flag2, flag3;
	int len = sscanf( text, " %[#] %d %[\"]%" xstr(FILENAME_MAX) "[^\"]%[\"] %d%d%d",
					  &d, &line, &d, &file[0], &d, &flag1, &flag2, &flag3 );
  if ( len == 1 ) { line += 1; return; }				// normal directive, increment line number
	uassert( ((void)"internal error, bad line number directive", len >= 5 ) );
	// https://gcc.gnu.org/onlinedocs/cpp/Preprocessor-Output.html
	// 3 indicates the following text comes from a system header file, so certain warnings should be suppressed.
	// 4 indicates that the following text should be treated as being wrapped in an implicit extern "C" block.
	switch ( len ) {
	  case 5:
		flag = 0;
		break;
	  case 6:
		if ( flag1 == 3 ) flag = 1;
		if ( flag1 == 4 ) flag = 2;
		break;
	  case 7:
		if ( flag1 == 3 && flag2 == 4 ) flag = 3;
		else if ( flag1 == 3 ) flag = 1;
		else if ( flag2 == 4 ) flag = 2;
		break;
	  case 8:
		if ( flag2 == 3 && flag3 == 4 ) flag = 3;
		else if ( flag2 == 3 ) flag = 1;
		else if ( flag3 == 4 ) flag = 2;
		break;
	} // switch	  
} // parse_directive

// The routine 'output' converts a token value into text.  A considerable amount of effort is taken to keep track of the
// current file name and line number so that when error and warning messages appear, the exact origin of those messages
// can be displayed.

void putoutput( token_t * token ) {
	uassert( token != nullptr );
	uassert( token->hash != nullptr );
	uassert( token->hash->text != nullptr );

	switch ( token->value ) {
	  case '\n':
	  case '\r':
		line += 1;
		*yyout << token->hash->text;
		break;
	  case '#':
		parse_directive( token->hash->text, file, line, flag );
		file_token = token;
		*yyout << token->hash->text;
		break;
	  case ERROR:
		cerr << file << ":" << line << ": uC++ Translator error: " << token->hash->text << endl;
		error = true;
		break;
	  case WARNING:
		cerr << file << ":" << line << ": uC++ Translator warning: " << token->hash->text << endl;
		break;
	  case AT:											// do not print these keywords
	  case CATCHRESUME:
	  case CONN_OR:
	  case CONN_AND:
	  case SELECT_LP:
	  case SELECT_RP:
	  case MUTEX:
	  case NOMUTEX:
	  case WHEN:
		break;
	  case CATCH:										// catch and _Catch => catch
		*yyout << " " << "catch";
		break;
	  case ACTOR:
	  case EXCEPTION:
	  case CORACTOR:
	  case COROUTINE:
	  case TASK:
	  case PTASK:
	  case RTASK:
	  case STASK:
	  case DISABLE:
	  case ENABLE:
	  case RESUME:
	  case RESUMETOP:
	  case UTHROW:
	  case ACCEPT:
	  case ACCEPTRETURN:
	  case ACCEPTWAIT:
	  case SELECT:
	  case TIMEOUT:
	  case WITH:
		{
			// if no code or error was generated, print an error now
			int value = token->next_parse_token()->value;
			if ( value != CODE && value != ERROR ) {
				cerr << file << ":" << line << ": uC++ Translator warning: no code generated for " << token->hash->text << "." << endl;
			} // if
			if ( value == ERROR ) {
				cerr << file << ":" << line << ": uC++ Translator error: parse error before " << token->hash->text << "." << endl;
				error = true;
			} // if
			break;
		}
	  default:
		*yyout << " " << token->hash->text;
		break;
	} // switch
} // putoutput

// The routine 'write_all_output' takes the stream of tokens and calls 'putoutput' to convert them all to a stream of
// text.  It then deletes each token in the list.

void write_all_output() {
	for ( ;; ) {
		token_t * token = token_list->remove_from_head();
	  if ( token->value == EOF ) break;
		putoutput( token );
	} // for
} // write_all_output

// Local Variables: //
// compile-command: "make install" //
// End: //
