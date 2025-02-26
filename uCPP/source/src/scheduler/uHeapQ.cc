//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Ashif S. Harji 2000
//
// uHeapQ.cc --
//
// Author           : Ashif S. Harji
// Created On       : Fri Feb  4 10:57:13 2000
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr  9 17:04:34 2022
// Update Count     : 57
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
#include <uHeapQ.h>
//#include <uDebug.h>

#define uLockAcquired  0
#define uLockReleased  1

// compare for Heapify property, ie parent(i) <= i
int uPriorityQ::compare( int k1, int k2 ) {
	if ( k1 < k2 ) {
		return 1;
	} else if ( k1 == k2 ){
		return 0;
	} else {
		return -1;
	} // if
} // compare


void uPriorityQ::exchange( uHeapable<int, uHeapBaseSeq *> &x, uHeapable<int, uHeapBaseSeq *> &y ) {
	uHeapable<int, uHeapBaseSeq *> temp = x;
	int t_index = x.data->index;
	x = y;
	y = temp;
	y.data->index = x.data->index;						// swapping locations requires the index pointers to be updated
	x.data->index = t_index;
	#ifdef DEBUG_SHOW_COUNT
	NumExchg += 1;
	#endif
} // uPriorityQ::exchange


uPriorityQ::uPriorityQ() : heap( &compare, &exchange ) {
	//for ( int i = 0; i < __U_MAX_NUMBER_PRIORITIES__ ; i += 1 ) {
	//    heap.A[i + 1].data = &(objects[i]);
	//} // for
	//num_priorities = 1; // first is always non-real-time tasks
	// objects[0].priority = 2147483647;  // use a large number, syn which addInitialize
	// uPriorityValue = -1;
	// uInheritTask = nullptr;
	// uQueueNum = -1;
	executeHooks = true;
	currPriority = -1;
	currQueueNum = -1;
} // uPriorityQ::uPriorityQ


bool uPriorityQ::empty() const {
	return heap.size() == 0;
} // uPriorityQ::empty


uBaseTaskDL *uPriorityQ::head() const {
	if ( ! empty() ) {
		uHeapable<int, uHeapBaseSeq *> rqueue;
		heap.getRoot( rqueue );
		return rqueue.data->queue.head();
	} else {
		return nullptr;
	} // if
} // uPriorityQ::head


int uPriorityQ::add( uBaseTaskDL *node, uBaseTask *owner ) {
	// Dynamic check to verify that the task being added to entry queue is compliant with PIHeap type.
	uPIHeap *PIHptr = dynamic_cast<uPIHeap *>(node->task().uPIQ);
	if ( PIHptr == nullptr ) {
		abort("(uPriorityQ &)%p.add : Task %p has incorrect uPIQ type for mutex object.", this, &node->task());
	} //if

	// check if your priority needs to be updated
	if ( PIHptr->getHighestPriority() < getActivePriorityValue( node->task() )  ) {
		uThisCluster().taskSetPriority( node->task(), node->task() );
	} // if

	// As uCurrentSerial is updated, the calling task's priority can no longer change because the tasks uPIQ is fixed as
	// the entry lock is acquired.  So subsequent updates will only reaffirm the task's current priority.

	int priority = getActivePriorityValue( node->task() );
	int queueNum = getActiveQueueValue( node->task() ); // use the node for you active priority

	// if empty then must add to heap, otherwise just insert node
	if ( objects[queueNum].queue.empty() ) {
		objects[queueNum].queue.add(node);
		// the index must be set before inserting because the index can change as part of the insert
		objects[queueNum].index = heap.size() + 1;
		heap.insert( priority, &(objects[queueNum]) );
	} else {
		objects[queueNum].queue.add(node);
	} // if

	// only perform inheritance for entry list
	if ( isEntryBlocked( node->task() ) && checkHookConditions( *owner, node->task() ) ) { // TEMP: entry queue??
		return( afterEntry( owner ) );					// perform any priority inheritance
	} else {
		return uLockAcquired;
	} // if
} // uPriorityQ::add


