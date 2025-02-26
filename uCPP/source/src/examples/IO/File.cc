// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// File.cc -- Print multiple copies of the same file to standard output
// 
// Author           : Peter A. Buhr
// Created On       : Tue Jan  7 08:44:56 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Jan 20 08:03:21 2024
// Update Count     : 47
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

#include <uFile.h>
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

_Task Copier {
	uFile &input;

	void main() {
		uFile::FileAccess in( input, O_RDONLY );
		int count;
		char buf[1];

		for ( int i = 0;; i += 1 ) {					// copy in-file to out-file
			count = in.read( buf, sizeof( buf ) );
		  if ( count == 0 ) break;						// eof ?
			cout << buf[0];
			if ( i % 20 == 0 ) yield();
		} // for
	} // Copier::main
  public:
	Copier( uFile &in ) : input( in ) {
	} // Copier::Copier
}; // Copier

int main( int argc, char *argv[] ) {
	switch ( argc ) {
	  case 2:
		break;
	  default:
		cerr << "Usage: " << argv[0] << " input-file" << std::endl;
		exit( EXIT_FAILURE );
	} // switch

	uFile input( argv[1] );								// connect with UNIX files
	{
		Copier c1( input ), c2( input );
	}
} // main

// Local Variables: //
// compile-command: "u++ File.cc" //
// End: //
