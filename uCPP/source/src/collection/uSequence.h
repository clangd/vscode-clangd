//                              -*- Mode: C++ -*- 
// 
// Copyright (C) Glen Ditchfield 1994
// 
// uSequence.h -- 
// 
// Author           : Glen Ditchfield
// Created On       : Sun Feb 13 19:56:07 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Dec 27 21:40:25 2020
// Update Count     : 172
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

// Class that sequence elements derive from.

class uSeqable : public uColable {
	friend class uSFriend;

	uSeqable * back;									// pointer to previous node
  public:
	uSeqable() {
		back = nullptr;
	}
	uSeqable * getback() {
		return back;
	}
};


// uSFriend and its descendents have access to uSeqable::back.
 
class uSFriend {
  protected:
	uSeqable *& uBack( uSeqable * sp ) const {
		return sp->back;
	}
};


// A uSequence<T> is a uCollection<T> defining the ordering of a uStack and uQueue, and to insert and remove elements
// anywhere in the sequence. T must be a public descendant of uSeqable.

// The implementation is a typical doubly-linked list, except the next field of the last node points at the first node
// and the back field of the last node points at the first node (circular).

template<typename T> class uSequence: public uCollection<T>, protected uSFriend {
  protected:
	using uCollection<T>::root;
  public:
	uSequence( const uSequence & ) = delete;			// no copy
	uSequence( uSequence && ) = delete;
	uSequence & operator=( const uSequence ) = delete;	// no assignment
	uSequence & operator=( uSequence && ) = delete;

	using uCollection<T>::empty;
	using uCollection<T>::head;
	using uCollection<T>::uNext;

	uSequence() : uCollection<T>() {}					// post: empty().

	// Return a pointer to the last sequence element, without removing it.
	T * tail() const {
		return root ? (T *)uBack( root ) : nullptr;
	} // post: empty() & tail() == nullptr | !empty() & tail() in *this

	// Return a pointer to the element after *n, or nullptr if list empty.
	T * succ( T * n ) const {							// pre: *n in *this
		#ifdef __U_DEBUG__
		if ( ! n->listed() ) abort( "(uSequence &)%p.succ( %p ) : Node is not on a list.", this, n );
		#endif // __U_DEBUG__
		return (uNext( n ) == root) ? nullptr : (T *)uNext( n );
	} // post: n == tail() & succ(n) == nullptr | n != tail() & *succ(n) in *this

	// Return a pointer to the element before *n, or nullptr if list empty.
	T * pred( T * n ) const {							// pre: *n in *this
		#ifdef __U_DEBUG__
		if ( ! n->listed() ) abort( "(uSequence &)%p.pred( %p ) : Node is not on a list.", this, n );
		#endif // __U_DEBUG__
		return (n == root) ? nullptr : (T *)uBack( n );
	} // post: n == head() & head(n) == nullptr | n != head() & *pred(n) in *this

	// Insert *n into the sequence before *bef, or at the end if bef == nullptr.
	T * insertBef( T * n, T * bef ) {					// pre: !n->listed() & *bef in *this
		#ifdef __U_DEBUG__
		if ( n->listed() ) abort( "(uSequence &)%p.insertBef( %p, %p ) : Node is already on another list.", this, n, bef );
		#endif // __U_DEBUG__
		if ( bef == root ) {							// must change root
			if ( root ) {
				uNext( n ) = root;
				uBack( n ) = uBack( root );
				// inserted node must be consistent before it is seen
				asm( "" : : : "memory" );				// prevent code movement across barrier
				uBack( root ) = n;
				uNext( uBack( n ) ) = n;
			} else {
				uNext( n ) = n;
				uBack( n ) = n;
			} // if
			// inserted node must be consistent before it is seen
			asm( "" : : : "memory" );					// prevent code movement across barrier
			root = n;
		} else {
			if ( ! bef ) bef = root;
			uNext( n ) = bef;
			uBack( n ) = uBack( bef );
			// inserted node must be consistent before it is seen
			asm( "" : : : "memory" );					// prevent code movement across barrier
			uBack( bef ) = n;
			uNext( uBack( n ) ) = n;
		} // if
		return n;
	} // post: n->listed() & *n in *this & succ(n) == bef

	// Insert *n into the sequence after *aft, or at the beginning if aft == nullptr.
	T * insertAft( T * aft, T * n ) {					// pre: !n->listed() & *aft in *this
		#ifdef __U_DEBUG__
		if ( n->listed() ) abort( "(uSequence &)%p.insertAft( %p, %p ) : Node is already on another list.", this, aft, n );
		#endif // __U_DEBUG__
		if ( ! aft ) {									// must change root
			if ( root ) {
				uNext( n ) = root;
				uBack( n ) = uBack( root );
				// inserted node must be consistent before it is seen
				asm( "" : : : "memory" );				// prevent code movement across barrier
				uBack( root ) = n;
				uNext( uBack( n ) ) = n;
			} else {
				uNext( n ) = n;
				uBack( n ) = n;
			} // if
			asm( "" : : : "memory" );					// prevent code movement across barrier
			root = n;
		} else {
			uNext( n ) = uNext( aft );
			uBack( n ) = aft;
			// inserted node must be consistent before it is seen
			asm( "" : : : "memory" );					// prevent code movement across barrier
			uBack( (T *)uNext( n ) ) = n;
			uNext( aft ) = n;
		} // if
		return n;
	} // post: n->listed() & *n in *this & succ(n) == bef

	T * remove( T * n ) {								// O(1)
		#ifdef __U_DEBUG__
		if ( ! n->listed() ) abort( "(uSequence &)%p.remove( %p ) : Node is not on a list.", this, n );
		#endif // __U_DEBUG__
		if ( n == root ) {
			if ( uNext( root ) == root ) root = nullptr;
			else root = (T *)uNext( root );
		} // if
		uBack( (T *)uNext( n ) ) = uBack( n );
		uNext( uBack( n ) ) = uNext( n );
		uNext( n ) = uBack( n ) = nullptr;
		return n;
	} // post: !n->listed().

	// Add an element to the head of the sequence.
	T * addHead( T * n ) {								// pre: !n->listed(); post: n->listed() & head() == n
		return insertAft( nullptr, n );
	}

	// Add an element to the tail of the sequence.
	T * addTail( T * n ) {								// pre: !n->listed(); post: n->listed() & head() == n
		return insertBef( n, nullptr );
	}

	// Add an element to the tail of the sequence.
	T * add( T * n ) {									// pre: !n->listed(); post: n->listed() & head() == n
		return addTail( n );
	}

	// Remove and return the head element in the sequence.
	T * dropHead() {
		T * n = head();
		return n ? remove( n ), n : nullptr;
	}

	// Remove and return the head element in the sequence.
	T * drop() {
		return dropHead();
	}

	// Remove and return the tail element in the sequence.
	T * dropTail() {
		T * n = tail();
		return n ? remove( n ), n : nullptr;
	}

	// Transfer the "from" list to the end of this sequence; the "from" list is empty after the transfer.
	void transfer( uSequence<T> & from ) {
		if ( from.empty() ) return;						// "from" list empty ?
		if ( empty() ) {								// "to" list empty ?
			root = from.root;
		} else {										// "to" list not empty
			T * toEnd = (T *)uBack( root );
			T * fromEnd = (T *)from.uBack( from.root );
			uBack( root ) = fromEnd;
			from.uNext( fromEnd ) = root;
			from.uBack( from.root ) = toEnd;
			uNext( toEnd ) = from.root;
		} // if
		from.root = nullptr;							// mark "from" list empty
	}

	// Transfer the "from" list up to node "n" to the end of this list; the "from" list becomes the sequence after node "n".
	// Node "n" must be in the "from" list.
	void split( uSequence<T> & from, T * n ) {
		#ifdef __U_DEBUG__
		if ( ! n->listed() ) abort( "(uSequence &)%p.split( %p ) : Node is not on a list.", this, n );
		#endif // __U_DEBUG__
		uSequence<T> to;
		to.root = from.root;							// start of "to" list
		from.root = (T *)uNext( n );					// start of "from" list
		if ( to.root == from.root ) {					// last node in list ?
			from.root = nullptr;						// mark "from" list empty
		} else {
			uBack( from.root ) = (T *)uBack( to.root );	// fix "from" list
			uNext( uBack( to.root ) ) = from.root;
			uNext( n ) = to.root;						// fix "to" list
			uBack( to.root ) = n;
		} // if
		transfer( to );
	}
};


