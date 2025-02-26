// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2016
// 
// ActorFork.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Dec 19 08:22:05 2016
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Apr 25 17:26:17 2022
// Update Count     : 28
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
using namespace std;
#include <uActor.h>

#ifdef NOOUTPUT											// disable printing for experiments
#define PRT( stmt )
#else
#define PRT( stmt ) stmt
#endif // NOOUTPUT

#define KERNELTHREADS 2
unsigned int uDefaultActorThreads() { return KERNELTHREADS; }
unsigned int uDefaultActorProcessors() { return KERNELTHREADS - 1; } // plus given processor

int MaxLevel = 3;										// default value

_Actor Fork {
	unsigned int level;

	Allocation receive( Message & msg ) {
		Case( StartMsg, msg ) {
			PRT( osacquire( cout ) << this << " level " << level << endl; )
				if ( level < (unsigned int)MaxLevel ) {
					*(new Fork( level + 1 )) | uActor::startMsg; // left
					*(new Fork( level + 1 )) | uActor::startMsg; // right
				} // if
		} // Case
		return Delete;
	} // Fork::receive
  public:
	Fork( unsigned int level ) : level( level ) {}
}; // Fork

int main( int argc, char *argv[] ) {
	try {
		switch ( argc ) {
		  case 2:
			MaxLevel = stoi( argv[1] );
			if ( MaxLevel < 1 ) throw 1;
		  case 1:										// use defaults
			break;
		  default:
			throw 1;
		} // switch
	} catch( ... ) {
		cout << "Usage: " << argv[0] << " [ maximum level (> 0) ]" << endl;
		exit( EXIT_FAILURE );
	} // try

	PRT( cout << "MaxLevel " << MaxLevel << endl; )
		MaxLevel -= 1;									// decrement to handle created leaves
	uActor::start();									// start actor system
	*new Fork( 0 ) | uActor::startMsg;
	uActor::stop();										// wait for all actors to terminate
	cout << ((2 << MaxLevel) - 1) << " actors created" << endl;
} // main

// Local Variables: //
// compile-command: "u++-work -Wall -g -O2 -multi ActorFork.cc" //
// End: //
