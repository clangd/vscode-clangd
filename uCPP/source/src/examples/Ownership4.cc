//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2006
// 
// Ownership4.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Fri Feb 24 17:30:59 2006
// Last Modified By : Peter A. Buhr
// Last Modified On : Wed Apr 20 23:11:25 2022
// Update Count     : 14
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
_Cormonitor CM;

_Monitor M {
	CM &c;
  public:
	M( CM &c ) : c( c ) {}
	void mem();
}; // M

_Coroutine C {
	M &m;

	void main() {
		m.mem();
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

_Cormonitor CM {
	uCondition c;
  public:
	CM() {}

	void mem() {
		resume();
	} // CM::mem

	void mem2() {
		cout << "Here1.1" << endl;
		c.wait();
	} // CM::mem2
  private:
	void main() {
		cout << "Here2" << endl;
		c.signalBlock();
		_Accept( mem2 );
		cout << "Here3" << endl;
	} // C::main
}; // CM

void M::mem() {
	c.mem();
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
		cout << "Here4" << endl;
	} // T::main
}; // T

int main() {
	CM cm;
	M m( cm );
	C c( m );
	T &t = *new T( c, "T" );
	cm.mem2();
	cout << "Here5" << endl;
} // main
