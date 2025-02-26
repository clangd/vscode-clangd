// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2017
// 
// ActorFork2.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Wed Jan 11 08:18:18 2017
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr 23 16:45:03 2022
// Update Count     : 15
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
	unsigned int total;
	unsigned int leafNodes;
	Allocation receive( Message & msg );
  public:
	Fork() : total(0), leafNodes( (2 << MaxLevel) - 2 ) {}
}; // Fork

Fork * root;											// root node

_Actor Node {
	unsigned int level;

	Allocation receive( Message & msg ) {
		Case( StartMsg, msg ) {
			PRT( osacquire( cout ) << this << " Node level " << level << endl; )
				if ( level < (unsigned int)MaxLevel ) {
					*(new Node( level + 1 )) | startMsg; // left
					*(new Node( level + 1 )) | startMsg; // right
				} // if
		} // Case

		*root | stopMsg;
	    return Delete;
	} // Node::receive
  public:
	Node( unsigned int level ) : level( level ) {}
}; // Node

uActor::Allocation Fork::receive( Message &msg ) {
	Case( StartMsg, msg ) {
		PRT( osacquire( cout ) << this << " Fork level 0 " << endl; )
			*(new Node( 1 )) | startMsg;				// left
		*(new Node( 1 )) | startMsg;					// right
	} else Case( StopMsg, msg ) {
			PRT( osacquire( cout ) << this << " Fork node ends " << endl; )
				total += 1;
			if ( total == leafNodes ) {					// all node ended ?
				return Delete;
			} // if
		} // Case
	return Nodelete;
} // Fork::receive

int main( int argc, char *argv[] ) {
	try {
		switch ( argc ) {
		  case 2:
			MaxLevel = stoi( argv[1] );
			if ( MaxLevel < 2 ) throw 1;
		  case 1:										// use defaults
			break;
		  default:
			throw 1;
		} // switch
	} catch( ... ) {
		cout << "Usage: " << argv[0] << " [ maximum level (> 1) ]" << endl;
		exit( EXIT_SUCCESS );
	} // try

	PRT( cout << "MaxLevel " << MaxLevel << endl; )
		MaxLevel -= 1;									// decrement to handle created leaves
	uActor::start();									// start actor system
	root = new Fork();
	*root | uActor::startMsg;
	uActor::stop();										// wait for all actors to terminate
	cout << ((2 << MaxLevel) - 1) << " actors created" << endl;
} // main

// Local Variables: //
// compile-command: "u++-work -Wall -g -O2 -multi ActorFork2.cc" //
// End: //
