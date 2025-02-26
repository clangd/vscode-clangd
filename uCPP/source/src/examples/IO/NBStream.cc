//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// NBStream.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Wed Apr 28 13:23:14 1993
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Apr 21 18:40:03 2022
// Update Count     : 32
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
using std::cin;
using std::cerr;

int d = 0;												// shared by reader and writer

_Task Reader {
	void main() {
		try {
			for ( ;; ) {
				cin >> d;								// read number from stdin
			  if ( cin.fail() ) break;
			} // for
		} catch( ... ) {
			abort( "reader failure" );
		} // try
	} // Reader::main
}; // Reader

_Task Writer {
	void main() {
		try {
			for ( unsigned int i = 0;; i += 1 ) {
			  if ( cin.fail() ) break;
				if ( i % 500 == 0 ) {					// don't print too much
					cerr << d;							// write number to stderr (no buffering)
				} // if
				yield( 1 );
			} // for
		} catch( ... ) {
			abort( "writer failure" );
		} // try
	} // Writer::main
}; // Writer

int main() {
	Reader r;
	Writer w;
} // main

// Local Variables: //
// compile-command: "u++ NBStream.cc" //
// End: //
