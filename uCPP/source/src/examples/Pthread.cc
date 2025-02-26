//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2002
// 
// PThread.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Thu Jan 17 17:06:03 2002
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Apr 25 22:37:50 2022
// Update Count     : 193
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

#if defined( __U_CPLUSPLUS__ )
#endif // __U_CPLUSPLUS__

#include <iostream>
using std::cout;
using std::endl;
#if defined( __U_CPLUSPLUS__ )
#define cout osacquire( cout )
using std::osacquire;
#endif // __U_CPLUSPLUS__
#include <pthread.h>
#include <limits.h>										// access: PTHREAD_KEYS_MAX
#include <cerrno>
#include <cassert>
#include <stdlib.h>										// prototype: rand


class MutexMem {
	pthread_mutex_t &mutex;
  public:
	MutexMem( pthread_mutex_t &mutex ) : mutex( mutex ) {
		pthread_mutex_lock( &mutex );
	}
	~MutexMem() {
		pthread_mutex_unlock( &mutex );
	}
};

template <class ELEMTYPE> class BoundedBuffer {
	pthread_mutex_t mutex;
	pthread_cond_t Full, Empty;							// waiting consumers & producers
	int front, back, count;
	ELEMTYPE Elements[20];
  public:
	BoundedBuffer() {
		front = back = count = 0;
		pthread_mutex_init( &mutex, nullptr );
		pthread_cond_init( &Full, nullptr );
		pthread_cond_init( &Empty, nullptr );
	}

	~BoundedBuffer() {
		pthread_mutex_lock( &mutex );
		pthread_cond_destroy( &Empty );
		pthread_cond_destroy( &Full );
		pthread_mutex_destroy( &mutex );
	}

	int query() { return count; }

	void insert( ELEMTYPE elem ) {
		MutexMem lock( mutex );
		while ( count == 20 ) pthread_cond_wait( &Empty, &mutex ); // block producer
		Elements[back] = elem;
		back = ( back + 1 ) % 20;
		count += 1;
		pthread_cond_signal( &Full );					// unblock consumer
	}
	ELEMTYPE remove() {
		MutexMem lock( mutex );
		while ( count == 0 ) pthread_cond_wait( &Full, &mutex ); // block consumer
		ELEMTYPE elem = Elements[front];
		front = ( front + 1 ) % 20;
		count -= 1;
		pthread_cond_signal( &Empty );					// unblock producer
		return elem;
	}
};

void *producer( void *arg ) {
	BoundedBuffer<int> &buf = *(BoundedBuffer<int> *)arg;
	const int NoOfItems = rand() % 40;
	int item;

	for ( int i = 1; i <= NoOfItems; i += 1 ) {			// produce a bunch of items
		item = rand() % 100 + 1;						// produce a random number
		cout << "Producer:" << pthread_self() << " value:" << item << endl;
		buf.insert( item );								// insert element into queue
	} // for
	cout << "Producer:" << pthread_self() << " is finished" << endl;
	return (void *)1;
} // producer

void *consumer( void *arg ) {
	BoundedBuffer<int> &buf = *(BoundedBuffer<int> *)arg;
	int item;

	for ( ;; ) {										// consume until a negative element appears
		item = buf.remove();							// remove from front of queue
		cout << "Consumer:" << pthread_self() << " value:" << item << endl;
	  if ( item == -1 ) break;
	} // for
	cout << "Consumer:" << pthread_self() << " is finished" << endl;
	return (void *)0;
} // consumer


void *Worker1( void * ) {
#if defined( __U_CPLUSPLUS__ )
	int stacksize = uThisTask().stackSize();
	cout << "Worker1, stacksize:" << stacksize << endl;
#endif // __U_CPLUSPLUS__
	for ( int i = 0; i < 100000; i += 1 ) {}
	cout << "Worker1, ending" << endl;
	return (void *)0;
} // Worker1

void *Worker2( void * ) {
	pthread_key_t key[PTHREAD_KEYS_MAX];
	unsigned long int i;
	for ( i = 0; i < 60; i += 1 ) {
		if ( pthread_key_create( &key[i], nullptr ) != 0 ) {
			cout << "Create key" << endl;
			exit( EXIT_FAILURE );
		} // if
		cout << pthread_self() << ", i:" << i << " key:" << key[i] << endl;
	} // for
	for ( i = 0; i < 60; i += 1 ) {
		if ( pthread_setspecific( key[i], (const void *)i ) != 0 ) {
			cout << "Set key" << endl;
			exit( EXIT_FAILURE );
		} // if
	} // for
	for ( i = 0; i < 60; i += 1 ) {
		if ( (unsigned long int)pthread_getspecific( key[i] ) != i ) {
			cout << "Get key" << endl;
			exit( EXIT_FAILURE );
		} // if
	} // for
	pthread_exit( nullptr );
	return (void *)0;
} // Worker2


