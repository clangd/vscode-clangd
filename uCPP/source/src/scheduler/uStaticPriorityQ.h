//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Ashif S. Harji 2000
//
// uStaticPriorityQ.h --
//
// Author           : Ashif S. Harji
// Created On       : Fri Jan 14 17:59:34 2000
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr  9 17:16:04 2022
// Update Count     : 108
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
#include <uStaticPIQ.h>

#include <cstring>										// access: ffs


#define __U_MAX_NUMBER_PRIORITIES__ 32


class uStaticPriorityQ : public uBasePrioritySeq {
  protected:
	uBaseTaskSeq objects[__U_MAX_NUMBER_PRIORITIES__];
	unsigned int mask;									// allow access to all queue flags

	int currPriority;

	int afterEntry( uBaseTask *owner );
  public:
	uStaticPriorityQ() {
		mask = 0;
		executeHooks = true;
		currPriority = -1;
	} // uStaticPriorityQ::uStaticPriorityQ

	bool empty() const {
		return mask == 0;
	} // uStaticPriorityQ::empty

	uBaseTaskDL *head() const {
		int highestPriority = ffs( mask ) - 1;

		if ( highestPriority >= 0 ) {
			uBaseTaskDL *node = objects[highestPriority].head();
			return node;
		} else {
			return nullptr;
		} // if
	} // uStaticPriorityQ::head

	virtual int add( uBaseTaskDL *node, uBaseTask *owner );

	uBaseTaskDL *drop() {
		int highestPriority = ffs( mask ) - 1;

		if ( highestPriority >= 0 ) {
			uBaseTaskDL *node = objects[highestPriority].drop();
			if ( objects[highestPriority].empty() ) {
				mask &= ~ ( 1ul << highestPriority );
			} // if
			return node;
		} else {
			return nullptr;
		} // if
	} // uStaticPriorityQ::drop

	void remove( uBaseTaskDL *node ) {
		// use stored priority value because this task has entry lock, so its uPIQ may be updated, but not its position
		// on the entry queue.
		int priority = getActivePriorityValue( node->task() );

		objects[priority].remove( node );
		if ( objects[priority].empty() ) {
			mask &= ~ ( 1ul << priority );
		} // if
	} // uStaticPriorityQ::remove

	virtual void transfer( uBaseTaskSeq & /* from */ ) {
		abort( "uStaticPriorityQ::transfer, internal error, unsupported operation" );
	} // uStaticPriorityQ

	virtual void onAcquire( uBaseTask &oldOwner );
	virtual void onRelease( uBaseTask &owner );
}; // StaticPriorityQ


