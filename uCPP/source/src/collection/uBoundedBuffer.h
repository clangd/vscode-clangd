//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1993
// 
// uBoundedBuffer.h -- Generic bounded buffer problem using a monitor and uAccept
// 
// Author           : Peter A. Buhr
// Created On       : Sun Apr  4 10:20:32 1993
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr  5 08:00:38 2022
// Update Count     : 21
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


template<typename ElemType> _Monitor uBoundedBuffer {
  protected:
	const int size;										// number of buffer elements
	int front, back;									// position of front and back of queue
	int count;											// number of used elements in the queue
	ElemType * elements;
  public:
	uBoundedBuffer( const int size = 10 ) : size( size ) {
		front = back = count = 0;
		elements = new ElemType[size];
	} // uBoundedBuffer::uBoundedBuffer

	~uBoundedBuffer() {
		delete [] elements;
	} // uBoundedBuffer::~uBoundedBuffer

	_Nomutex int query() {
		return count;
	} // uBoundedBuffer::query

	void insert( ElemType elem );
	ElemType remove();
}; // uBoundedBuffer

template<typename ElemType> inline void uBoundedBuffer<ElemType>::insert( ElemType elem ) {
	if ( count == size ) {								// buffer full ?
		_Accept( remove );								// only allow removals
	} // if

	elements[back] = elem;
	back = ( back + 1 ) % size;
	count += 1;
} // uBoundedBuffer::insert

template<typename ElemType> inline ElemType uBoundedBuffer<ElemType>::remove() {
	ElemType elem;

	if ( count == 0 ) {									// buffer empty ?
		_Accept( insert );								// only allow insertions
	} // if

	elem = elements[front];
	front = ( front + 1 ) % size;
	count -= 1;

	return elem;
} // uBoundedBuffer::remove


// Local Variables: //
// compile-command: "make install" //
// End: //
