//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Ashif S. Harji 1997
// 
// TimeoutPthreadsCond.cc -- 
// 
// Author           : Ashif S. Harji
// Created On       : Thu Dec 11 10:17:16 1997
// Last Modified By : Peter A. Buhr
// Last Modified On : Wed Apr 20 23:01:33 2022
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
#include <pthread.h>
#include <iostream>
using std::cout;
using std::osacquire;
using std::endl;

pthread_mutex_t mutex;
pthread_cond_t waitc;
uBarrier b( 2 );

const unsigned int NoOfTimes = 20;

void *r1main( void * ) {
	struct timeval now;
	struct timespec timeout;
	
	pthread_mutex_lock( &mutex );

	gettimeofday( &now, 0 );
	timeout.tv_sec = now.tv_sec + 1;
	timeout.tv_nsec = now.tv_usec * 1000;
	if ( pthread_cond_timedwait( &waitc, &mutex, &timeout ) != ETIMEDOUT ) {
		abort( "timeout failed" );
	} // if
	osacquire( cout ) << &uThisTask() << " timedout" << endl;

	b.block();

	// Test calls which occur increasingly close to timeout value.

	for ( unsigned int i = 0; i < NoOfTimes + 3; i += 1 ) {
		gettimeofday( &now, 0 );
		timeout.tv_sec = now.tv_sec + 1;
		timeout.tv_nsec = now.tv_usec * 1000;
		int rc;

		rc = pthread_cond_timedwait( &waitc, &mutex, &timeout );
		if ( rc == 0 ) {
			osacquire( cout ) << &uThisTask() << " signalled" << endl;
		} else if ( rc ==  ETIMEDOUT ) { 
			osacquire( cout ) << &uThisTask() << " timedout" << endl;
		} else {
			abort( "timeout invalid\n" );
		} // if

		b.block();
	} // for

	return 0;
} // r1main

void *r2main( void * ) {
	// Test if timing out works.

	b.block();

	// Test calls which occur increasingly close to timeout value.

	_Timeout( uDuration( 0, 100000000 ) );
	pthread_cond_signal( &waitc );
	b.block();

	_Timeout( uDuration( 0, 500000000 ) );
	pthread_cond_signal( &waitc );
	b.block();

	_Timeout( uDuration( 0, 900000000 ) );
	pthread_cond_signal( &waitc );
	b.block();

	for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
		_Timeout( uDuration( 0, 999950000 ) );
		pthread_cond_signal( &waitc );
		b.block();
	} // for

	return 0;
} // r2main

int main(){
	uProcessor processor[1] __attribute__(( unused ));	// more than one processor
	pthread_t r1, r2;

	pthread_mutex_init( &mutex, nullptr );
	pthread_cond_init( &waitc, nullptr );
		
	if ( pthread_create( &r1, nullptr, r1main, nullptr ) != 0 ) {
		cout << "create thread r1 failure" << endl;
		exit( EXIT_FAILURE );
	}

	if ( pthread_create( &r2, nullptr, r2main, nullptr ) != 0 ) {
		cout << "create thread r2 failure" << endl;
		exit( EXIT_FAILURE );
	}

	pthread_join( r1, nullptr );
	pthread_join( r2, nullptr );
} // main


// Local Variables: //
// compile-command: "u++ TimeoutPthreadsCond.cc" //
// End: //
