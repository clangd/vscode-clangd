//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2007
//
// uCeilingQ.h --
//
// Author           :
// Created On       : Thu Aug  9 14:50:41 2007
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr  9 16:57:01 2022
// Update Count     : 14
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

//#include <uDebug.h>

#include <uC++.h>


#define __U_MAX_NUMBER_PRIORITIES__ 32


class uCeilingQ : public uBasePrioritySeq {
  protected:
	uBaseTaskSeq objects[__U_MAX_NUMBER_PRIORITIES__];
	unsigned int mask;									// allow access to all queue flags

	int currPriority;
  public:
	uCeilingQ() {
		executeHooks = true;
	} // uCeilingQ::uCeilingQ

	bool empty() const {
		return list.empty();
	} // uCeilingQ::empty

	uBaseTaskDL *head() const {
		return list.head();
	} // uCeilingQ::head

	int add( uBaseTaskDL *node, uBaseTask *owner ) {
		#ifdef KNOT
		uFetchAdd( UPP::Statistics::mutex_queue, 1 );
		#endif // KNOT
		list.addTail( node );
		return 0;
	} // uCeilingQ::add

	uBaseTaskDL *drop() {
		#ifdef KNOT
		uFetchAdd( UPP::Statistics::mutex_queue, -1 );
		#endif // KNOT
		return list.dropHead();
	} // uCeilingQ::drop

	void remove( uBaseTaskDL *node ) {
		#ifdef KNOT
		uFetchAdd( UPP::Statistics::mutex_queue, -1 );
		#endif // KNOT
		list.remove( node );
	} // uCeilingQ::remove

	void onAcquire( uBaseTask &owner ) {
		setActivePriority( owner, getActivePriorityValue( owner ) + 1 );
	} // uCeilingQ::onAcquire

	void onRelease( uBaseTask &oldOwner ) {
		setActivePriority( oldOwner, getActivePriorityValue( oldOwner ) - 1 );
	} // uCeilingQ::onRelease
}; // uCeilingQ


// Local Variables: //
// compile-command: "make install" //
// End: //
