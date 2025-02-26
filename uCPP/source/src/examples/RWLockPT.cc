// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2016
// 
// RWLockPT.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Dec 19 23:01:36 2016
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Jun  9 17:28:06 2023
// Update Count     : 7
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

#include <cstdio>
#include <cstdlib>
#include <pthread.h>

const unsigned int NoOfTimes = 1'000'000;
const unsigned int Work = 100;

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

void * reader( void * ) {
	for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
		pthread_rwlock_rdlock( &rwlock );
		for ( volatile unsigned int b = 0; b < Work; b += 1 );
		pthread_rwlock_unlock( &rwlock );
	} // for
	return nullptr;
} // reader

void *writer( void * ) {
	for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
		for ( volatile unsigned int b = 0; b < Work; b += 1 ); // do work
		pthread_rwlock_wrlock( &rwlock );
		for ( volatile unsigned int b = 0; b < Work; b += 1 ); // do work
		pthread_rwlock_unlock( &rwlock );
		for ( volatile unsigned int b = 0; b < Work; b += 1 ); // do work
	} // for
	return nullptr;
} // writer

int main() {
	enum { Readers = 6, Writers = 2 };

	pthread_t readers[Readers];
	pthread_t writers[Writers];
	int rc_readers[Readers];
	int rc_writers[Writers];

	//int rc_rwlock = pthread_rwlock_init(&rwlock,nullptr);
	//int rc_mutex = pthread_mutex_init(&mutex, nullptr);
	
	// create

	for ( unsigned int i = 0; i < Writers; i += 1 ) {
		if ( rc_writers[i] = pthread_create( &writers[i], nullptr, writer, nullptr ) ) {
			perror( "pthread_create" );
			exit( EXIT_FAILURE );
		} // if
	} // for
	for ( unsigned int i = 0; i < Readers; i += 1 ) {
		if ( rc_readers[i] = pthread_create( &readers[i], nullptr, reader, nullptr ) ) {
			perror( "pthread_create" );
			exit( EXIT_FAILURE );
		} // if
	} // for

	for ( unsigned int i = 0; i < Readers; i += 1 ) {
		if ( pthread_join( readers[i], nullptr ) ) {
			perror( "pthread_join" );
			exit( EXIT_FAILURE );
		} // if
	} // for
	for ( unsigned int i = 0; i < Writers; i += 1 ) {
		if ( pthread_join(writers[i], nullptr) ) {
			perror( "pthread_join" );
			exit( EXIT_FAILURE );
		} // if
	} // for
} // main

// Local Variables: //
// compile-command: "g++ -Wall -Wextra -O3 -g RWLockPT.cc -pthread" //
// End: //
