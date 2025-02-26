//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Ashif S. Harji 2000
//
// uHeapQ.h --
//
// Author           : Ashif S. Harji
// Created On       : Fri Jan 14 17:59:34 2000
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr  9 17:05:39 2022
// Update Count     : 72
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
#include <uHeap.h>

#include <limits.h>


#define __U_MAX_NUMBER_PRIORITIES__ 32


class uPriorityQ : public uBasePrioritySeq {
	struct uHeapBaseSeq {
		int index;
		uBaseTaskSeq queue;
	};

	uHeapBaseSeq objects[__U_MAX_NUMBER_PRIORITIES__ ];
	uHeap<int, uHeapBaseSeq *, __U_MAX_NUMBER_PRIORITIES__> heap;

	int currPriority;
	int currQueueNum;

	static int compare(int k1, int k2);
	static void exchange( uHeapable<int, uHeapBaseSeq *> &x, uHeapable<int, uHeapBaseSeq *> &y );
  public:
	uPriorityQ();
	virtual bool empty() const;
	virtual uBaseTaskDL *head() const;
	virtual int add( uBaseTaskDL *node, uBaseTask *owner );
	virtual uBaseTaskDL *drop();
	virtual void remove( uBaseTaskDL *node );

	virtual void transfer( uBaseTaskSeq & /* from */ ) {
		abort( "uPriorityQ::transfer, internal error, unsupported operation" );
	} // uPriorityQ::transfer

	virtual void onAcquire(uBaseTask &uOwner );
	virtual void onRelease(uBaseTask &uOwner );

	int afterEntry( uBaseTask *owner );
}; // PriorityQ


template<typename List, typename Node> class uPriorityScheduleQ : public uBaseSchedule<Node> {
	struct uHeapScheduleSeq {
		int priority;
		int index;
		List queue;
	};

	// compare for Heapify property, ie parent(i) <= i
	static int compare( int k1, int k2 ) {
		if ( k1 < k2 ) {
			return 1;
		} else if ( k1 == k2 ){
			return 0;
		} else {
			return -1;
		}
	} // compare

	static void exchange( uHeapable<int, uHeapScheduleSeq *> &x, uHeapable<int, uHeapScheduleSeq *> &y ) {
		uHeapable<int, uHeapScheduleSeq *> temp = x;
		int t_index = x.data->index;
		x = y;
		y = temp;
		y.data->index = x.data->index;					// swapping locations requires the index pointers to be updated
		x.data->index = t_index;
#ifdef DEBUG_SHOW_COUNT
		NumExchg += 1;
#endif
	} // exchange
  protected:
	using uBaseSchedule<Node>::getActiveQueueValue;

	uHeapScheduleSeq objects[ __U_MAX_NUMBER_PRIORITIES__ ];
	uHeap<int, uHeapScheduleSeq *, __U_MAX_NUMBER_PRIORITIES__> heap;
	unsigned int verCount;
	int num_priorities;
  public:
	uPriorityScheduleQ() : heap( &compare, &exchange ) {
		verCount = 0;
		num_priorities = 1;								// first is always non-real-time tasks
		objects[0].priority = INT_MAX;					// use a large number, syn which addInitialize
	} // uPriorityScheduleQ::uPriorityScheduleQ

	virtual bool empty() const {
		return heap.size() == 0;
	} // uPriorityScheduleQ::empty

	virtual Node *head() const {
		if ( ! empty() ) {
			uHeapable<int, uHeapScheduleSeq *> rqueue;
			heap.getRoot( rqueue );
			return rqueue.data->queue.head();
		} else {
			return nullptr;
		} // if
	} // uPriorityScheduleQ::head

	virtual void add( Node *node ) {
		int queueNum = getActiveQueueValue( node->task() ); // use the node for you active priority

		// if empty then must add to heap, otherwise just insert node
		if ( objects[queueNum].queue.empty() ) {
			objects[queueNum].queue.add(node);
			// the index must be set before inserting because the index can change as part of the insert
			objects[queueNum].index = heap.size() + 1;
			heap.insert( objects[queueNum].priority, &(objects[queueNum]) );
		} else {
			objects[queueNum].queue.add(node);
		} // if
	} // uPriorityScheduleQ::add

	virtual Node *drop() {
		if ( ! empty() ) {
			uHeapable<int, uHeapScheduleSeq *> rqueue;
			heap.getRoot( rqueue );
			Node *pnode = rqueue.data->queue.drop();

			if ( rqueue.data->queue.empty() ){
				heap.deleteRoot();
			} // if
			return pnode;
		} else {
			return nullptr;
		} // if
	} // uPriorityScheduleQ::drop

	virtual bool checkPriority( Node &, Node & ) {
		return false;
	} // uPriorityScheduleQ::checkPriority

	virtual void resetPriority( Node &, Node & ) {
	} // uPriorityScheduleQ::resetPriority

	virtual void addInitialize( uBaseTaskSeq & ) {
	} // uPriorityScheduleQ::addInitialize

	virtual void removeInitialize( uBaseTaskSeq & ) {
	} // uPriorityScheduleQ::removeInitialize

	virtual void rescheduleTask( uBaseTaskDL *, uBaseTaskSeq & ) {
	} // uPriorityScheduleQ::rescheduleTask
}; // uPriorityScheduleQ


