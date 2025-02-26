//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Ashif S. Harji 2000
//
// uPIHeap.cc --
//
// Author           : Ashif S. Harji
// Created On       : Fri Feb  4 11:10:44 2000
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr  9 17:10:23 2022
// Update Count     : 52
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


#define __U_KERNEL__
#include <uC++.h>
#include <uStaticPIQ.h>
//#include <uDebug.h>

#include <cstring>										// access: ffs


uStaticPIQ::uStaticPIQ() {
	for ( int i = 0; i < __U_MAX_NUMBER_PRIORITIES__ ; i += 1 ) {
		objects[i] = 0;
	} // for
	mask = 0;

} // uStaticPIQ::uStaticPIQ

bool uStaticPIQ::empty() const {
	return mask == 0;
} // uStaticPIQ::empty

//int uStaticPIQ::head() {
//	int highestPriority = ffs( mask ) - 1;
//
//	if ( highestPriority >= 0 ) {
//	    return highestPriority;
//	} else {
//	    return -1;
//	} // if
//
//} // uStaticPIQ::head

int uStaticPIQ::getHighestPriority() {
	int highestPriority = ffs( mask ) - 1;

	if ( highestPriority >= 0 ) {
		return highestPriority;
	} else {
		return -1;
	} // if
} // uStaticPIQ::getHighestPriority

void uStaticPIQ::add( int priority ) {
	lock.acquire();

	assert( 0 <= priority && priority <= __U_MAX_NUMBER_PRIORITIES__ - 1 );
	objects[priority] += 1;
	mask |= 1ul << priority;

	lock.release();
} // uPriorityScheduleQueue::add

int uStaticPIQ::drop() {
	lock.acquire();

	int highestPriority = ffs( mask ) - 1;

	if ( highestPriority >= 0 ) {
		objects[highestPriority] -= 1;

		if ( objects[highestPriority] == 0 ) {
			mask &= ~ ( 1ul << highestPriority );
		} // if

	} // if

	lock.release();
	return highestPriority;
} // uStaticPIQ::drop

void uStaticPIQ::remove( int priority ) {
	lock.acquire();

	assert( 0 <= priority && priority <= __U_MAX_NUMBER_PRIORITIES__ - 1 );
	objects[priority] -= 1;
	if ( objects[priority] == 0 ) {
		mask &= ~ ( 1ul << priority );
	} // if

	lock.release();
} // uStaticPIQ::remove


// Local Variables: //
// compile-command: "make install" //
// End: //
