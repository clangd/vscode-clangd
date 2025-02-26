//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2006
// 
// CRAII.cc -- Test deletion of a non-terminated coroutine results in deallocation of all objects on its stack.
// 
// Author           : Peter A. Buhr
// Created On       : Thu Feb 23 21:42:16 2006
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr 23 18:26:22 2022
// Update Count     : 55
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

const unsigned int NoInTest = 4;
const unsigned int MaxStackDepth = 4;

int check = 0;

class RAII {
  public:
	RAII() {
		check += 1;
		//cout << "RAII constructor (" << this << ") :" << check << endl;
	}
	~RAII() {
		check -= 1;
		//cout << "RAII destructor(" << this << "):" << check << endl;
	}
}; // RAII

_Coroutine CRAII {
	RAII ol[NoInTest];									// object level

	void main() {
		RAII sl[NoInTest] __attribute__(( unused ));	// stack level
		//cout << "coroutine(" << this << ") starts:" << check << endl;

		suspend();										// return without termination
		assert( false );								// CONTROL NEVER REACHES HERE!
	}
  public:
	CRAII() {
		check += 1;
		//cout << "CRAII constructor(" << this << ") :" << check << endl;
	}
	~CRAII() {
		check -= 1;
		//cout << "CRAII destructor(" << this << "):" << check << endl;
	}
	void mem() { resume(); }
}; // CRAII

_Coroutine C {
	CRAII ol[NoInTest];									// object level

	void main() {
		CRAII sl[NoInTest];								// stack level

		for ( unsigned int i = 0; i < NoInTest; i += 1 ) {
			ol[i].mem();								// start coroutines
		} // for
		for ( unsigned int i = 0; i < NoInTest; i += 1 ) {
			sl[i].mem();								// start coroutines
		} // for

		//cout << "C main(" << this << "):" << check << endl;
		suspend();										// return without termination
		assert( false );								// CONTROL NEVER REACHES HERE!
	} // C::main
  public:
	void mem() { resume(); }
}; // C


//***********************************************************************************


int checkarray[NoInTest] = { 0 };
bool flags[NoInTest] = { false };

class SRAII {											// silent RAII
	unsigned int id;
  public:
	SRAII( unsigned int i ) : id( i ) {
		checkarray[i] += 1;
	} // SRAII::SRAII

	~SRAII() {
		checkarray[id] -= 1;
		if ( uThisTask().cancelInProgress() ) {
			assert( flags[id] );
			//osacquire( cout ) << "Task " << id << " being cancelled" << endl;
		} // if
	} // SRAII::~SRAII
}; // SRAII


_Task Cancelee {
	static unsigned int idno;
	unsigned int id;
	SRAII ol;
	
	void main() {
		for ( unsigned int i = 0; i < 100000; i+= 1 ) {
			rec();
			assert( checkarray[id] > 0 );
		} // for
		flags[id] = ! flags[id];
	} // Cancelee::main
  public:
	Cancelee() : id( idno ), ol( id ) {
		if ( id == 0 ) setCancelState( CancelDisabled ); // task 0 never enables cancellation
		idno += 1;
	} // Cancelee::Cancelee
	
	void rec() {
		yield();										// check for stack overflow
		if ( rand() % MaxStackDepth != 0 ) {			// base case for random recursion depth
			SRAII sl( id );
			rec();	    
		} // if
		// stack has formed with RAII objects in each frame
		if ( id != 0 ) {								// deliver cancellation
			assert( getCancelState() == CancelEnabled );
			uEnableCancel dummy;						// implicit poll
		} else {
			assert( getCancelState() == CancelDisabled );
			uEHM::poll();								// task 0 resists cancellation
		} // if
	} // Cancelee::rec
}; // Cancelee

unsigned int Cancelee::idno = 0;


int main() {
	uProcessor processors[3] __attribute__(( unused ));
	srand( getpid() );

	// Coroutine cancellation test : deletion of a nonterminated coroutine implicitly forces its cancellation.
	{ 
		C c;
		c.mem();
	}
	if ( check != 0 ) abort( "not all destructors called" );

	// Task cancellation test
	{
		Cancelee ca[NoInTest];
		uBaseTask::yield();
		ca[0].cancel();									// special check for complete _Disable
	    for ( unsigned int i = 1; i < NoInTest; i += 1 ) {
			if ( rand() % 2 != 0 ) {
				flags[i] = true;
				ca[i].cancel();
			} // if
		} // for
	}
	for ( unsigned int i = 0; i < NoInTest; i += 1 ) {
		if ( checkarray[i] != 0 || ! flags[i] ) abort( "unsuccessful task cancellation" );
	} // for

	cout << "successful completion" << endl;
} // main
