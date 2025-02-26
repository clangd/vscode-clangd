// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2020
// 
// ActorFib4.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Fri Apr 30 08:14:04 2021
// Last Modified By : Peter A. Buhr
// Last Modified On : Wed Jul  6 09:18:18 2022
// Update Count     : 31
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

_CorActor Fib {
	FibMsg * fmsg;										// communication
	long int * fn;										// pointer into return message

	Allocation receive( Message & msg ) {
		Case( FibMsg, msg ) {
			fmsg = msg_d;								// coroutine communication
			fn = &msg_d->fn;							// compute answer in message
			resume();
		} else Case( StopMsg, msg ) return Finished;
		return Nodelete;
	} // Fib::receive

	void main() {
		long int fn1, fn2;

		*fn = 0; fn1 = *fn;								// fib(0) => 0
		*fmsg->sender() | *fmsg;						// return fn
		suspend();

		*fn = 1; fn2 = fn1; fn1 = *fn;					// fib(1) => 1
		*fmsg->sender() | *fmsg;						// return fn
		suspend();

		for ( ;; ) {
			*fn = fn1 + fn2; fn2 = fn1; fn1 = *fn;		// fib(n) => fib(n-1) + fib(n-2)
			*fmsg->sender() | *fmsg;					// return fn
			suspend();
		} // for
	} // Fib::main
}; // Fib

int Times = 10;											// default values

_CorActor Generator {
	Fib fib;											// create fib actor
	FibMsg fibMsg;										// only message
	FibMsg * fmsg;										// communication
	Allocation state = Nodelete;

	void preStart() {
		fib | fibMsg;									// start generator
	} // Generator::preStart

	Allocation receive( Message & msg ) {
		Case( FibMsg, msg ) {
			fmsg = msg_d;								// coroutine communication
			resume();
		} // Case
		return state;
	} // Generator::receive

	void main() {
		for ( int i = 0; i < Times; i += 1 ) {
			cout << fmsg->fn << endl;
			fib | fibMsg;
			suspend();
		} // for
		fib | stopMsg;									// stop fib actor
		state = Finished;								// mark as done
	} // Generator::main
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
// compile-command: "u++-work -g -O2 -multi ActorFib4.cc" //
// End: //
