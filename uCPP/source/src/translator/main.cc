//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// main.c --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:25:22 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Apr  3 15:33:39 2022
// Update Count     : 186
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

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <csignal>
#include <cstdlib>										// exit
#include <cstring>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::string;

#include "main.h"
#include "key.h"
#include "hash.h"
#include "symbol.h"
#include "token.h"
#include "table.h"
#include "input.h"
#include "output.h"
#include "parse.h"

//#define __U_DEBUG_H__
#include "debug.h"

istream *yyin = &cin;
ostream *yyout = &cout;

bool error = false;
bool profile = false;
bool stdcpp11 = false;

extern void sigSegvBusHandler( int sig );

void check( string arg ) {
	if ( arg == "__U_PROFILE__" ) {
		profile = true;
	} else if ( arg == "__U_STD_CPP11__" ) {
		stdcpp11 = true;
	} // if
} // check

int main( int argc, char *argv[] ) {
	char *infile = nullptr;
	char *outfile = nullptr;
	strcpy( file, "no file name" );

	// The translator can receive 2 kinds of arguments.
	//
	// The first kind begins with a '-' character and are generally -D<string> arguments.  Arguments like __GNUG__
	// affect the code that is produced by the translator.
	//
	// The second kind of argument are input and output file specifications.  These arguments do not begin with a '-'
	// character.  The first file specification is taken to be the input file specification while the second file
	// specification is taken to be the output file specification.  If no files are specified, stdin and stdout are
	// assumed.  If more files are specified, an error results.

	for ( int i = 1; i < argc; i += 1 ) {
		uDEBUGPRT( cerr << "argv[" << i << "]:\"" << argv[i] << "\"" << endl; );
		if ( argv[i][0] == '-' ) {
			string arg = string( argv[i] );
			if ( arg == "-D" ) {
				i += 1;									// advance to next argument
				uDEBUGPRT( cerr << "argv[" << i << "]:\"" << argv[i] << "\"" << endl; );
				check( string( argv[i] ) );
			} else if ( arg.substr(0,2) == "-D" ) {
				check( arg.substr(2) );
			} // if
		} else {
			if ( infile == nullptr ) {
				infile = argv[i];
				uDEBUGPRT( cerr << "infile:" << infile << endl; );
				yyin = new ifstream( infile );
				if ( yyin->fail() ) {
					cerr << "uC++ Translator error: could not open file " << infile << " for reading." << endl;
					exit( EXIT_FAILURE );
				} // if
			} else if ( outfile == nullptr ) {
				outfile = argv[i];
				uDEBUGPRT( cerr << "outfile:" << outfile << endl; );
				yyout = new ofstream( outfile );
				if ( yyout->fail() ) {
					cerr << "uC++ Translator error: could not open file " << outfile << " for writing." << endl;
					exit( EXIT_FAILURE );
				} // if
			} else {
				cerr << "Usage: " << argv[0] << " [options] [input-file [output-file]]" << endl;
				exit( EXIT_FAILURE );
			} // if
		} // if
	} // for

	uDEBUGPRT( cerr << " profile:" << profile << " std cpp11:" << stdcpp11 << endl; );

	*yyin >> std::resetiosflags( std::ios::skipws );	// turn off white space skipping during input

	signal( SIGSEGV, sigSegvBusHandler );
	signal( SIGBUS,  sigSegvBusHandler );

	// This is the heart of the translator.  Although inefficient, it is very simple.  First, all the input is read and
	// convert to a list of tokens.  Second, this list is parsed, extracting and inserting tokens as necessary. Third,
	// this list of tokens is converted into an output stream again.

	hash_table = new hash_table_t;

	focus = root = new table_t( nullptr );				// start at the root table
	top = new lexical_t( focus );

	// Insert the keywords into the root symbol table.

	int i;
	for ( i = 0; key[i].text != nullptr; i += 1 ) {
		hash_table->lookup( key[i].text, key[i].value );
	} // for
	if ( stdcpp11 ) {									// insert C++11 keywords
		for ( i += 1; key[i].text != nullptr; i += 1 ) {
			hash_table->lookup( key[i].text, key[i].value );
		} // for
	} // if

	read_all_input();
	translation_unit();									// parse the program
	write_all_output();

#if 0
	root->display_table( 0 );
#endif


// TEMPORARY: deleting this data structure does not work!!!!!!!!!
//    delete root;

	delete hash_table;

	// close any open files before quitting.

	if ( yyin != &cin ) delete yyin;
	if ( yyout != &cout ) delete yyout;

	// If an error has occurred during the translation phase, return a failure result to signify this fact. This causes
	// the host compiler to terminate the compilation at this point, just as if the regular cpp had failed.

	return error ? EXIT_FAILURE : EXIT_SUCCESS;
} // main

// Local Variables: //
// compile-command: "make install" //
// End: //
