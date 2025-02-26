//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Ashif S. Harji 2000
//
// uPIHeap.cc --
//
// Author           : Ashif S. Harji
// Created On       : Fri Feb  4 11:10:44 2000
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr  9 17:07:00 2022
// Update Count     : 33
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


// compare for Heapify property, ie parent(i) <= i
int uPIHeap::compare( int k1, int k2 ) {
	if ( k1 < k2 ) {
		return 1;
	} else if ( k1 == k2 ){
		return 0;
	} else {
		return -1;
	} // if
} // compare


void uPIHeap::exchange( uHeapable<int, uHeapBaseSeq *> &x, uHeapable<int, uHeapBaseSeq *> &y ) {
	uHeapable<int, uHeapBaseSeq *> temp = x;
	int t_index = x.data->index;
	x = y;
	y = temp;
	y.data->index = x.data->index;						// swapping locations requires the index pointers to be updated
	x.data->index = t_index;
	#ifdef DEBUG_SHOW_COUNT
	NumExchg += 1;
	#endif
} // uHeap::exchange


uPIHeap::uPIHeap() : heap( &compare, &exchange ) {
	for ( int i = 0; i < __U_MAX_NUMBER_PRIORITIES__ ; i += 1 ) {
		objects[i].count = 0;
	} // for
	//num_priorities = 1; // first is always non-real-time tasks
	//objects[0].priority = 2147483647;  // use a large number, syn which addInitialize
	//objects[0].count = 0;
} // uPIHeap::uPIHeap


bool uPIHeap::empty() const {
	return heap.size() == 0;
} // uPIHeap::empty


int uPIHeap::head() const {
	if ( ! empty() ) {
		uHeapable<int,uHeapBaseSeq *> rqueue;
		heap.getRoot( rqueue );
		return rqueue.data->queueNum;
	} else {
		return -1;
	} // if
} // uPIHeap::head


int uPIHeap::getHighestPriority() {
	if ( ! empty() ) {
		uHeapable<int,uHeapBaseSeq *> rqueue;
		heap.getRoot( rqueue );
		return rqueue.key;
	} else {
		return -1;
	} // if
} // uPIHeap::getHighestPriority


void uPIHeap::add( int priority, int queueNum ) {
	//int priority;
	//int queueNum;

	// as the entry lock is acquired, no other task can be manipulating this node,
	// so lock does not need to be aquired yet.
	//if ( uOwner == &(node->task()) ) {
	//priority = getBasePriority( node->task() );
	//queueNum = getBaseQueue( node->task() );  // use the node for your base priority
	//} else {
	//priority = getActivePriority( node->task() );
	//queueNum = getActiveQueue( getInheritTask( node->task() ) ); // use the node for your active priority
	//} // if

	lock.acquire();

	// if empty then must add to heap, otherwise just insert node
	if ( objects[queueNum].count == 0 ) {
		// the index must be set before inserting because the index can change as part of the insert
		objects[queueNum].index = heap.size() + 1;
		heap.insert( priority, &(objects[queueNum]) );
	} // if
	//objects[queueNum].queue.add(node);
	objects[queueNum].count += 1;
	objects[queueNum].queueNum = queueNum;
	lock.release();
} // uPriorityScheduleQueue::add


int uPIHeap::drop() {
	lock.acquire();

	if ( ! empty() ) {
		uHeapable<int,uHeapBaseSeq *> rqueue;
		heap.getRoot( rqueue );
		int queueNum = rqueue.data->queueNum;

		if ( rqueue.data->count == 0 ){
			heap.deleteRoot();
		} // if
		lock.release();
		return queueNum;
	} else {
		lock.release();
		return -1;
	} // if
} // uPIHeap::drop


void uPIHeap::remove( int /* priority */, int queueNum ) {
	//int queueNum;

	// As the entry lock is acquired, no other task can be manipulating this node, so lock does not need to be aquired
	// yet.

	//if ( uOwner == &(node->task()) ) {
	//queueNum = getBaseQueue( node->task() );	// use the node for your base priority
	//} else {
	//queueNum = getActiveQueue( getInheritTask(node->task() ) ); // use the node for your active priority
	//} // if

	lock.acquire();

	// do remove
	objects[queueNum].count -= 1;

	// if empty then must remove from heap
	if ( objects[queueNum].count == 0 ) {
		heap.deletenode( heap.A[objects[queueNum].index] );
	} // if

	lock.release();
} // uPIHeap::remove


// Local Variables: //
// compile-command: "make install" //
// End: //
