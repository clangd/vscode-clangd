//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard Bilson 2003
// 
// AbortExit.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Fri Dec 19 10:33:58 2003
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Feb 25 06:58:13 2022
// Update Count     : 20
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

#include <uSemaphore.h>
#include <iostream>
#include <unistd.h>

using namespace std;

enum Mode { SPIN, EXIT, UABORT, EXPLODE, ABORT, ASSERT, RETURN, PTHREAD_RETURN } mode;

void *spinner( void * ) {
	for ( ;; );
} // spinner

_Task worker {
	void main() {
		switch( mode ) {
		  case SPIN:
			for ( ;; ) {}
		  case EXIT:
			exit( EXIT );
		  case UABORT:
			abort( "worker %d %s", UABORT, "text" );
		  case EXPLODE:
			kill( getpid(), SIGKILL );
			for ( ;; ) {}				// delay until signal delivered to some kernel thread
		  case ABORT:
			abort();
		  case ASSERT:
			assert( false );
		  default: ;
		} // switch
		// CONTROL NEVER REACHES HERE!
	} // main
  public:
	worker( uCluster &c ) : uBaseTask( c ) {}
}; // worker

int main( int argc, char *argv[] ) {
	if ( argc <= 1 ) {
		cerr << "usage: " << argv[0] << " [0-7]" << endl;
		exit( EXIT_FAILURE );
	} // if
	mode = (Mode)atoi( argv[1] );

	switch (mode) {
	  case RETURN:
		uRetCode = RETURN;
		return;
	  case PTHREAD_RETURN:
		uRetCode = PTHREAD_RETURN;
		pthread_t pt;
		pthread_create( &pt, nullptr, spinner, nullptr );
		return;
	  default:
		uCluster cluster;
		uProcessor processor( cluster );
		worker t( cluster );
		uSemaphore s( 0 );
		s.P();
	} // switch
} // main
