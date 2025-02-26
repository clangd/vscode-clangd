//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2006
// 
// Ownership3.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Wed Feb 22 23:20:03 2006
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 26 15:08:58 2022
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
using std::osacquire;
using std::endl;

_Coroutine C;											// foward declaration

_Monitor M {
  public:
  void mem( C &c );
}; // M

_Coroutine C {
	M &m;

	void main() {
		m.mem( *this );
	} // C::main
  public:
	C( M &m ) : m( m ) {}
	~C() {
		cout << "Here1" << endl;
	}

	void mem() {
		resume();
	} // C::mem

	void mem2() {
		suspend();
	} // C::mem
}; // C

void M::mem( C &c ) {
	c.mem2();
} // M::mem

_Task T {
	C &c;
  public:
	T( C &c, const char *name ) : c( c ) {
		setName( name );
	} // T::T

	void mem() {}
  private:
	void main() {
		c.mem();
		cout << "Here2" << endl;
		_Accept( mem );
		_Accept( mem );
	} // T::main
}; // T

int main() {
	M m;
	C c( m );
	T &t = *new T( c, "T" );
	t.mem();
	cout << "Here3" << endl;
} // main