template<typename List, typename Node> class uPriorityScheduleQSeq : public uPriorityScheduleQ<List, Node> {
  protected:
	using uPriorityScheduleQ<List, Node>::objects;
	using uPriorityScheduleQ<List, Node>::setActivePriority;
	using uPriorityScheduleQ<List, Node>::setActiveQueue;
	using uPriorityScheduleQ<List, Node>::heap;
	using uPriorityScheduleQ<List, Node>::getActivePriorityValue;
	using uPriorityScheduleQ<List, Node>::getActiveQueueValue;
	using uPriorityScheduleQ<List, Node>::add;
  public:
	virtual bool checkPriority( Node &owner, Node &calling ) {
		return getActivePriorityValue( owner.task() ) > getActivePriorityValue( calling.task() );
	} // uPriorityScheduleQSeq::checkPriority

	virtual void resetPriority( Node &owner, Node &calling ) {
		int queueNum;
		uBaseTask &uOwner = owner.task();
		uBaseTask &uCalling = calling.task();
		// if same, update owner based on uPIQ
		if ( &uOwner == &uCalling ) {
			queueNum = (static_cast<uPIHeap *>(uOwner.uPIQ))->head(); // TODO: dynamic needed???
			// if queue is empty use base priority
			if ( queueNum == -1 ) {
				queueNum = uOwner.getBaseQueue();
			} // if
		} else {  // otherwise, update to atmost calling task's priority
			if ( uCalling.getActivePriorityValue() > uOwner.getActivePriorityValue() ) return;
			queueNum = uCalling.getActiveQueueValue();
		} // if

		if ( owner.listed() ) {
			remove( &owner );
			setActivePriority( uOwner, objects[queueNum].priority );
			setActiveQueue( uOwner, queueNum );
			add( &owner );
		} else {
			setActivePriority( uOwner, objects[queueNum].priority );
			setActiveQueue( uOwner, queueNum );
		} // if
	} // uPriorityScheduleQSeq::resetPriority

	virtual void remove( Node *node ) {
		int queueNum = getActiveQueueValue( node->task() ); // use the node for you active priority

		// do remove
		objects[queueNum].queue.remove(node);
		// if empty then must remove from heap
		if ( objects[queueNum].queue.empty() ) {
			heap.deletenode( heap.A[objects[queueNum].index] );
		}
	} // uPriorityScheduleQSeq::remove

	virtual void transfer( uBaseTaskSeq & /* from */ ) {
		abort( "uPriorityScheduleQSeq::transfer, internal error, unsupported operation" );
	} // uPriorityScheduleQSeq::transfer
}; // uPriorityScheduleQSeq


// Local Variables: //
// compile-command: "make install" //
// End: //
