//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Roy Krischer 2002
// 
// EHM4.cc -- 
// 
// Author           : Roy Krischer
// Created On       : Tue Mar 26 23:01:30 2002
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Oct  7 08:08:15 2023
// Update Count     : 220
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
#include <iostream>
using std::cout;
using std::osacquire;
using std::endl;

#define NTASK 5
#define ROUNDS 10000

_Task worker {
	int id, round;

	void main();
  public:
	worker ( int id ) : id(id), round(ROUNDS) {
		osacquire( cout ) << "task " << id << " creation" << endl;
	} 
	~worker() {
		osacquire( cout ) << "task " << id << " destruction" << endl;
	}
}; // worker


uBarrier b( NTASK + 1 );								// control start and finish of main/worker tasks
worker * w[NTASK];										// shared resource controlled by barrier

_Exception Rev {
  public:
	int ticket;
	Rev( const char * msg, int ticket ) : ticket( ticket ) { setMsg( msg ); }
};

void worker::main() {
	int cnt = 0;

	b.block();											// wait for all tasks to start

	try {
		_Enable {
			for ( int n = 0; n < ROUNDS; n += 1 ) {		// generate other 1/2 of the exceptions
				yield();								// allow delivery of concurrent resumes
				for ( int i = 0; i < NTASK; i += 1 ) {	// send exceptions to other tasks
					_Resume Rev( "other", /*uFetchAdd( tickets, 1 )*/ 0 ) _At *w[i];
				} // for
			} // for
			// retrieve outstanding non-local exceptions
			for ( ; cnt < ROUNDS * NTASK; ) { yield(); } // yield => poll
		} // _Enable
	} _CatchResume ( Rev & ) {
		cnt += 1;
	} // try

	b.block();											// wait for all tasks to finish
	osacquire( cout ) << "task " << id << " finishing " << cnt << endl;
} // worker::main


int main() {
	uProcessor processors[4] __attribute__(( unused ));	// more than one processor

	for ( int i = 0; i < NTASK; i += 1 ) {
		w[i] = new worker( i );
	} // for
	b.block();											// wait for all tasks to start

	b.block();											// wait for all tasks to finish
	for ( int i = 0; i < NTASK; i += 1 ) {
		delete w[i];
	} // for
} // main


// Local Variables: //
// tab-width: 4 //
// End: //
