//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// Inheritance.cc -- test inheritance of tasks
// 
// Author           : Peter A. Buhr
// Created On       : Thu Mar 22 20:20:04 1990
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 26 13:30:14 2022
// Update Count     : 59
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

/*
  Correct output:

  super::main
  sub::main
  super::fred: 3
  super::mary: 3
  sub::fred: 5
  sub::mary: 7
  sub::jane: a
  sub::main terminating
  sub::~sub
  super::~super
  super::main terminating
  super::~super
*/

#include <iostream>
using std::cout;
using std::osacquire;
using std::endl;

_Task super {
	void main();
  public:
	virtual ~super();
	int fred( int );
	virtual float mary( float );
}; // super

void super::main() {
	osacquire( cout ) << "super::main" << endl;
	for ( ;; ) {
		_Accept( ~super ) {
			break;
		} or _Accept( fred ) {
		} or _Accept( mary ) {
		} // accept
	} // for
	osacquire( cout ) << "super::main terminating" << endl;
} // super::main

super::~super() {
	osacquire( cout ) << "super::~super" << endl;
} // super::~super

int super::fred( int n ) {
	osacquire( cout ) << "super::fred: " << n << endl;
	return n;
} // super::fred

float super::mary( float f ) {
	osacquire( cout ) << "super::mary: " << f << endl;
	return f;
} // super::mary

_Task sub : public super {
	void main();
  public:
	virtual ~sub();
	int fred( int );
	virtual float mary( float );
	char jane( char );
}; // sub

void sub::main() {
	osacquire( cout ) << "sub::main" << endl;
	for ( ;; ) {
		_Accept( ~sub ) {
			break;
		} or _Accept( fred ) {
		} or _Accept( mary ) {
		} or _Accept( jane ) {
		} // accept
	} // for
	osacquire( cout ) << "sub::main terminating" << endl;
} // sub::main

sub::~sub() {
	osacquire( cout ) << "sub::~sub" << endl;
} // sub::~sub

int sub::fred( int n ) {
	osacquire( cout ) << "sub::fred: " << n << endl;
	return n;
} // sub::fred

float sub::mary( float f ) {
	osacquire( cout ) << "sub::mary: " << f << endl;
	return f;
} // sub::mary

char sub::jane( char c ) {
	osacquire( cout ) << "sub::jane: " << c << endl;
	return c;
} // sub::jane

int main() {
	super S;
	sub s;

	S.fred( 3 );
	S.mary( 3.0 );
	s.fred( 5 );
	s.mary( 7.0 );
	s.jane( 'a' );
} // main

// Local Variables: //
// compile-command: "u++ Inheritance.cc" //
// End: //
