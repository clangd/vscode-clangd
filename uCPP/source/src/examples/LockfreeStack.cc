// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2023
// 
// LockfreeStack.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue May 30 18:33:40 2023
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Jun 19 08:31:12 2023
// Update Count     : 28
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

_Task Worker {
	void main() {
		for ( volatile int i = 0; i < Times; i += 1 ) {
			Stack::Node & n = *stack.pop();
			assert( (void *)&n != nullptr );			// force check
			n.next.top = nullptr;						// shrub fields
			n.next.count = 0;
			// yield( rand() % 3 );
			stack.push( n );
		}
	}
};

int main() {
	enum { N = 8 };
	uProcessor p[N - 1];								// kernel threads

	for ( int i = 0; i < N; i += 1 ) {					// push N values on stack
		stack.push( *new Stack::Node );					// must be 16-byte aligned
	}
	{
		Worker workers[N];								// run test
	}
	for ( int i = 0; i < N; i += 1 ) {					// pop N nodes from list
		delete stack.pop();
	}
	cout << "successful completion" << endl;
}

// Local Variables: //
// compile-command: "u++-work -g -multi -O3 -nodebug -DNDEBUG LockfreeStack.cc" //
// End: //