template<typename List, typename Node> class uStaticPriorityScheduleQ : public uBaseSchedule<Node> {
  protected:
	using uBaseSchedule<Node>::getActivePriorityValue;

	List objects[__U_MAX_NUMBER_PRIORITIES__];
	unsigned int mask;					// allow access to all queue flags
	unsigned int verCount;
  public:
	uStaticPriorityScheduleQ() {
		verCount = mask = 0;
	} // uStaticPriorityScheduleQ::uStaticPriorityScheduleQ

	virtual bool empty() const {
		return mask == 0;
	} // uStaticPriorityScheduleQ::empty

	virtual Node *head() const {
		int highestPriority = ffs( mask ) - 1;

		if ( highestPriority >= 0 ) {
			Node *node = objects[highestPriority].head();
			return node;
		} else {
			return nullptr;
		} // if
	} // uStaticPriorityScheduleQ::head

	virtual void add( Node *node ) {
		int priority = getActivePriorityValue( node->task() );
#ifdef __U_DEBUG__
		assert( 0 <= priority && priority <= __U_MAX_NUMBER_PRIORITIES__ - 1 );
#endif // __U_DEBUG__
		objects[priority].add( node );
		mask |= 1ul << priority;
		uDEBUGPRT( uDebugPrt( "(uStaticPriorityScheduleQ &)%p.add( %p ) task %.256s (%p) adding task %.256s (%p) with priority %d on cluster %p\n",
							  this, node, uThisTask().getName(), &uThisTask(), node->task().getName(), &node->task(), priority, &uThisCluster() ); );
	} // uStaticPriorityScheduleQ::add

	virtual Node *drop() {
		int highestPriority = ffs( mask ) - 1;

		if ( highestPriority >= 0 ) {
			Node *node = objects[highestPriority].drop();
			if ( objects[highestPriority].empty() ) {
				mask &= ~ ( 1ul << highestPriority );
			} // if
			uDEBUGPRT( uDebugPrt( "(uStaticPriorityScheduleQ &)%p.drop( %p ) task %.256s (%p) removing task %.256s (%p) with priority %d on cluster %p\n",
								  this, node, uThisTask().getName(), &uThisTask(), node->task().getName(), &node->task(), highestPriority, &uThisCluster() ); );
			return node;
		} else {
			return nullptr;
		} // if
	} // uStaticPriorityScheduleQ::drop

	virtual bool checkPriority( Node &, Node & ) {
		return false;
	} // uStaticPriorityScheduleQ::checkPriority

	virtual void resetPriority( Node &, Node & ) {
	} // uStaticPriorityScheduleQ::resetPriority

	virtual void addInitialize( uBaseTaskSeq & ) {
	} // uStaticPriorityScheduleQ::addInitialize

	virtual void removeInitialize( uBaseTaskSeq & ) {
	} // uStaticPriorityScheduleQ::removeInitialize

	virtual void rescheduleTask( uBaseTaskDL *, uBaseTaskSeq & ) {
	} // uStaticPriorityScheduleQ::rescheduleTask
}; // uStaticPriorityScheduleQ


template<typename List, typename Node> class uStaticPriorityScheduleSeq : public uStaticPriorityScheduleQ<List, Node> {
  protected:
	using uStaticPriorityScheduleQ<List, Node>::objects;
	using uStaticPriorityScheduleQ<List, Node>::mask;
	using uStaticPriorityScheduleQ<List, Node>::getActivePriorityValue;
	using uStaticPriorityScheduleQ<List, Node>::setActivePriority;
	using uStaticPriorityScheduleQ<List, Node>::add;
  public:
	virtual bool checkPriority( Node &owner, Node &calling ) {
		return getActivePriorityValue( owner.task() ) > getActivePriorityValue( calling.task() );
	} // uStaticPriorityScheduleSeq::checkPriority

	virtual void resetPriority( Node &owner, Node &calling ) {
		int priority;
		uBaseTask &ownerTask = owner.task();
		uBaseTask &callingTask = calling.task();
		// if same, update owner based on uPIQ
		if ( &ownerTask == &callingTask ) {
			priority = (static_cast<uStaticPIQ *>(ownerTask.uPIQ))->getHighestPriority(); // TODO: dynamic needed???
			// if no inheritance priority use base priority
			if ( priority == -1 ) {
				priority = ownerTask.getBasePriority();
			} // if
		} else {  // otherwise, update to atmost calling task's priority
			if ( callingTask.getActivePriorityValue() > ownerTask.getActivePriorityValue() ) return;
			priority = callingTask.getActivePriorityValue();
		} // if

		if ( owner.listed() ) {
			remove( &owner );
			setActivePriority( ownerTask, priority );
			add( &owner );
		} else {
			setActivePriority( ownerTask, priority );
		} // if
	} // uStaticPriorityScheduleSeq::resetPriority

	virtual void remove( Node *node ) {
		int priority = getActivePriorityValue( node->task() );
		objects[priority].remove( node );
		if ( objects[priority].empty() ) {
			mask &= ~ ( 1ul << priority );
		} // if
	} // uStaticPriorityScheduleSeq::remove

	virtual void transfer( uBaseTaskSeq & /* from */ ) {
		abort( "uStaticPriorityScheduleSeq::transfer, internal error, unsupported operation" );
	} // uStaticPriorityScheduleSeq
}; // uStaticPriorityScheduleSeq


// Local Variables: //
// compile-command: "make install" //
// End: //
