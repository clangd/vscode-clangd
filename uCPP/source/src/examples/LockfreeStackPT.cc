// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2023
// 
// LockfreeStackPT.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Fri Jun  9 11:29:13 2023
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Jun 12 07:49:55 2023
// Update Count     : 5
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
#include <pthread.h>
#include <assert.h>
#include <malloc.h>

template< typename T > static inline __attribute__((always_inline)) bool uCompareAssignValue( volatile T & assn, T & comp, T replace ) {
//	return __atomic_compare_exchange_n( &assn, &comp, replace, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST );
	T temp = comp;
	T old = __sync_val_compare_and_swap( &assn, comp, replace );
	return old == temp ? true : ( comp = old, false );
} // uCompareAssignValue

class Stack {
  public:
	struct Node;										// forward declaration
  private:
	union Link {
		struct {										// 32/64-bit x 2
			Node * volatile top;						// pointer to stack top
			uintptr_t count;							// count each push
		};
		#if defined( __SIZEOF_INT128__ )
		__int128 atom;									// gcc, 128-bit integer
		#else
		int64_t atom;
		#endif // __SIZEOF_INT128__
	} stack;
  public:
	struct Node {
		// resource data
		Link next;										// pointer to next node/count (resource)
	};
	void push( Node & n ) {
		n.next = stack;									// atomic assignment unnecessary
		for ( ;; ) {									// busy wait
			Link temp{ &n, n.next.count + 1 };
		  if ( uCompareAssignValue( stack.atom, n.next.atom, temp.atom ) ) break; // attempt to update top node
		} // for
	}
	Node * pop() {
		Link t = stack;									// atomic assignment unnecessary
		for ( ;; ) {									// busy wait
		  if ( t.top == nullptr ) return nullptr;		// empty stack ?
			Link temp{ t.top->next.top, t.count };
		  if ( uCompareAssignValue( stack.atom, t.atom, temp.atom ) ) return t.top; // attempt to update top node
		} // for
	}
	Stack() { stack.atom = 0; }
};

Stack stack;											// global stack

enum { Times = 2'000'000 };

void * worker( void * ) {
	for ( volatile int i = 0; i < Times; i += 1 ) {
		Stack::Node & n = *stack.pop();
		assert( &n != nullptr );
		n.next.top = nullptr;							// shrub fields
		n.next.count = 0;
		//yield( rand() % 3 );
		stack.push( n );
	}
	return nullptr;
}

int main() {
	enum { N = 8 };

	for ( int i = 0; i < N; i += 1 ) {					// push N values on stack
		stack.push( *new Stack::Node );					// must be 16-byte aligned
	}

	pthread_t workers[N];
	for ( size_t tid = 0; tid < N; tid += 1 ) {
		if ( pthread_create( &workers[tid], NULL, worker, nullptr ) < 0 ) {
			abort();
		} // if
	} // for
	for ( size_t tid = 0; tid < N; tid += 1 ) {
		if ( pthread_join( workers[tid], NULL ) < 0 ) {	// join
			abort();
		} // if
	} // for

	for ( int i = 0; i < N; i += 1 ) {					// pop N nodes from list
		delete stack.pop();
	}
	cout << "successful completion" << endl;
}

// Local Variables: //
// compile-command: "g++-12 -g -O3 -mcx16 (-mno-outline-atomics) LockfreeStackPT.cc -pthread" //
// End: //
