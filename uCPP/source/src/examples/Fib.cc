//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// Fib.cc -- Produce the fibonacci numbers in sequence on each call.
//
//  No explicit states, communication with argument-parameter mechanism between suspend and resume
//
//  Demonstrate multiple instances of the same coroutine.
// 
// Author           : Peter A. Buhr
// Created On       : Thu Aug  2 11:55:37 1990
// Last Modified By : Peter A. Buhr
// Last Modified On : Wed Apr 20 23:04:08 2022
// Update Count     : 42
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

_Coroutine fibonacci {
	int fn;

	void main() {
		int fn1, fn2;

		fn = 1;											// special case f0
		fn1 = fn;
		suspend();
		fn = 1;											// special case f1
		fn2 = fn1;
		fn1 = fn;
		suspend();
		for ( ;; ) {									// general case fn
			fn = fn1 + fn2;
			suspend();
			fn2 = fn1;
			fn1 = fn;
		} // for
	} // fibonacci::main
  public:
	int next() {
		resume();
		return fn;
	}; // next
}; // fibonacci

int main() {
	const int NoOfFibs = 10;
	fibonacci f1, f2;									// create two fibonacci generators
	int i;

	cout << "Fibonacci Numbers" << endl;
	for ( i = 1; i <= NoOfFibs; i += 1 ) {
		cout << f1.next() << " " << f2.next() << endl;
	} // for
	cout << "successful completion" << endl;
} // main

// Local Variables: //
// compile-command: "u++ Fib.cc" //
// End: //
