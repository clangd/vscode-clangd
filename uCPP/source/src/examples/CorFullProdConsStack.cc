//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2007
// 
// CorFullProdConsStack.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Jul  2 10:11:13 2007
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Apr 24 09:51:13 2022
// Update Count     : 13
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

const unsigned int stackSize = 8 * 1024;
char prodStack[stackSize] __attribute__(( aligned (8) ));
char consStack[stackSize] __attribute__(( aligned (16) ));

_Coroutine Prod;										// forward declaration

_Coroutine Cons {
	Prod &prod;											// communication
	int p1, p2, status, done;
	void main();
  public:
	Cons( Prod &p ) : uBaseCoroutine( ::consStack, ::stackSize ), prod( p ), status( 0 ), done( 0 ) {
	} // Cons::Cons

	int delivery( int p1, int p2 ) {
		Cons::p1 = p1;
		Cons::p2 = p2;
		resume();										// restart cons in Cons::main 1st time
		return status;									// and cons in Prod::payment afterwards
	}; // Cons::delivery

	void stop() {
		done = 1;
		resume();
	}; // Cons::stop
}; // Cons

_Coroutine Prod {
	Cons *cons;											// communication
	int N, money, receipt;

	void main() {
	    int i, p1, p2, status;
	    // 1st resume starts here
	    for ( i = 1; i <= N; i += 1 ) {
			p1 = rand() % 100;							// generate a p1 and p2
			p2 = rand() % 100;
			cout << "Producer delivers: " << p1 << ", " << p2 << endl;
			status = cons->delivery( p1, p2 );
			cout << "Producer deliver status: " << status << endl;
	    } // for
		cout << "Producer stopping" << endl;
	    cons->stop();
	}; // main
  public:
	Prod() : uBaseCoroutine( ::prodStack, ::stackSize ) {}
	int payment( int money ) {
		Prod::money = money;
		cout << "Producer receives payment of $" << money << endl;
	    resume();										// restart prod in Cons::delivery
		receipt += 1;
	    return receipt;
	}; // payment

	void start( int N, Cons *c ) {
		Prod::N = N;
		cons = c;
		receipt = 0;
	    resume();
	}; // start
}; // Prod

void Cons::main() {
	int money = 1, receipt;
	// 1st resume starts here
	for ( ;; ) {
		cout << "Consumer receives: " << p1 << ", " << p2;
	  if ( done ) break;
		status += 1;
		cout << " and pays $" << money << endl;
		receipt = prod.payment( money );
		cout << "Consumer receives receipt #" << receipt << endl;
		money += 1;
	} // for
	cout << " and stops" << endl;
}; // Cons::main

int main() {
	Prod prod;
	Cons cons( prod );

	prod.start( 5, &cons );
} // main

// Local Variables: //
// compile-command: "u++ CorFullProdCons.cc" //
// End: //
