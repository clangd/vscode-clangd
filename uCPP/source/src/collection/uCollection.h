//                              -*- Mode: C++ -*- 
// 
// Copyright (C) Glen Ditchfield 1994
// 
// uCollection.h -- Classes defining collections and related iterators.
// 
// Author           : Glen Ditchfield
// Created On       : Sun Feb 13 14:18:43 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Nov 16 14:57:56 2021
// Update Count     : 114
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


// Class that collection elements inherit from.  A uColable can only be in one collection at a time.

class uColable {
	friend class uCFriend;

	uColable * next;									// next node in the list
	// class invariant: (next != 0) <=> listed()
  public:
	inline uColable() {
		next = 0;
	}													// post: ! listed()
	// Return true iff *this is an element of any collection.
	inline bool listed() const {						// pre:  this != 0
		return next != 0;
	}
	inline uColable * getnext() {
		return next;
	}
};


// uCFriend and its descendants have access to uColable::next.
 
class uCFriend {
  protected:
	uColable *& uNext( uColable * cp ) const {
		return cp->next;
	}
};


// A uCollection<T> contains elements of class T, which must be a public descendant of uColable.  No particular ordering
// of elements is specified; descendants of uCollection<T> specify orderings.  uCollection<T> is an abstract class:
// instances cannot be declared.

// The default implementation of head() returns root, since it must point at some collection element if the collection
// is not empty.  drop() and add() are left to descendants, since they define the order of elements.

template<typename T> class uCollection : protected uCFriend {
  protected:
	T * root;											// pointer to root element of list
	// class invariant: root == 0 & empty() | *root in *this
  public:
	uCollection( const uCollection & ) = delete;		// no copy
	uCollection( uCollection && ) = delete;
	uCollection & operator=( const uCollection & ) = delete; // no assignment
	uCollection & operator=( uCollection && ) = delete;

	inline uCollection() {	
		root = 0;
	}													// post: empty()
	inline bool empty() const {							// 0 <=> *this contains no elements
		return root == 0;
	}
	inline T * head() const {
		return root;
	}													// post: empty() & head() == 0 | !empty() & head() in *this
	inline void add( T * n ) {							// pre: !n->listed();
		n = 0;
	}													// post: n->listed() & *n in *this
	inline T * drop() {
		return 0;
	}													// post: empty() & drop() == 0 | !empty() & !drop()->listed()
};


// A uColIter<T> a stream of the elements of a uCollection<T>.  It is used to
// iterate over a collection.  Every descendant of uCollection has associated
// iterators descended from uColIter.  By convention, the iterator ThingIter<T>
// for collection Thing<T> has members
//	ThingIter<T>::ThingIter<T>();
// to create inactive iterators, and
//	ThingIter<T>::ThingIter<T>(const Thing<T> &);
//	void ThingIter<T>::over(const Thing<T> &);
// to create active iterators.  Usage is
//	Thing<Foo>	c;
//	ThingIter<Foo>	g(c);
//	Foo		*foop;
//	while (g >> foop) ...
// or
//	for (g.over(c); g >> foop;) ...;
// If a collection with an active iterator is modified, subsequent values
// produced by the iterator are not defined.

template<typename T> class uColIter : protected uCFriend {
  protected:
	T * curr;											// element to be returned by >>
  public:
	inline uColIter() {
		curr = 0;
	}													// post: elts = null
	bool operator>>( T *& tp ) {
		tp = 0;
		return 0;
	}
	// post: elts == null & !operator>>(tp) | elts != null & *tp' in elts & elts' == elts - *tp & operator>>(tp)
};


// Local Variables: //
// compile-command: "make install" //
// End: //
