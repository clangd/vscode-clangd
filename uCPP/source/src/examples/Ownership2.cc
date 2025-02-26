//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Ashif S. Harji 2005
// 
// OwnerShip.cc -- 
// 
// Author           : Ashif S. Harji
// Created On       : Sun Jan  9 16:09:19 2005
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Apr 24 18:29:03 2022
// Update Count     : 16
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

_Cormonitor CM {
	void main() {
		osacquire( cout ) << uThisTask().getName() << " (" << &uThisTask() << ") " << uThisCoroutine().getName() << " (" << &uThisCoroutine() << ") " << this << " CM::main enter" << endl;
		uBaseTask::yield();
		osacquire( cout ) << uThisTask().getName() << " (" << &uThisTask() << ") " << uThisCoroutine().getName() << " (" << &uThisCoroutine() << ") " << this << " CM::main exit" << endl;
	} // CM::main
  public:
	void mem() {
		osacquire( cout ) << uThisTask().getName() << " (" << &uThisTask() << ") " << uThisCoroutine().getName() << " (" << &uThisCoroutine() << ") " << this << " CM::mem enter" << endl;
		resume();
		osacquire( cout ) << uThisTask().getName() << " (" << &uThisTask() << ") " << uThisCoroutine().getName() << " (" << &uThisCoroutine() << ") " << this << " CM::mem exit" << endl;
	} // CM::mem
}; // CM

_Coroutine C {
	CM &cm;
	int i;

	void main() {
		osacquire( cout ) << uThisTask().getName() << " (" << &uThisTask() << ") " << uThisCoroutine().getName() << " (" << &uThisCoroutine() << ") " << this << " C::main enter" << endl;
		if ( i == 1 ) cm.mem();
		osacquire( cout ) << uThisTask().getName() << " (" << &uThisTask() << ") " << uThisCoroutine().getName() << " (" << &uThisCoroutine() << ") " << this << " C::main exit" << endl;
	} // C::main
  public:
	C( CM &cm ) : cm( cm ), i( 0 ) {}

	void mem() {
		osacquire( cout ) << uThisTask().getName() << " (" << &uThisTask() << ") " << uThisCoroutine().getName() << " (" << &uThisCoroutine() << ") " << this << " C::mem enter" << endl;
		i += 1;
		resume();
		osacquire( cout ) << uThisTask().getName() << " (" << &uThisTask() << ") " << uThisCoroutine().getName() << " (" << &uThisCoroutine() << ") " << this << " C::mem exit" << endl;
	} // C::mem
}; // C

_Task T {
	C &c;

	void main() {
		osacquire( cout ) << uThisTask().getName() << " (" << &uThisTask() << ") " << uThisCoroutine().getName() << " (" << &uThisCoroutine() << ") " << this << " T::main enter" << endl;
		c.mem();
		osacquire( cout ) << uThisTask().getName() << " (" << &uThisTask() << ") " << uThisCoroutine().getName() << " (" << &uThisCoroutine() << ") " << this << " T::main exit" << endl;
	} // T::main
  public:
	T( C &c, const char *name ) : c( c ) {
		setName( name );
	} // T::T
}; // T

int main() {
	CM cm;
	C c( cm );
	T t1( c, "T1" ), t2( c, "T2" );
} // main
