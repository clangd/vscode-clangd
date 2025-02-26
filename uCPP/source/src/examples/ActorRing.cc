// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2016
// 
// ActorRing.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Dec 19 08:25:19 2016
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Apr 24 09:53:57 2022
// Update Count     : 25
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

#ifdef NOOUTPUT
#define PRT( stmt )
#else
#define PRT( stmt ) stmt
#endif // NOOUTPUT

struct Token : public uActor::Message { unsigned int cnt = 0; } token; // token passed around ring
int RingSize = 3, Times = 10;				// default values

// A general version of the ping/pong pattern with N actors arranged in a ring passing a positive integer token T times.

_Actor Passer {
	int id;
	Passer * partner;									// chain actors

	Allocation receive( Message & msg ) {
		Case( Token, msg ) {							// determine message kind
			// msg_d is an implicit new variable of type "Token *"
			PRT( osacquire( cout ) << id << " " << msg_d->cnt << endl; )
				msg_d->cnt += 1;
			if ( msg_d->cnt >= (unsigned int)Times ) {
				// force token completely around the cycle so all actors stop
				if ( msg_d->cnt < (unsigned int)Times + RingSize - 1 ) *partner | msg;
				PRT( osacquire( cout ) << id << " stopping " << msg_d->cnt << endl; )
					return Delete;						// delete actor
			} else {
				*partner | msg;							// pass token to partner
			} // if
		} // Case
		return Nodelete;								// reuse actor
	} // Ping::receive
  public:
	Passer( int id ) : id( id ) {}
	Passer( int id, Passer *partner ) : id( id ), partner( partner ) {}
	void close( Passer *partner ) { Passer::partner = partner; }
}; // Passer

int main( int argc, char * argv[] ) {
	try {
		switch ( argc ) {
		  case 3:
			Times = stoi( argv[2] );
			if ( Times < 1 ) throw 1;
		  case 2:
			RingSize = stoi( argv[1] );
			if ( RingSize < 1 ) throw 1;
		  case 1:										// use defaults
			break;
		  default:
			throw 1;
		} // switch
	} catch( ... ) {
		cout << "Usage: " << argv[0] << " [ ring-size (> 0) [ rounds (> 0) ] ]" << endl;
		exit( EXIT_FAILURE );
	} // try

	Passer * passers[RingSize];							// actors in cycle

	uActor::start();					// start actor system
	// create cycle of actor; special case to close cycle
	passers[RingSize - 1] = new Passer( RingSize - 1 );	// player without partner
	for ( int p = RingSize - 2; 0 <= p; p -= 1 ) {		// link all but one player
		passers[p] = new Passer( p, passers[p + 1] );
	} // for
	passers[RingSize - 1]->close( passers[0] );			// close cycle by linking head to tail
	
	*passers[0] | token;								// start cycling token
	uActor::stop();										// wait for all actors to terminate
} // main

// Local Variables: //
// compile-command: "u++-work -Wall -g -O2 -multi ActorRing.cc" //
// End: //
