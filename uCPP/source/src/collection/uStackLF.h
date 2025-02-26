//                               -*- Mode: C -*- 
// 
// Copyright (C) Peter A. Buhr 2017
// 
// uStackLF.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue Apr  4 22:46:32 2017
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Nov 16 15:05:55 2021
// Update Count     : 110
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


#pragma once


template<typename T> class StackLF {
  public:
	union Link {
		struct {										// 32/64-bit x 2
			T * volatile top;							// pointer to stack top
			uintptr_t count;							// count each push
		};
		#if __SIZEOF_INT128__ == 16
		__int128										// gcc, 128-bit integer
		#else
		uint64_t										// 64-bit integer
		#endif // __SIZEOF_INT128__ == 16
		atom;
	};
  private:
	Link stack;
  public:
	StackLF() { stack.atom = 0; }

	T * top() { return stack.top; }

	void push( T & n ) {
		*n.getNext() = stack;							// atomic assignment unnecessary, or use CAA
		for ( ;; ) {									// busy wait
			// if ( __atomic_compare_exchange_n( &stack.atom, &n.getNext()->atom, (Link){ {&n, n.getNext()->count + 1} }.atom, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST ) ) break; // attempt to update top node
			if ( uCompareAssignValue( stack.atom, n.getNext()->atom, (Link){ {&n, n.getNext()->count + 1} }.atom ) ) break; // attempt to update top node
			#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::spins, 1 );
			#endif // __U_STATISTICS__
		} // for
	} // StackLF::push

	T * pop() {
		Link t = stack;									// atomic assignment unnecessary, or use CAA
		for ( ;; ) {									// busy wait
			if ( t.top == nullptr ) return nullptr;		// empty stack ?
			// if ( __atomic_compare_exchange_n( &stack.atom, &t.atom, (Link){ {t.top->getNext()->top, t.count} }.atom, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST ) ) return t.top; // attempt to update top node
			if ( uCompareAssignValue( stack.atom, t.atom, (Link){ {t.top->getNext()->top, t.count} }.atom ) ) return t.top; // attempt to update top node
			#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::spins, 1 );
			#endif // __U_STATISTICS__
		} // for
	} // StackLF::pop
}; // StackLF


// Local Variables: //
// compile-command: "make install" //
// End: //
