//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2020
// 
// EHM11.cc -- ping-pong using resumption (algebraic effects)
// 
// Author           : Peter A. Buhr
// Created On       : Tue Jul 28 07:23:06 2020
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Oct  7 08:08:24 2023
// Update Count     : 3
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
#include <chrono>

using namespace std;

_Exception Ping {};
_Exception Pong {};

void pinger( int i, int N ) {
	cout << "Enter pinger " << i << ' ' << N << endl;
	try {
	    if (i < N) {
	        Ping pi;
	        cout << "Raising Ping from pinger" << endl;
	        _ResumeTop pi;
	        cout << "Return Ping from pinger" << endl;
	    }
	} _CatchResume( Pong & ) {
	    cout << "Caught Pong in pinger" << endl;
	    pinger( i + 1, N );
	    cout << "Return Pong in pinger" << endl;
	}
	cout << "Exit pinger " << i << ' ' << N << endl;
}

void ponger1() {
	cout << "Enter ponger1" << endl;
	Pong po;
	cout << "Raising Pong from ponger1" << endl;
	_ResumeTop po;
	cout << "Return Pong from ponger1" << endl;
	cout << "Exit ponger1" << endl;
}

void ponger2() {
	cout << "Enter ponger2" << endl;
	try {
	    Pong po;
	    cout << "Raising Pong from ponger2" << endl;
	    _ResumeTop po;									// This Pong appears to go unhandled
	    cout << "Return Pong from ponger2" << endl;
	} _CatchResume( Ping & ) {
	    cout << "Caught Ping in ponger2" << endl;
	    ponger2();
	    cout << "Return Ping in ponger2" << endl;
	}
	cout << "Exit ponger2" << endl;
}

void test( void ponger() ) {
	cout << "Enter test" << endl;
	try {
	    pinger( 0, 1 );
	} _CatchResume ( Ping & ) {
	    cout << "Caught Ping in test" << endl;
	    ponger();
	    cout << "Return Ping in test" << endl;
	}
	cout << "Exit test" << endl;
}

int main() {
	cout << "Testing ponger1..." << endl;
	test(ponger1);
	cout << "Testing ponger2..." << endl;
	test(ponger2);
}
