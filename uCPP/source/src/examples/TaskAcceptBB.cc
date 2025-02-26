//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// TaskAcceptBB.cc -- Generic bounded buffer using a task
// 
// Author           : Peter A. Buhr
// Created On       : Sun Sep 15 20:24:44 1991
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Dec 19 23:03:29 2016
// Update Count     : 75
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


template<typename ELEMTYPE> _Task BoundedBuffer {
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

	void insert( ELEMTYPE elem ) {
		Elements[back] = elem;
	} // BoundedBuffer::insert

	ELEMTYPE remove() {
		return Elements[front];
	} // BoundedBuffer::remove
  protected:
	void main() {
		for ( ;; ) {
			_Accept( ~BoundedBuffer )
				break;
			or _When ( count != size ) _Accept( insert ) {
				back = ( back + 1 ) % size;
				count += 1;
			} or _When ( count != 0 ) _Accept( remove ) {
				front = ( front + 1 ) % size;
				count -= 1;
			} // _Accept
		} // for
	} // BoundedBuffer::main
}; // BoundedBuffer

#include "ProdConsDriver.i"

// Local Variables: //
// tab-width: 4 //
// compile-command: "u++ TaskAcceptBB.cc" //
// End: //
