//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uFlexArray.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Wed Nov 25 07:50:19 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Nov 16 14:59:52 2021
// Update Count     : 68
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


template<typename T> class uFlexArray {
	int NoOfElems, InitMaxNoOfElems, MaxNoOfElems;
	T * elems;
	void init( int max ) {
		if ( max < 1 ) max = 1;							// pathological case
		MaxNoOfElems = InitMaxNoOfElems = max;
		elems = new T[MaxNoOfElems];
	} // uFlexArray::init
  public:
	uFlexArray( const int max = 20 ) {
		init( max );
		NoOfElems = 0;
	} // uFlexArray::uFlexArray

	uFlexArray( const uFlexArray & rhs ) {
		init( rhs.MaxNoOfElems );
		memcpy( elems, rhs.elems, sizeof(T) * rhs.NoOfElems ); // copy old data to new array
		NoOfElems = rhs.NoOfElems;
	} // uFlexArray::uFlexArray

	~uFlexArray() {
		delete [] elems;
	} // uFlexArray::uFlexArray
	
	uFlexArray & operator=( const uFlexArray & rhs ) {
		if ( this != &rhs ) {				// x = x ?
			delete [] elems;
			init( rhs.MaxNoOfElems );
			memcpy( elems, rhs.elems, sizeof(T) * rhs.NoOfElems ); // copy old data to new array
			NoOfElems = rhs.NoOfElems;
		} // if
		return *this;
	} // uFlexArray::operator=
	
	int size() {
		return NoOfElems;
	} // uFlexArray::size

	const T & operator[]( int pos ) const {
		return const_cast<uFlexArray*>(this)->operator[]( pos );
	} // uFlexArray::operator[]

	T & operator[]( int pos ) {
		if ( 0 <= pos && pos < NoOfElems ) {			// in range ?
			return elems[pos];
		} else {
			abort( "uFlexArray::[] : Attempt to subscript an element at position %d in an array of size 0 to %d.", pos, NoOfElems - 1 );
		} // if
	} // uFlexArray::operator[]
	
	void reserve( int size ) {
		if ( size > MaxNoOfElems ) {
			int tempMaxNoOfElems = size + MaxNoOfElems;
			T * temp = new T[tempMaxNoOfElems];
			memcpy( temp, elems, sizeof(T) * MaxNoOfElems ); // copy old data to new array
			delete [] elems;
			MaxNoOfElems = tempMaxNoOfElems;
			elems = temp;
		} // if
		NoOfElems = size;
	} // uFlexArray::reserve

	void add( T elem ) {
		reserve( NoOfElems + 1 );
		elems[NoOfElems - 1] = elem;
	} // uFlexArray::add
	
	void remove( int pos ) {
		if ( 0 <= pos && pos < NoOfElems ) {			// in range ?
			for ( int i = pos; i < NoOfElems - 1; i += 1 ) { // shuffle values down
				elems[i] = elems[i + 1];
			} // for
			NoOfElems -= 1;
		} else {
			abort( "uFlexArray::remove : Attempt to remove an element at position %d in an array of size 0 to %d.", pos, NoOfElems - 1 );
		} // if
	} // uFlexArray::remove

	void clear() {
		if ( MaxNoOfElems > InitMaxNoOfElems * 3 ) {	// reduce size ?
			delete [] elems;
			init( InitMaxNoOfElems );
		} // if 
		NoOfElems = 0;
	} // uFlexArray::clear
}; // uFlexArray


// Local Variables: //
// compile-command: "make install" //
// End: //