uBaseTaskDL *uPriorityQ::drop() {
	if ( ! empty() ) {
		uHeapable<int,uHeapBaseSeq *> rqueue;
		heap.getRoot( rqueue );
		uBaseTaskDL *pnode = rqueue.data->queue.drop();

		if ( rqueue.data->queue.empty() ){
			heap.deleteRoot();
		} // if
		return pnode;
	} else {
		return nullptr;
	} // if
} // uPriorityQ::drop


void uPriorityQ::remove( uBaseTaskDL *node ) {
	// Use stored queue value because this task has entry lock, so its uPIQ may be updated, but not its position on the
	// entry queue.
	int queueNum = getActiveQueueValue( node->task() );	// use the node for you active priority

	// do remove
	objects[queueNum].queue.remove(node);

	// if empty then must remove from heap
	if ( objects[queueNum].queue.empty() ) {
		heap.deletenode( heap.A[objects[queueNum].index] );
	} // if
} // uPriorityQ::remove


int uPriorityQ::afterEntry(uBaseTask *owner ) {			// use pointer to owner as it could be Null
	// Static_cast to PIHeap are valid here as add and onAcquire already verify that the associated tasks use type
	// uPIHeap.

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
		(static_cast<uPIHeap *>(owner->uPIQ))->remove( currPriority, currQueueNum );

		// reset priority value for monitor
		currPriority = uCalling.getActivePriorityValue();
		currQueueNum = uCalling.getActiveQueueValue();

	    // update mutex owner's uPIQ for new priority
		(static_cast<uPIHeap *>(owner->uPIQ))->add( currPriority, currQueueNum ) ;

		// does inheritance occur ?
		if ( currPriority < owner->getActivePriorityValue() ) {

			uRepositionEntry rep(*owner, uCalling);
	        // if task is blocked on entry list, adjust and perform transitivity
			if ( isEntryBlocked( *owner ) ) {
				uRelPrevLock = rep.uReposition(true);
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
} // uPriorityQ::afterEntry


void uPriorityQ::onAcquire(uBaseTask &owner ) {
	// Dynamic check to verify that the task acquiring the serial is compliant with PIHeap type.
	uPIHeap *PIHptr = dynamic_cast<uPIHeap *>(owner.uPIQ);
	if ( PIHptr == nullptr ) {
		abort("(uPriorityQ &)%p.onAcquire : Task %p has incorrect uPIQ type for mutex object.", this, &owner);
	} //if

	// check if mutex owner's priority needs to be updated
	if ( PIHptr->getHighestPriority() < getActivePriorityValue( owner ) ) {
		uThisCluster().taskSetPriority( owner, owner );
	} // if

	// remember current priority value, update task's uPIQ
	currPriority = owner.getBasePriority();
	currQueueNum = owner.getBaseQueue();
	PIHptr->add( currPriority, currQueueNum );

	// perform priority inheritance
	afterEntry( &owner );
} // uPriorityQ::onAcquire


void uPriorityQ::onRelease(uBaseTask &uOldOwner ) {
	// static_cast to PIHeap are valid here as add and onAcquire already verify that the associated tasks use type
	// uPIHeap.

	// update task's uPIQ, reset stored values
	(static_cast<uPIHeap *>(uOldOwner.uPIQ))->remove( currPriority, currQueueNum );
	currPriority  = -1;
	currQueueNum = -1;

	// reset active priority if necessary only case where priority can decrease
	if ( (static_cast<uPIHeap *>(uOldOwner.uPIQ))->empty() ||
		 (static_cast<uPIHeap *>(uOldOwner.uPIQ))->getHighestPriority() > getActivePriorityValue( uOldOwner ) ) {
		uThisCluster().taskSetPriority( uOldOwner, uOldOwner );
	} // if
} // uPriorityQ::onRelease

// Local Variables: //
// compile-command: "make install" //
// End: //
