//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Ashif S. Harji 2000
//
// uStaticPriorityQ.cc --
//
// Author           : Ashif S. Harji
// Created On       : Mon Feb  1 15:06:12 1999
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr  9 17:12:10 2022
// Update Count     : 160
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
#include <uStaticPriorityQ.h>
#include <uStaticPIQ.h>
//#include <uDebug.h>

#define uLockAcquired  0
#define uLockReleased  1


int uStaticPriorityQ::afterEntry( uBaseTask *owner ) { // use pointer to owner as it could be Null
	// static_cast to uStaticPIQ are valid here as add and onAcquire already verify that the associated tasks use type
	// uStaticPIQ.

	// assume entry lock acquired
	int uRelPrevLock = uLockAcquired;

	// if entry queue empty (called by owner) or no owner, then no inheritance
	if ( empty() || owner == nullptr /* || currPriority == -1 */ ) {
		return uRelPrevLock;
	} // if

	uBaseTask &uCalling = head()->task();				// can't be null as not empty

	// does node need to be updated?
	if ( uCalling.getActivePriorityValue() < currPriority ) {
		// only task with entry lock can be modifying this mutex's node remove node
		(static_cast<uStaticPIQ *>(owner->uPIQ))->remove( currPriority );

		// reset priority value for monitor
		currPriority = uCalling.getActivePriorityValue();

		// update mutex owner's uPIQ for new priority
		(static_cast<uStaticPIQ *>(owner->uPIQ))->add( currPriority ) ;

		// does inheritance occur ?
		if ( currPriority < owner->getActivePriorityValue() ) {

			uRepositionEntry rep(*owner, uCalling);
			//uSerial *uRememberSerial = &(owner->getSerial());
			// if task is blocked on entry list, adjust and perform transitivity
			if ( isEntryBlocked( *owner ) ) {

				uRelPrevLock = rep.uReposition(true);
				//uRepositionWrapper( owner, uRememberSerial, s );
				//uRememberSerial->lock.acquire();

				// if owner's current mutex object changes, then owner fixes
				// its own active priority. Recheck if inheritance is necessary
				// as only owner can lower its priority => updated
				//if ( uRememberSerial != &(owner->getSerial()) || ! isEntryBlocked( *owner ) ||
				//	(static_cast<uStaticPIQ *>(owner->uPIQ))->getHighestPriority() >= owner->getActivePriorityValue() ) {
				// As owner restarted, the end of the blocking chain has been reached.
				//uRememberSerial->lock.release();
				//return uRelPrevLock;
				//} // if

				//s->lock.release();  // release the old lock as correct current lock is acquired
				//uRelPrevLock = uLockReleased;

				//if ( uReposition( owner, uRememberSerial ) == uLockAcquired ) {
				// only last call does not release lock, so reacquire first entry lock
				//uThisTask().getSerial().lock.acquire();
				//uRememberSerial->lock.release();
				//} // if

				// proceed with transitivity
				// remove from entry queue
				//uRememberSerial->uEntryList.remove( &(owner->uEntryRef) );
				// remove from mutex queue
				//owner->uCalledEntryMem->remove( &(owner->uMutexRef) );

				// call cluster routine to adjust ready queue and active
				// priority as owner is not on entry queue, it can be updated
				// based on its uPIQ
				//uThisCluster().taskSetPriority( *owner, *owner );

				// add to mutex queue
				//owner->uCalledEntryMem->add( &(owner->uMutexRef), uRememberSerial->uMutexOwner, uRememberSerial );

				// add to entry queue, automatically does transitivity
				//if ( uRememberSerial->uEntryList.add( &(owner->uEntryRef), uRememberSerial->uMutexOwner, uRememberSerial ) == uLockAcquired ) {
				// only last call does not release lock, so reacquire first entry lock
				//uThisTask().uCurrentSerial->lock.acquire();
				//uRememberSerial->lock.release();
				//} // if

			} else {
				// call cluster routine to adjust ready queue and active priority Note: can only raise priority to at
				// most uCalling, otherwise updating owner's priority can conflit with the owner blocking on an entry
				// queue at a particular priority level.  Furthermore, uCalling's priority is fixed while the entry lock
				// of where it is blocked (s->lock) is acquired, but uThisTask()'s priority can change as entry lock's
				// are released along inheritance chain.
				uThisCluster().taskSetPriority( *owner, uCalling );
			} // if
		} // if
	} // if

	return uRelPrevLock;
} // uStaticPriorityQ::afterEntry


