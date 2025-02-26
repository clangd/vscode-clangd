//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// MonAcceptBB.cc -- Generic bounded buffer problem using a monitor and uAccept
// 
// Author           : Peter A. Buhr
// Created On       : Thu Aug  2 11:35:05 1990
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Dec 19 08:52:13 2016
// Update Count     : 125
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


template<typename ELEMTYPE> _Monitor BoundedBuffer {
	const int size;										// number of buffer elements
	int front, back;									// position of front and back of queue
	int count;											// number of used elements in the queue
	ELEMTYPE *Elements;
  public:
	BoundedBuffer( const int size = 10 ) : size( size ) {
		front = back = count = 0;
		Elements = new ELEMTYPE[size];
	} // BoundedBuffer::BoundedBuffer

	~BoundedBuffer() {
		delete [] Elements;
	} // BoundedBuffer::~BoundedBuffer

	_Nomutex int query() {
		return count;
	} // BoundedBuffer::query

	void insert( ELEMTYPE elem );
	ELEMTYPE remove();
}; // BoundedBuffer

template<typename ELEMTYPE> inline void BoundedBuffer<ELEMTYPE>::insert( ELEMTYPE elem ) {
	if ( count == size ) {								// buffer full ?
		_Accept( remove );								// only allow removals
	} // if

	Elements[back] = elem;
	back = ( back + 1 ) % size;
	count += 1;
} // BoundedBuffer::insert

template<typename ELEMTYPE> inline ELEMTYPE BoundedBuffer<ELEMTYPE>::remove() {
	ELEMTYPE elem;

	if ( count == 0 ) {									// buffer empty ?
		_Accept( insert );								// only allow insertions
	} // if

	elem = Elements[front];
	front = ( front + 1 ) % size;
	count -= 1;

	return elem;
} // BoundedBuffer::remove

#include "ProdConsDriver.i"

// Local Variables: //
// tab-width: 4 //
// compile-command: "u++ MonAcceptBB.cc" //
// End: //
