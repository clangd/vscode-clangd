//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// CorSemiProdCons.cc -- Producer-Consumer Problem, Semi-Coroutine
// 
// Author           : Peter A. Buhr
// Created On       : Tue Mar 19 21:15:40 1991
// Last Modified By : Peter A. Buhr
// Last Modified On : Wed Apr 20 16:31:48 2022
// Update Count     : 48
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
using std::cout;
using std::endl;

_Coroutine Cons {
	int p1, p2, status, done;							// communication

	void main() {
		int money = 1;
	    // 1st resume starts here
		for ( ;; ) {
			cout << "Consumer receives: " << p1 << ", " << p2;
		  if ( done ) break;
			cout << " and pays $" << money << endl;
	        suspend();									// restart Cons::delivery
			money += 1;
			status += 1;
		} // for
		cout << " and stops" << endl;
	}; // Cons::main
  public:
	Cons() { done = 0; };
	int delivery( int p1, int p2 ) {
	    Cons::p1 = p1;
	    Cons::p2 = p2;
	    resume();										// restart Cons::main
	    return status;
	}; // Concs::delivery
	void stop() {
		done = 1;
		resume();
	}; // Cons::stop
}; // Cons

_Coroutine Prod {
	Cons &cons;											// communication
	int N;

	void main() {
		int i, p1, p2, status;
		// 1st resume starts here
		for ( i = 1; i <= N; i += 1 ) {
			p1 = rand() % 100;							// generate a p1 and p2
			p2 = rand() % 100;
			cout << "Producer delivers: " << p1 << ", " << p2 << endl;
			status = cons.delivery( p1, p2 );
		} // for
		cout << "Producer stopping" << endl;
		cons.stop();
	}; // Prod::main
  public:
	Prod( Cons &c ) : cons(c) {
	} // Prod::Prod
	void start( int N ) {
	    Prod::N = N;
	    resume();										// restart Prod::main
	} // Prod::start
}; // Prod

int main() {
	Cons cons;											// create consumer
	Prod prod( cons );									// create producer

	prod.start( 5 );									// start producer
} // main

// Local Variables: //
// compile-command: "u++ CorSemiProdCons.cc" //
// End: //
