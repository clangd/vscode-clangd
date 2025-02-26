//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2015
// 
// MutexCondBB.cc -- Generic bounded buffer problem using a mutex lock and condition variables
// 
// Author           : Peter A. Buhr
// Created On       : Sun May  3 23:11:32 2015
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Dec 19 22:26:43 2016
// Update Count     : 8
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

template<typename ELEMTYPE> class BoundedBuffer {
	const int size;										// number of buffer elements
	int front, back;									// position of front and back of queue
	int count;											// number of used elements in the queue
	uOwnerLock mutex;
	uCondLock BufFull, BufEmpty;
	ELEMTYPE *Elements;
  public:
	BoundedBuffer( const unsigned int size = 10 ) : size( size ) {
		front = back = count = 0;
		Elements = new ELEMTYPE[size];
	} // BoundedBuffer::BoundedBuffer

	~BoundedBuffer() {
		delete [] Elements;
	} // BoundedBuffer::~BoundedBuffer

	int query() {
		return count;
	} // BoundedBuffer::query

	void insert( ELEMTYPE elem ) {
		mutex.acquire();
		while ( count == size ) BufFull.wait( mutex );

		assert( count < size );
		Elements[back] = elem;
		back = ( back + 1 ) % size;
		count += 1;

		BufEmpty.signal();
		mutex.release();
	}; // BoundedBuffer::insert

	ELEMTYPE remove() {
		mutex.acquire();
		while ( count == 0 ) BufEmpty.wait( mutex );

		assert( count > 0 );
		ELEMTYPE elem = Elements[front];
		front = ( front + 1 ) % size;
		count -= 1;

		BufFull.signal();
		mutex.release();
		return elem;
	} // BoundedBuffer::remove
}; // BoundedBuffer

#include "ProdConsDriver.i"

// Local Variables: //
// tab-width: 4 //
// compile-command: "u++ MutexCondBB.cc" //
// End: //