int uStaticPriorityQ::add( uBaseTaskDL *node, uBaseTask *owner ) {
	// Dynamic check to verify that the task being added to entry queue is
	// compliant with PIHeap type.
	uStaticPIQ *PIQptr = dynamic_cast<uStaticPIQ *>(node->task().uPIQ);
	if ( PIQptr == nullptr ) {
		//return uLockAcquired; // TEMPORARY
		abort("(uStaticPriorityQ &)%p.add : Task %p has incorrect uPIQ type for mutex object.", this, &node->task());
	} //if

	// check if your priority needs to be updated
	if ( PIQptr->getHighestPriority() < getActivePriorityValue( node->task() )  ) {
		uThisCluster().taskSetPriority( node->task(), node->task() );
	} // if

	// As uCurrentSerial is updated, the calling task's priority can no longer change because the tasks uPIQ is fixed as
	// the entry lock is acquired.  So subsequent updates will only reaffirm the task's current priority.

	int priority = getActivePriorityValue( node->task() );

	assert( 0 <= priority && priority <= __U_MAX_NUMBER_PRIORITIES__ - 1 );
	objects[priority].add( node );
	mask |= 1ul << priority;

	// only perform inheritance for entry list
	if ( /* this == &(s->uEntryList) */ isEntryBlocked( node->task() ) && checkHookConditions( *owner, node->task() ) ) { // TEMP: entry queue??
		return( afterEntry( owner ) );					// perform any priority inheritance
	} else {
		return uLockAcquired;
	} // if

} // uStaticPriorityQ::add


void uStaticPriorityQ::onAcquire( uBaseTask &owner ) {
	// Dynamic check to verify that the task acquiring the serial is compliant with PIHeap type.
	uStaticPIQ *PIQptr = dynamic_cast<uStaticPIQ *>(owner.uPIQ);
	if ( PIQptr == nullptr ) {
		abort("(uStaticPriorityQ &)%p.onAcquire : Task %p has incorrect uPIQ type for mutex object.", this, &owner);
	} //if

	// check if mutex owner's priority needs to be updated
	if ( PIQptr->getHighestPriority() < getActivePriorityValue( owner ) ) {
		uThisCluster().taskSetPriority( owner, owner );
	} // if

	// remember current priority value, update task's uPIQ
	currPriority = owner.getBasePriority();

	PIQptr->add( currPriority );

	// perform priority inheritance
	afterEntry( &owner );
} // uStaticPriorityQ::onAcquire


void uStaticPriorityQ::onRelease( uBaseTask &oldOwner ) {
	// static_cast to PIHeap are valid here as add and onAcquire already verify that the associated tasks use type
	// uStaticPIQ.

	// update task's uPIQ, reset stored values
	(static_cast<uStaticPIQ *>(oldOwner.uPIQ))->remove( currPriority );
	currPriority = -1;

	// reset active priority if necessary only case where priority can decrease
	if ( (static_cast<uStaticPIQ *>(oldOwner.uPIQ))->empty() || (static_cast<uStaticPIQ *>(oldOwner.uPIQ))->getHighestPriority() > getActivePriorityValue( oldOwner ) ) {
		uThisCluster().taskSetPriority( oldOwner, oldOwner );
	} // if
} // uStaticPriorityQ::onRelease

// Local Variables: //
// compile-command: "make install" //
// End: //
