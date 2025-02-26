// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2016
// 
// ActorRestart.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Dec 19 08:24:42 2016
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Jul 17 08:00:23 2022
// Update Count     : 38
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

struct DummyMsg : public uActor::PromiseMsg< int > {
	int id;
	DummyMsg( int id ) : PromiseMsg( uActor::Delete ), id( id ) {}
}; // DummyMsg


_Actor Restart {
	int cnt = 0;

	void preStart() {
		PRT( osacquire( cout ) << "preStart" << endl; );
	} // Restart::preStart

	Allocation receive( Message & msg ) {
		Case( DummyMsg, msg ) {							// determine message kind
			PRT( osacquire( cout ) << "receive " << msg_d->id << endl; );
			cnt += 1;
			msg_d->delivery( 12 );
		} // Case
		become( &Restart::receive2 );
		return Nodelete;								// reuse actor
	} // Restart::receive

	Allocation receive2( Message & msg ) {
		Case( DummyMsg, msg ) {							// determine message kind
			PRT( osacquire( cout ) << "receive2 " << msg_d->id << endl; );
			cnt += 1;
			msg_d->delivery( 12 );
		} else Case( StopMsg, msg ) {
			PRT( osacquire( cout ) << "stop" << endl; );
			return Delete;								// delete actor
		} // Case
		return Nodelete;								// reuse actor
	} // Restart::receive2
}; // Restart

uActor::Allocation idcb( int id ) { osacquire( cout ) << "callback id " << id << endl; return uActor::Nodelete; };

int main() {
	srandom( getpid() );
	enum { NoOfMsgs = 10 };
	uActor::Promise< int > pi[NoOfMsgs];

	uActor::start();									// start actor system
	Restart *restart = new Restart;						// create actor

	for ( int i = 0; i < NoOfMsgs; i += 1 ) {
		pi[i] = *restart || *new DummyMsg( i );			// send N promise messages
		if ( i == 4 ) restart->restart();				// restart actor to initial state
	} // for
	uThisTask().yield( random() % 7 );					// work asynchronously
	for ( int i = 0; i < NoOfMsgs; i += 1 ) {			// access promise values
		#ifdef THEN
		pi[i].then( idcb );
		#else
		if ( pi[i].maybe( idcb ) ) osacquire( cout ) << "fullfilled id " << pi[i]() << endl;
		#endif // THEN
	} // for

	restart->restart();									// restart actor to initial state
	for ( int i = 0; i < NoOfMsgs; i += 1 ) {
		pi[i] = *restart || *new DummyMsg( i );			// send N promise messages
	} // for
	uThisTask().yield( random() % 7 );					// work asynchronously
	for ( int i = 0; i < NoOfMsgs; i += 1 ) {			// access promise values
		#ifdef THEN
		pi[i].then( idcb );
		#else
		if ( pi[i].maybe( idcb ) ) osacquire( cout ) << "fullfilled id " << pi[i]() << endl;
		#endif // THEN
	} // for

	*restart | uActor::stopMsg;							// stop restart actor
	uActor::stop();										// wait for all actors to terminate
} // main

// Local Variables: //
// compile-command: "u++-work -g -O2 -multi ActorRestart.cc" //
// End: //
