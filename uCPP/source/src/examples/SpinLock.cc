//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// SpinLock.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Feb 13 15:47:48 1995
// Last Modified By : Peter A. Buhr
// Last Modified On : Wed Apr 20 23:10:52 2022
// Update Count     : 19
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


unsigned int uDefaultPreemption() {
	return 1;
} // uDefaultPreemption

void CriticalSection() {
	static volatile uBaseTask *CurrTid;					// current task id
	CurrTid = &uThisTask();								// address of current task
	
	for ( int i = 1; i <= 100; i += 1 ) {				// delay
		// perform critical section operation
		if ( CurrTid != &uThisTask() ) {				// check for mutual exclusion violation
			abort( "interference" );
		} // if
	} // for
} // CriticalSection   

uSpinLock Lock;

_Task Tester {
	void main() {
		for ( int i = 1; i <= 10000000; i += 1 ) {
			::Lock.acquire();
			CriticalSection();							// critical section
			::Lock.release();
		} // for
	} // main
  public:
	Tester() {}
}; // Tester
	
int main() {
	uProcessor processor[1] __attribute__(( unused ));	// more than one processor
	Tester t[10];
} // main

// Local Variables: //
// compile-command: "../../bin/u++ -multi -g SpinLock.cc" //
// End: //
