//                              -*- Mode: C++ -*- 
// 
// Copyright (C) Glen Ditchfield 1994
// 
// uStack.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sun Feb 13 19:35:33 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Dec 27 21:28:56 2020
// Update Count     : 77
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


#include "uCollection.h"

// A uStack<T> is a uCollection<T> defining the ordering that nodes are returned by drop() in the reverse order from
// those added by add(). T must be a public descendant of uColable.

// The implementation is a typical singly-linked list, except the next field of the last element points to itself
// instead of being null.

template<typename T> class uStack: public uCollection<T> {
  protected:
	using uCollection<T>::root;
  public:
	uStack( const uStack & ) = delete;					// no copy
	uStack( uStack && ) = delete;
	uStack & operator=( const uStack & ) = delete;		// no assignment
	uStack & operator=( uStack && ) = delete;

	using uCollection<T>::head;
	using uCollection<T>::uNext;

	uStack() : uCollection<T>() {}						// post: isEmpty()

	T * top() const {
		return head();
	}

	T * addHead( T * n ) {
		#ifdef __U_DEBUG__
		if ( n->listed() ) abort( "(uStack &)%p.addHead( %p ) node is already on another list.", this, n );
		#endif // __U_DEBUG__
		uNext(n) = root ? root : n;
		root = n;
		return n;
	}

	T * add( T * n ) {
		return addHead( n );
	}

	T * push( T * n ) {
		addHead( n );
		return n;
	}

	T * drop() {
		T * t = root;
		if (root) {
			root = ( T *)uNext(root);
			if (root == t) root = 0;					// only one element ?
			uNext(t) = 0;
		} // if
		return t;
	}

	T * pop() {
		return drop();
	}
};

// A uStackIter<T> is a subclass of uColIter<T> that generates the elements of a uStack<T>.  It returns the elements in
// the order returned by drop().

template<typename T> class uStackIter : public uColIter<T> {
  protected:
	using uColIter<T>::curr;
	using uColIter<T>::uNext;
  public:
	uStackIter() : uColIter<T>() {}						// post: elts = null.

	// create an iterator active in stack s
	uStackIter( const uStack<T> & s ) {
		curr = s.head();
	}

	uStackIter( const uStack<T> & s, T * start ) {
		curr = start;
	}

	// make existing iterator active in stack s
	void over( const uStack<T> & s ) {					// post: elts = {e in s}.
		curr = s.head();
	}

	bool operator>>( T *& tp ) {
		if (curr) {
			tp = curr;
			T * n = (T *)uNext(curr);
			curr = (n == curr) ? nullptr : n;
		} else tp = nullptr;
		return tp != nullptr;
	}
};


// Local Variables: //
// compile-command: "make install" //
// End: //
