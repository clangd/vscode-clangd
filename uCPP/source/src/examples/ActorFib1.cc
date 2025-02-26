// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2016
// 
// ActorFib1.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sun Dec 18 23:46:22 2016
// Last Modified By : Peter A. Buhr
// Last Modified On : Wed Jul  6 09:25:50 2022
// Update Count     : 76
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

struct FibMsg : public uActor::SenderMsg { long int fn; }; // Nodelete

_Actor Fib {
	long int fn1, fn2;
	int state = 1;

	Allocation receive( Message & msg ) {
		Case( FibMsg, msg ) {
			long int & fn = msg_d->fn;					// compute answer in message
			switch( state ) {							// discriminate Fibonacci state
			  case 1:
				fn = 0; fn1 = fn;						// fib(0) => 0
				state = 2;
				break;
			  case 2:
				fn = 1; fn2 = fn1; fn1 = fn;			// fib(1) => 1
				state = 3;
				break;
			  case 3:
				fn = fn1 + fn2; fn2 = fn1; fn1 = fn;	// fib(n) => fib(n-1) + fib(n-2)
				break;
			} // switch
			*msg_d->sender() | *msg_d;					// return fn
		} else Case( StopMsg, msg ) return Finished;
		return Nodelete;
	} // Fib::receive
}; // Fib

int Times = 10;											// default values

_Actor Generator {
	int i = 0;
	Fib fib;
	FibMsg figMsg;

	void preStart() {
		fib | figMsg;
	} // Generator::preStart

	Allocation receive( Message & msg ) {
		if ( i < Times ) {
			Case( FibMsg, msg ) {
				cout << msg_d->fn << endl;
				fib | figMsg;
			} // Case
			i += 1;
			return Nodelete;
		} else {
			fib | stopMsg;
			return Finished;
		} // if
	} // Fib::receive
}; // Generator

int main( int argc, char * argv[] ) {
	try {
		switch ( argc ) {
		  case 2:
			Times = stoi( argv[1] );
			if ( Times < 1 ) throw 1;
		  case 1:										// use defaults
			break;
		  default:
			throw 1;
		} // switch
	} catch( ... ) {
		cout << "Usage: " << argv[0] << " [ numbers (> 0) ]" << endl;
		exit( EXIT_FAILURE );
	} // try

	uActor::start();									// start actor system
	Generator fib1, fib2;
	uActor::stop();										// wait for all actors to terminate
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "u++-work -g -O2 -multi ActorFib1.cc" //
// End: //
