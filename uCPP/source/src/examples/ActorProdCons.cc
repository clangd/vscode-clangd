// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2016
// 
// ActorProdCons.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Wed Jul 25 13:05:45 2018
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Jan 29 21:19:55 2023
// Update Count     : 428
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

//#define NOOUTPUT

#ifdef NOOUTPUT
#define PRT( stmt )
#else
#define PRT( stmt ) stmt
#endif // NOOUTPUT

enum { N = 2 };
size_t uDefaultExecutorProcessors() { return N; }
size_t uDefaultExecutorWorkers() { return N; }
size_t uDefaultExecutorRQueues() { return N; }
bool uDefaultExecutorSepClus() { return true; }
int uDefaultExecutorAffinity() { return 0; }

struct ValueMsg : public uActor::Message {
	size_t value;
	ValueMsg( size_t value ) : Message( uActor::Delete ), value( value ) {}
};

_Actor Cons {
	Allocation receive( uActor::Message & msg ) {
		Case( ValueMsg, msg ) {							// receive value
			PRT( osacquire( cout ) << "cons " << this << " remove " << msg_d->value << endl; )
			//for ( volatile size_t delay = 0; delay < 100; delay += 1 );
		} else Case( StopMsg, msg ) return Delete;		// stop consuming
		else abort( "Cons unknown message" );
		return Nodelete;								// reuse actor
	}
}; // Cons

Cons ** consumers; 										// consumer array, consumers are self-delete

static struct DelayMsg : public uActor::Message {} delayMsg; // Delay producer

_Actor Prod {
	static size_t stops;
	const size_t prods, cons, times;
	size_t i = -1;

	Allocation receive( uActor::Message & ) {
		for ( i += 1 ; i < times; i += 1 ) {			// loop return  misses increment
			//for ( volatile size_t delay = 0; delay < 100; delay += 1 );
			PRT( osacquire( cout ) << "prod " << this << " " << i << endl; );
			// WARNING: This allocation can flood the system with messages and cause huge contention in the heap.
			*(consumers[i % cons]) | *new ValueMsg{ i }; // round-robin consumer send
			if ( i % 10 == 0 ) {						// allow consumers to run
				*this | delayMsg;
				return Nodelete;
			} // if
		} // for
		uFetchAdd( stops, 1 );
		if ( stops == prods ) for ( size_t i = 0; i < cons; i += 1 ) *consumers[i] | uActor::stopMsg;
		return Delete;
	}
  public:
	Prod( size_t prods, size_t cons, size_t times ) : prods( prods ), cons( cons ), times( times ) {}
}; // Prod

size_t Prod::stops = 0;

void check( const char * arg, int & value ) {
	if ( strcmp( arg, "d" ) != 0 ) {					// user default ?
		value = stoi( arg );
		if ( value < 1 ) throw 1;
	} // if
} // check

int main( int argc, char * argv[] ) {
	int prods = 1, cons = 1, times = 10;
	try {
		switch ( argc ) {
		  case 4:
			check( argv[3], times );
		  case 3:
			check( argv[2], cons );
		  case 2:
			check( argv[1], prods );
		  case 1:										// use defaults
			break;
		  default:
			throw 1;
		} // switch
	} catch( ... ) {
		cout << "Usage: " << argv[0] << " [ prods (> 0) | 'd' [ cons (>0) | 'd' [ times (>0) ] | 'd' ] ]" << endl;
		exit( EXIT_SUCCESS );
	} // try
	cout << "producers " << prods << " consumers " << cons << " times " << times << endl;

	uProcessor p[prods + cons - 1];
	uActor::start();									// wait for all actors to terminate
	consumers = new Cons *[cons];						// large size => use heap
	for ( int i = 0; i < cons; i += 1 ) consumers[i] = new Cons; // create consumers
	for ( int i = 0; i < prods; i += 1 ) *(new Prod( prods, cons, times ) ) | uActor::startMsg; // create and start producers
	uActor::stop();										// wait for all actors to terminate
	delete [] consumers;
	malloc_stats();										// print memory statistics
} // main

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" a.out 320 320 100000

// Local Variables: //
// tab-width: 4 //
// compile-command: "u++-work -g -O2 -nodebug -multi ActorProdCons.cc -DNOOUTPUT" //
// End: //
