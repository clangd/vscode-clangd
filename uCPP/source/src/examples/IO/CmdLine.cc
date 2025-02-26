//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2024
// 
// CmdLine.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue Jun 11 11:15:00 2024
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Jun 13 14:39:48 2024
// Update Count     : 6
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

// Command syntax:
//   $ ./a.out [ size (>= 0) | 'd' (default 20) [ code (>= 0) | 'd' (default 5) [ input-file [ output-file ] ] ] ]
// all parameters are optional. Defaults: size=>20, code=>5, input=>cin, output=>cout
//
// Examples:
//   $ ./a.out 34
//   $ ./a.out 34 0
//   $ ./a.out 34 0 in
//   $ ./a.out 34 0 in out

#include <iostream>
#include <fstream>
using namespace std;									// direct access to std

int main( int argc, char * argv[] ) {
	enum { DefaultSize = 20, DefaultCode = 5 };			// global defaults
	// MUST BE INT (NOT UNSIGNED) TO TEST FOR NEGATIVE VALUES
	intmax_t size = DefaultSize, code = DefaultCode;	// default value
	istream * infile = &cin;							// default value
	ostream * outfile = &cout;							// default value

	struct cmd_error {};
	try {
		switch ( argc ) {
		  case 5: case 4:
			// open input file first as output creates file
			infile = new ifstream();
			// must use open for bound exception
			dynamic_cast<ifstream *>(infile)->open( argv[3] );
			if ( argc == 5 ) {
				outfile = new ofstream();
				// must use open for bound exception
				dynamic_cast<ofstream *>(outfile)->open( argv[4] );
			} // if
			// FALL THROUGH
		  case 3:
			if ( strcmp( argv[2], "d" ) != 0 ) {		// default ?
				code = convert( argv[2] );
				if ( code < 0 ) throw cmd_error{};		// invalid ?
			} // if
			// FALL THROUGH
		  case 2:
			if ( strcmp( argv[1], "d" ) != 0 ) {		// default ?
				size = convert( argv[1] );
				if ( size < 0 ) throw cmd_error{};		// invalid ?
			} // if
			// FALL THROUGH
		  case 1:										// defaults
			break;
		  default:										// wrong number of options
			throw cmd_error{};
		} // switch
	} catch( *infile.uFile::Failure & ) {				// input open failed
		cerr << "Error! Could not open input file \"" << argv[3] << "\"" << endl;
		exit( EXIT_FAILURE );							// TERMINATE
	} catch( *outfile.uFile::Failure & ) {				// output open failed
		cerr << "Error! Could not open output file \"" << argv[4] << "\"" << endl;
		exit( EXIT_FAILURE );							// TERMINATE
	} catch( ... ) {									// command-line argument failed (invalid_argument)
		cerr << "Usage: " << argv[0]
			 << " [ size (>= 0) | 'd' (default " << DefaultSize
			 << ") [ code (>= 0) | 'd' (default " << DefaultCode
			 << ") [ input-file [ output-file ] ] ] ]" << endl;
		exit( EXIT_FAILURE );							// TERMINATE
	} // try
	//cout << "size " << size << " code " << code << endl;

	*infile >> noskipws;								// turn off white space skipping during input

	char ch;
	for ( ;; ) {										// copy input file to output file
		*infile >> ch;									// read a character
	  if ( infile->fail() ) break;						// failed/eof ?
		*outfile << ch;									// write a character
	} // for

	if ( infile != &cin ) delete infile;				// close file, do not delete cin!
	if ( outfile != &cout ) delete outfile;				// close file, do not delete cout!
} // main
