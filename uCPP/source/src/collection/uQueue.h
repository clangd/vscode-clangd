//                              -*- Mode: C++ -*- 
// 
// Copyright (C) Glen Ditchfield 1994
// 
// uQueue.h -- 
// 
// Author           : Glen Ditchfield
// Created On       : Sun Feb 13 17:35:59 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Dec 27 21:28:38 2020
// Update Count     : 126
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

// A uQueue<T> is a uCollection<T> defining the ordering that nodes are returned by drop() in the same order from those
// added by add(). T must be a public descendant of uColable.

// The implementation is a typical singly-linked list, except the next field of the last element points to itself
// instead of being null.

template<typename T> class uQueue : public uCollection<T> {
  protected:
	using uCollection<T>::root;

	T * last;											// last element, or nullptr if queue is empty.
  public:
	uQueue( const uQueue & ) = delete;					// no copy
	uQueue( uQueue && ) = delete;
	uQueue & operator=( const uQueue & ) = delete;		// no assignment
	uQueue & operator=( uQueue && ) = delete;

	using uCollection<T>::empty;
	using uCollection<T>::head;
	using uCollection<T>::uNext;

	uQueue() {											// post: isEmpty()
		last = nullptr;
	}

	T * tail() const {
		return last;
	}

	T * succ( T * n ) const {							// pre: *n in *this
		#ifdef __U_DEBUG__
		if ( ! n->listed() ) abort( "(uQueue &)%p.succ( %p ) : Node is not on a list.", this, n );
		#endif // __U_DEBUG__
		return (uNext(n) == n) ? nullptr : (T *)uNext(n);
	} // post: n == tail() & succ(n) == nullptr | n != tail() & *succ(n) in *this

	T * addHead( T * n ) {
		#ifdef __U_DEBUG__
		if ( n->listed() ) abort( "(uQueue &)%p.addHead( %p ) : Node is already on another list.", this, n );
		#endif // __U_DEBUG__
		if (last) {
			uNext(n) = root;
			root = n;
		} else {
			last = root = n;
			uNext(n) = n;								// last node points to itself
		}
		return n;
	}

	T * addTail( T * n ) {
		#ifdef __U_DEBUG__
		if ( n->listed() ) abort( "(uQueue &)%p.addTail( %p ) : Node is already on another list.", this, n );
		#endif // __U_DEBUG__
		if (last) uNext(last) = n;
		else root = n;
		last = n;
		uNext(n) = n;									// last node points to itself
		return n;
	}

	T * add( T * n ) {
		return addTail( n );
	}

	T * dropHead() {
		T * t = head();
		if (root) {
			root = (T *)uNext(root);
			if (root == t) {
				root = last = nullptr;					// only one element
			}
			uNext(t) = nullptr;
		}
		return t;
	}

	T * drop() {
		return dropHead();
	}

	T * remove( T * n ) {								// O(n)
		#ifdef __U_DEBUG__
		if ( ! n->listed() ) abort( "(uQueue &)%p.remove( %p ) : Node is not on a list.", this, n );
		#endif // __U_DEBUG__
		T * prev = nullptr;
		T * curr = root;
		for ( ;; ) {
			if (n == curr) {							// found => remove
				if (root == n) {
					dropHead();
				} else if (last == n) {
					last = prev;
					uNext(last) = last;
				} else {
					uNext(prev) = uNext(curr);
				}
				uNext(n) = nullptr;
				break;
			}
			#ifdef __U_DEBUG__
			// not found => error
			if (curr == last) abort( "(uQueue &)%p.remove( %p ) : Node is not in list.", this, n );
			#endif // __U_DEBUG__
			prev = curr;
			curr = (T *)uNext(curr);
		}
		return n;
	} // post: !n->listed().

	T * dropTail() {									// O(n)
		T * n = tail();
		return n ? remove( n ), n : nullptr;
	}

	// Transfer the "from" list to the end of queue sequence; the "from" list is empty after the transfer.
	void transfer( uQueue<T> & from ) {
		if ( from.empty() ) return;						// "from" list empty ?
		if ( empty() ) {								// "to" list empty ?
			root = from.root;
		} else {										// "to" list not empty
			uNext(last) = from.root;
		}
		last = from.last;
		from.root = from.last = nullptr;				// mark "from" list empty
	}

	// Transfer the "from" list up to node "n" to the end of queue list; the "from" list becomes the list after node "n".
	// Node "n" must be in the "from" list.
	void split( uQueue<T> & from, T * n ) {
		#ifdef __U_DEBUG__
		if ( ! n->listed() ) abort( "(uQueue &)%p.split( %p ) : Node is not on a list.", this, n );
		#endif // __U_DEBUG__
		uQueue<T> to;
		to.root = from.root;							// start of "to" list
		to.last = n;									// end of "to" list
		from.root = (T *)uNext( n );					// start of "from" list
		if ( n == from.root ) {							// last node in list ?
			from.root = from.last = nullptr;			// mark "from" list empty
		} else {
			uNext( n ) = n;								// fix end of "to" list
		}
		transfer( to );
	}
};


// A uQueueIter<T> is a subclass of uColIter<T> that generates the elements of a uQueue<T>.  It returns the elements in
// the order that they would be returned by drop().

template<typename T> class uQueueIter : public uColIter<T> {
  protected:
	using uColIter<T>::curr;
	using uColIter<T>::uNext;
  public:
	uQueueIter():uColIter<T>() {}						// post: elts = null.

	// create an iterator active in queue q
	uQueueIter( const uQueue<T> & q ) {					// post: elts = {e in q}.
		curr = q.head();
	}
	uQueueIter( T * start ) {							// post: elts = {e in q}.
		curr = start;
	}
	// make existing iterator active in queue q
	void over( const uQueue<T> & q ) {					// post: elts = {e in q}.
		curr = q.head();
	} // post: curr = {e in q}

	bool operator>>( T *& tp ) {
		if (curr) {
			tp = curr;
			T * n = (T *)uNext(curr);
			curr = (n == curr) ? nullptr : n;
		} else tp = nullptr;
		return tp != nullptr;
	}
	// post: elts == null & !operator>>(tp) | elts != null & *tp' in elts & elts' == elts - *tp & operator>>(tp)
};


// Local Variables: //
// compile-command: "make install" //
// End: //