int check = 1;

void clean1( void *) {
	cout << "Executing top cleanup" << endl;
	check /= 2;
}

void clean2( void *) {
	cout << "Executing bottom cleanup" << endl;
	check -= 1000;
}

class RAII {
  public:
	RAII() {
		check += 1;
//		cout << "Executing RAII constructor" << endl;
	}
	~RAII() {
		check -= 1;
//		cout << "Executing RAII destructor" << endl;
	}
}; // RAII


// seperate the cleanup test into two functions to test proper cleanup nesting across function call boundaries

void cancel_me() {
	try {
		RAII r;
		pthread_cleanup_push( clean2, nullptr );
		check += 1000;
		pthread_cleanup_push( clean1, nullptr );			
		check *= 2;
		pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, nullptr ); 
		pthread_testcancel();							// wait to be cancelled
		pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, nullptr );
		pthread_cleanup_pop( 0 );
		check /= 2;
		pthread_cleanup_pop( 0 );
		check -= 1000;
	} catch (...) {
		cout << "being cancelled" << endl;
#if ! defined( __U_CPLUSPLUS__ )
		throw;											// required for g++, implied for uC++
#endif // ! __U_CPLUSPLUS__
	} // try
} // cancel_me

void *cancellee( void * ) {
	int i;
	pthread_setcanceltype( PTHREAD_CANCEL_DEFERRED, nullptr );
	pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, nullptr ); // guard against undefined cancellation points
	for ( ;; ) {
		pthread_cleanup_push( clean2, nullptr );
		check += 1000;
		pthread_cleanup_push( clean1, nullptr );
		check *= 2;
		{
			RAII rai;				
			for ( i = 0; i < 10; i += 1 ) {				
				cancel_me();							// if we return normally or get cancelled, the top
														// handler MUST execute before any cleanup in this frame
			} // for
		}
		pthread_cleanup_pop( 0 );
		check /= 2;
		pthread_cleanup_pop( 0 );
		check -= 1000;
	} // for
	abort();											// CONTROL NEVER REACHES HERE!
} // cancellee

void *canceller( void *arg ) {
	pthread_t cancellee = *(pthread_t *)arg;
	cout << pthread_self() << " before cancel" << endl;
	pthread_cancel( cancellee );
	pthread_join( cancellee, nullptr );
	cout << pthread_self() << " after cancel" << endl;
	cout << "check value: " << check << endl;
	if ( check != 1 ) {
		cout << "not all destructors called" << endl;
		exit( EXIT_FAILURE );
	} // if
	return nullptr;
} // canceller


void maintestcancel() {
	pthread_setcanceltype( PTHREAD_CANCEL_DEFERRED, nullptr );
	pthread_cleanup_push( clean2, nullptr );
	check += 1000;	
	{
		RAII rai;				
		pthread_cleanup_push( clean1, nullptr );			
		check *= 2;
		pthread_testcancel();							// check to be cancelled (not gonna happen)		
		pthread_cleanup_pop( 1 );
	}
	pthread_cleanup_pop( 1 );
	assert( check == 1 );
} // maintestcancel

void bye( void * ) {
	cout << "successful completion" << endl;
} // bye

#if defined( __U_CPLUSPLUS__ )
_Task JoinTester : public uPthreadable {
	void main() {
		joinval = this;
	} // JoinTester::main
}; // JoinTester
#endif // __U_CPLUSPLUS__

