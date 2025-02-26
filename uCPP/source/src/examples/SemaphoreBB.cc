//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// SemaphoreBB.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Thu Aug 15 16:42:42 1991
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Nov 12 13:33:14 2020
// Update Count     : 60
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

template<typename ELEMTYPE> class BoundedBuffer {
	const int size;										// number of buffer elements
	int front, back;									// position of front and back of queue
	uSemaphore full, empty;								// synchronize for full and empty BoundedBuffer
	uSemaphore ilock, rlock;							// insertion and removal locks
	ELEMTYPE *Elements;
  public:
	BoundedBuffer( const BoundedBuffer & ) = delete;	// no copy
	BoundedBuffer( BoundedBuffer && ) = delete;
	BoundedBuffer & operator=( const BoundedBuffer & ) = delete; // no assignment
	BoundedBuffer & operator=( BoundedBuffer && ) = delete;

	BoundedBuffer( const unsigned int size = 10 ) : size( size ), full( 0 ), empty( size ) {
		front = back = 0;
		Elements = new ELEMTYPE[size];
	} // BoundedBuffer::BoundedBuffer

	~BoundedBuffer() {
		delete  Elements;
	} // BoundedBuffer::~BoundedBuffer

	void insert( ELEMTYPE elem ) {
		empty.P();										// wait if queue is full

		ilock.P();										// serialize insertion
		Elements[back] = elem;
		back = ( back + 1 ) % size;
		ilock.V();

		full.V();										// signal a full queue space
	} // BoundedBuffer::insert

	ELEMTYPE remove() {
		ELEMTYPE elem;
		
		full.P();										// wait if queue is empty

		rlock.P();										// serialize removal
		elem = Elements[front];
		front = ( front + 1 ) % size;
		rlock.V();

		empty.V();										// signal empty queue space
		return elem;
	} // BoundedBuffer::remove
}; // BoundedBuffer

#include "ProdConsDriver.i"

// Local Variables: //
// tab-width: 4 //
// compile-command: "u++ SemaphoreBB.cc" //
// End: //
