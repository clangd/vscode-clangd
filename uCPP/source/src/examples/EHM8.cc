//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2003
// 
// EHM8.cc -- 
// 
// Author           : Roy Krischer
// Created On       : Wed Oct  8 22:02:29 2003
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Oct  7 08:05:33 2023
// Update Count     : 90
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

#if __GNUC__ >= 12										// valid GNU compiler diagnostic ?
// For backwards compatibility, keep testing dynamic-exception-specifiers until they are no longer supported.
#pragma GCC diagnostic ignored "-Wdeprecated-declarations" // g++12 deprecates unexpected_handler
#endif // __GNUC__ >= 12

#include <iostream>
using std::cout;
using std::osacquire;
using std::endl;

#define ROUNDS  10000
#define NP 8

_Exception E1 {};
_Exception E2 {};

void one() {
	abort( "invalid 1\n" );
}
void two() {
	abort( "invalid 2\n" );
}
void three() {
	osacquire( cout ) << "success" << endl;
	exit( EXIT_SUCCESS );
}

_Task fred {
	int id;

	void main() {
		if ( id % 2 ) { 
			std::set_terminate( one );
			std::set_unexpected( two );
		} else {
			std::set_terminate( two );
			std::set_unexpected( one );
		} // if

		for ( int i = 0; i < ROUNDS; i += 1 ) {
			yield();
			if ( id % 2 ) {
				assert( one == std::set_terminate( one ) );
				assert( two == std::set_unexpected( two ) );
			} else {
				assert( two == std::set_terminate( two ) );
				assert( one == std::set_unexpected( one ) );
			} // if	    
		} // for
	} // fred::main
  public:
	fred( int id ) : id( id ) {}
}; // fred


void T1() {
	osacquire( cout ) << "T1" << endl;
	_Throw E2();
}
void T2() {
	osacquire( cout ) << "T2" << endl;
	_Throw E2();
}
void T3() {
	osacquire( cout ) << "T3" << endl;
	_Throw E2();
}

#if __cplusplus < 201703L // c++17
// For backwards compatibility, keep testing dynamic-exception-specifiers until they are no longer supported.
#pragma GCC diagnostic ignored "-Wdeprecated"

_Task mary {
  public:
	void mem() throw(E2) {
		_Throw E1();
	}
  private:
	void m1() throw(E2) {
		std::set_unexpected( T3 );
		_Throw E1();
	}
	void m2() throw(E2) {
		_Throw E1();
	}
	void main() {
		try {
			_Accept( mem );
		} catch( uMutexFailure::RendezvousFailure & ) {
			osacquire( cout ) << "mary::main 0 caught uRendezvousFailure" << endl;
		}
		try {
			m1();
		} catch( E2 & ) {
			osacquire( cout ) << "mary::main 1 caught E2" << endl;
		}
		std::set_unexpected( T1 );
		try {
			m2();
		} catch( E2 & ) {
			osacquire( cout ) << "mary::main 2 caught E2" << endl;
		}
	}
};
#endif // __cplusplus <= 201703L

int main() {
	uProcessor processors[NP - 1] __attribute__(( unused )); // more than one processor
	fred *f[NP];
	int i;

	for ( i = 0; i < NP; i += 1 ) {
		f[i] = new fred( i );
	} // for
	for ( i = 0; i < NP; i += 1 ) {
		delete f[i];
	} // for

#if __cplusplus < 201703L // c++17
	mary m;
	std::set_unexpected( T2 );
	try {
		m.mem();
	} catch( E2 & ) {
		osacquire( cout ) << "main caught E2" << endl;
	} // try
#endif // __cplusplus <= 201703L
} // main
