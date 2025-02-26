//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2016
// 
// Executor.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Dec 19 08:47:36 2016
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Feb 18 08:43:00 2023
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
using namespace std;
#include <uFuture.h>

int routine() {
	return 3;											// perform work and return result
}
struct Functor {										// closure: allows arguments to work
	double val;
	double operator()() {								// function-call operator
		return val;										// perfom work and return result
	}
	Functor( double val ) : val( val ) {}
} functor( 4.5 );

void routine2() {
	osacquire( cout ) << -1 << endl;					// perfom work and no result
}
struct Functor2 {										// closure: allows arguments to work
	double val;
	void operator()() {									// function-call operator
		osacquire( cout ) << val << endl;				// perfom work and no result
	}
	Functor2( double val ) : val( val ) {}
} functor2( 7.3 );

int main() {
	enum { NoOfRequests = 10 };
	uExecutor executor;									// work-pool of threads and processors
	Future_ISM<int> fi[NoOfRequests];
	Future_ISM<double> fd[NoOfRequests];
	Future_ISM<char> fc[NoOfRequests];

	for ( int i = 0; i < NoOfRequests; i += 1 ) {		// send off work for executor
		executor.sendrecv( routine, fi[i] );			//  and get return future
		executor.sendrecv( functor, fd[i] );
		executor.sendrecv( []() { return 'a'; }, fc[i] );
	} // for
	executor.send( routine2 );							// send off work but no return value
	executor.send( functor2 );
	executor.send( []() { osacquire( cout ) << 'd' << endl; } );
	for ( int i = 0; i < NoOfRequests; i += 1 ) {		// wait for results
		osacquire( cout ) << fi[i]() << " " << fd[i]() << " " << fc[i]() << " ";
	} // for
	osacquire( cout ) << endl;
}

// Local Variables: //
// tab-width: 4 //
// End: //