// uSeqIter<T> is used to iterate over a uSequence<T> in head-to-tail order.
template<typename T> class uSeqIter: public uColIter<T>, protected uSFriend {
  protected:
	using uColIter<T>::curr;

	const uSequence<T> * seq;
  public:
	uSeqIter() : uColIter<T>() {
		seq = nullptr;
	} // post: elts = null.

	// Create a iterator active in sequence s
	uSeqIter( const uSequence<T> & s ) {	
		seq = &s;
		curr = s.head();
	} // post: elts = {e in s}.

	uSeqIter( const uSequence<T> & s, T * start ) {	
		seq = &s;
		curr = start;
	} // post: elts = {e in s}.

	// Make the iterator active in sequence s.
	void over( const uSequence<T> & s ) {
		seq = &s;
		curr = s.head();
	} // post: elts = {e in s}.

	bool operator>>( T *& tp ) {
		if (curr) {
			tp = curr;
			T * n = seq->succ(curr);
			curr = (n == seq->head()) ? nullptr : n;
		} else tp = nullptr;
		return tp != nullptr;
	}
};


// A uSeqIterRev<T> is used to iterate over a uSequence<T> in tail-to-head order.
template<typename T> class uSeqIterRev: public uColIter<T>, protected uSFriend {
  protected:
	using uColIter<T>::curr;

	const uSequence<T> * seq;
  public:
	uSeqIterRev() : uColIter<T>() {
		seq = nullptr;
	} // post: elts = null

	// Create a iterator active in sequence s.
	uSeqIterRev( const uSequence<T> & s ) {
		seq = &s;
		curr = s.tail();
	} // post: elts = {e in s}.

	uSeqIterRev( const uSequence<T> & s, T * start ) {
		seq = &s;
		curr = start;
	} // post: elts = {e in s}.

	// Make the iterator active in sequence s.
	void over( const uSequence<T> & s ) {
		seq = &s;
		curr = s.tail();
	} // post: elts = {e in s}.

	bool operator>>( T *& tp ) {
		if (curr) {
			tp = curr;
			T * n = seq->pred(curr);
			curr = (n == seq->tail()) ? nullptr : n;
		} else tp = nullptr;
		return tp != nullptr;
	}
};


// Local Variables: //
// compile-command: "make install" //
// End: //