int main() {
	const int NoOfCons = 20, NoOfProds = 30;
	BoundedBuffer<int> buf;								// create a buffer monitor
	pthread_t cons[NoOfCons];							// pointer to an array of consumers
	pthread_t prods[NoOfProds];							// pointer to an array of producers

	// parallelism

	pthread_setconcurrency( 4 );

	// create/join and mutex/condition test

	cout << "create/join and mutex/condition test" << endl << endl;

	for ( int i = 0; i < NoOfCons; i += 1 ) {			// create consumers
		if ( pthread_create( &cons[i], nullptr, consumer, &buf ) != 0 ) {
			cout << "create thread failure, errno:" << errno << endl;
			exit( EXIT_FAILURE );
		} // if
	} // for
	for ( int i = 0; i < NoOfProds; i += 1 ) {			// 	create producers
		if ( pthread_create( &prods[i], nullptr, producer, &buf ) != 0 ) {
			cout << "create thread failure" << endl;
			exit( EXIT_FAILURE );
		} // if
	} // for

	void *result;
	for ( int i = 0; i < NoOfProds; i += 1 ) {			// wait for producers to end
		if ( pthread_join( prods[i], &result ) != 0 ) {
			cout << "join thread failure" << endl;
			exit( EXIT_FAILURE );
		} // if
		if ( result != (void *)1 ) {
			cout << "bad return value" << endl;
			exit( EXIT_FAILURE );
		} // if
		cout << "join prods[" << i << "]:" << prods[i] << " result:" << result << endl;
	} // for

	for ( int i = 0; i < NoOfCons; i += 1 ) {			// terminate each consumer
		buf.insert( -1 );
	} // for

	for ( int i = 0; i < NoOfCons; i += 1 ) {			// wait for consumer to end
		if ( pthread_join( cons[i], &result ) != 0 ) {
			cout << "join thread failure" << endl;
			exit( EXIT_FAILURE );
		} // if
		if ( result != (void *)0 ) {
			cout << "bad return value" << endl;
			exit( EXIT_FAILURE );
		} // if
		cout << "join cons[" << i << "]:" << cons[i] << " result:" << result << endl;
	} // for

	// thread attribute test

	cout << endl << endl << "thread attribute test" << endl << endl;

	pthread_attr_t attr;
	pthread_attr_init( &attr );
	pthread_attr_setstacksize( &attr, 64 * 1000 );
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
	size_t stacksize;
	pthread_attr_getstacksize( &attr, &stacksize );

	pthread_t worker1, worker2;

	if ( pthread_create( &worker1, &attr, Worker1, nullptr ) != 0 ) {
		cout << "create thread failure" << endl;
		exit( EXIT_FAILURE );
	} // if

	pthread_attr_destroy( &attr );

	// thread specific data test

	cout << endl << endl << "thread specific data test" << endl << endl;

	if ( pthread_create( &worker1, nullptr, Worker2, nullptr ) != 0 ) {
		cout << "create thread failure" << endl;
		exit( EXIT_FAILURE );
	} // if
	if ( pthread_create( &worker2, nullptr, Worker2, nullptr ) != 0 ) {
		cout << "create thread failure" << endl;
		exit( EXIT_FAILURE );
	} // if
	if ( pthread_join( worker1, nullptr ) != 0 ) {
		cout << "join thread failure" << endl;
		exit( EXIT_FAILURE );
	} // if
	if ( pthread_join( worker2, nullptr ) != 0 ) {
		cout << "join thread failure" << endl;
		exit( EXIT_FAILURE );
	} // if

	// thread cancel test: check uMain's pthread emulation

	maintestcancel();

#if defined( __U_CPLUSPLUS__ )
	{
		cout << "uPthreadable join test" << endl;
		JoinTester f;
		void *res;
		int retcode = pthread_join( f.pthreadId(), &res );
		if ( retcode != 0 ) abort( "uPthreadable join test failed with return code %d", retcode );
		if ( &f != res ) abort( "uPthreadable join test failed" );
	}
#endif // __U_CPLUSPLUS__

#if defined( __U_BROKEN_CANCEL__ )
	cout << endl << endl << "cancellation test skipped -- pthread cancellation not supported on this system" << endl << endl;
	cout << "successful completion" << endl;
#else
	cout << endl << endl << "thread cancellation test" << endl << endl;

	if ( pthread_create( &worker1, nullptr, cancellee, nullptr ) != 0 ) {
		cout << "create thread failure" << endl;
		exit( EXIT_FAILURE );
	} // if
	cout << "cancellee " << worker1 << endl;
	if ( pthread_create( &worker2, nullptr, canceller, &worker1 ) != 0 ) {
		cout << "create thread failure" << endl;
		exit( EXIT_FAILURE );
	} // if
	// worker1 joined in canceller
	if ( pthread_join( worker2, nullptr ) != 0 ) {
		cout << "join thread failure" << endl;
		exit( EXIT_FAILURE );
	} // if
	
	// now cancel yourself
	
	pthread_cleanup_push( bye, nullptr );
	pthread_cancel( pthread_self() );
	pthread_testcancel();
	pthread_cleanup_pop( 0 );
	
	cout << "main cancellation failure" << endl;

	abort();
#endif // __U_BROKEN_CANCEL__
} // main


// Local Variables: //
// compile-command: "g++ -g Pthread.cc -lpthread" //
// tab-width: 4 //
// End: //
