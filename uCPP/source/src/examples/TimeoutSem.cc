//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2007
// 
// TimeoutSem.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Jun 25 12:07:16 2007
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr 23 16:43:47 2022
// Update Count     : 73
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

#include <uBarrier.h>
#include <uSemaphore.h>
#include <iostream>
using std::cout;
using std::osacquire;
using std::endl;

uSemaphore sem(0);
_Cormonitor Mybarrier : public uBarrier {
	void last() {
		// signallee's P timeout increments (Vs) semaphore, but signaller also Vs semaphore. This race cannot be
		// detected outside the barrier. Therefore, check if timeout occurred and subtract off extra V before starting
		// the next sequence.
		if ( sem.counter() > 0 ) sem.P();
		resume();
	}
  public:
	Mybarrier( unsigned int total ) : uBarrier( total ) {} 
};
Mybarrier b( 2 );

const unsigned int NoOfTimes = 20;

_Task T1 {
	void main(){
		sem.P( uDuration( 1 ) );
		osacquire( cout ) << &uThisTask() << " timedout" << endl;

		b.block();

		// Test calls which occur increasingly close to timeout value.

		for ( unsigned int i = 0; i < NoOfTimes + 3; i += 1 ) {
			if ( sem.P( uDuration( 1 ) ) ) { 
				osacquire( cout ) << &uThisTask() << " signalled" << endl;
			} else {
				osacquire( cout ) << &uThisTask() << " timedout" << endl;
			} // if

			b.block();
		} // for
	} // t1::main
  public:
}; // T1

_Task T2 {
	void main(){
		// Test if timing out works.

		b.block();

		// Test calls which occur increasingly close to timeout value.

		_Timeout( uDuration( 0, 100000000 ) );
		sem.V();
		b.block();

		_Timeout( uDuration( 0, 500000000 ) );
		sem.V();
		b.block();

		_Timeout( uDuration( 0, 900000000 ) );
		sem.V();
		b.block();

		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			_Timeout( uDuration( 0, 999950000 ) );
			sem.V();
			b.block();
		} // for
	} // for
}; // T2::main

int main(){
	uProcessor processor[1] __attribute__(( unused ));	// more than one processor
	T1 r1;
	T2 r2;
} // main

// Local Variables: //
// compile-command: "u++ TimeoutSem.cc" //
// End: //
