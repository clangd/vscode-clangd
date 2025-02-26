//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Philipp E. Lim 1996
//
// uRealTime.h --
//
// Author           : Philipp E. Lim
// Created On       : Fri Jul 19 16:34:59 1996
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr  9 17:09:49 2022
// Update Count     : 147
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
#include <cstring>										// access: ffs


// provide default definition for real-time monitor
#define uRealTimeMonitor _Mutex<uPrioritySeq,uPrioritySeq> class

// Faked for now, since Events aren't implemented yet.
class uEvent {
  public:
	uEvent() {};
};


//######################### Priority Scheduling #########################


#define __U_MAX_NUMBER_PRIORITIES__ 32


template<typename List, typename Node> class uPriorityScheduleQueue : public uBaseSchedule<Node> {
  protected:
	using uBaseSchedule<Node>::getActiveQueueValue;
	using uBaseSchedule<Node>::getActivePriority;

	List objects[__U_MAX_NUMBER_PRIORITIES__];
	unsigned int mask;									// allow access to all queue flags
	unsigned int verCount;
  public:
	uPriorityScheduleQueue() {
		verCount = mask = 0;
	} // uPriorityScheduleQueue::uPriorityScheduleQueue

	virtual bool empty() const {
		return mask == 0;
	} // uPriorityScheduleQueue::empty

	virtual Node *head() const {
		int highestPriority = ffs( mask ) - 1;

		if ( highestPriority >= 0 ) {
			Node *node = objects[highestPriority].head();
			return node;
		} else {
			return nullptr;
		} // if
	} // uPriorityScheduleQueue::head

	virtual void add( Node *node ) {
		int priority = getActivePriority( node->task() );
#ifdef __U_DEBUG__
		assert( 0 <= priority && priority <= __U_MAX_NUMBER_PRIORITIES__ - 1 );
#endif // __U_DEBUG__
		objects[priority].add( node );
		mask |= 1ul << priority;
		uDEBUGPRT( uDebugPrt( "(uPriorityScheduleQueue &)%p.add( %p ) task %.256s (%p) adding   task %.256s (%p) with priority %d on cluster %p\n",
							  this, node, uThisTask().getName(), &uThisTask(), node->task().getName(), &node->task(), priority, &uThisCluster() ); );
	} // uPriorityScheduleQueue::add

	virtual Node *drop() {
		int highestPriority = ffs( mask ) - 1;

		if ( highestPriority >= 0 ) {
			Node *node = objects[highestPriority].drop();
			if ( objects[highestPriority].empty() ) {
				mask &= ~ ( 1ul << highestPriority );
			} // if
			uDEBUGPRT( uDebugPrt( "(uPriorityScheduleQueue &)%p.drop( %p ) task %.256s (%p) removing task %.256s (%p) with priority %d on cluster %s (%p)\n",
								  this, node, uThisTask().getName(), &uThisTask(), node->task().getName(), &node->task(), highestPriority, uThisCluster().getName(), &uThisCluster() ); );
			return node;
		} else {
			return nullptr;
		} // if
	} // uPriorityScheduleQueue::drop

	virtual void transfer( uBaseTaskSeq & /* from */ ) {
		abort( "uPriorityScheduleQueue::transfer, internal error, unsupported operation" );
	} // uPriorityScheduleQueue::transfer

	virtual bool checkPriority( Node &, Node & ) {
		return false;
	} // uPriorityScheduleQueue::checkPriority

	virtual void resetPriority( Node &, Node & ) {
	} // uPriorityScheduleQueue::resetPriority

	virtual void addInitialize( uBaseTaskSeq & ) {
	} // uPriorityScheduleQueue::addInitialize

	virtual void removeInitialize( uBaseTaskSeq & ) {
	} // uPriorityScheduleQueue::removeInitialize

	virtual void rescheduleTask( uBaseTaskDL *, uBaseTaskSeq & ) {
	} // uPriorityScheduleQueue::rescheduleTask
}; // uPriorityScheduleQueue


class uPrioritySeq : public uBasePrioritySeq {
  protected:
	using uBasePrioritySeq::setActivePriority;

	uBaseTaskSeq objects[__U_MAX_NUMBER_PRIORITIES__];
	unsigned int mask;									// allow access to all queue flags
	uBaseTask *oldInheritTask;

	int afterEntry( uBaseTask *owner ) {
		if ( ! empty() ) {
			uThisCluster().taskResetPriority( *owner, head()->task() );
		} // if
		return 0;
	} // uPrioritySeq::afterEntry
  public:
	uPrioritySeq() {
		mask = 0;
		executeHooks = true;
	} // uPrioritySeq::uPriorityScheduleQueue

	bool empty() const {
		return mask == 0;
	} // uPrioritySeq::empty

	uBaseTaskDL *head() const {
		int highestPriority = ffs( mask ) - 1;

		if ( highestPriority >= 0 ) {
			uBaseTaskDL *node = objects[highestPriority].head();
			return node;
		} else {
			return nullptr;
		} // if
	} // uPrioritySeq::head

	int add( uBaseTaskDL *node, uBaseTask *owner ) {
		int priority = getActivePriority( node->task() );
		assert( 0 <= priority && priority <= __U_MAX_NUMBER_PRIORITIES__ - 1 );
		objects[priority].add( node );
		mask |= 1ul << priority;
		// only perform inheritance for entry list
		if ( isEntryBlocked( node->task() ) && checkHookConditions( *owner, node->task() ) ) { // TEMP: entry queue??
			return( afterEntry( owner ) );		// perform any priority inheritance
		} else {
			return 0;
		} // if
	} // uPrioritySeq::add

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
	} // uPrioritySeq::drop

	void remove( uBaseTaskDL *node ) {
		int priority = getActivePriority( node->task() );
		objects[priority].remove( node );
		if ( objects[priority].empty() ) {
			mask &= ~ ( 1ul << priority );
		} // if
	} // uPrioritySeq::remove

	void transfer( uBaseTaskSeq & /* from */ ) {
		abort( "uPrioritySeq::transfer, internal error, unsupported operation" );
	} // uPrioritySeq::transfer

	void onAcquire( uBaseTask &owner ) {
		oldInheritTask = &getInheritTask( owner );
		afterEntry( &owner );
	} // uPrioritySeq::onAcquire

	void onRelease( uBaseTask &owner ) {
		// no queue adjustment as task is running
		setActivePriority( owner, *oldInheritTask );
	} // uPrioritySeq::onRelease
}; // uPrioritySeq


template<typename List, typename Node> class uPriorityScheduleSeq : public uPriorityScheduleQueue<List, Node> {
  protected:
	using uPriorityScheduleQueue<List, Node>::objects;
	using uPriorityScheduleQueue<List, Node>::mask;
	using uPriorityScheduleQueue<List, Node>::getActivePriority;
	using uPriorityScheduleQueue<List, Node>::setActivePriority;
	using uPriorityScheduleQueue<List, Node>::add;
  public:
	virtual bool checkPriority( Node &owner, Node &calling ) {
		return getActivePriority( owner.task() ) > getActivePriority( calling.task() );
	} // uPriorityScheduleSeq::checkPriority

	virtual void resetPriority( Node &owner, Node &calling ) {
		if ( owner.listed() ) {
			remove( &owner );
			setActivePriority( owner.task(), calling.task() );
			add( &owner );
		} else {
			setActivePriority( owner.task(), calling.task() );
		} // if
	} // uPriorityScheduleSeq::resetPriority

	virtual void remove( Node *node ) {
		int priority = getActivePriority( node->task() );
		objects[priority].remove( node );
		if ( objects[priority].empty() ) {
			mask &= ~ ( 1ul << priority );
		} // if
	} // uPriorityScheduleSeq::remove

	virtual void transfer( uBaseTaskSeq & /* from */ ) {
		abort( "uPriorityScheduleSeq::transfer, internal error, unsupported operation" );
	} // uPriorityScheduleSeq::transfer
}; // uPriorityScheduleSeq


//######################### uRealTimeBaseTask #########################


class uRealTimeBaseTask : public uBaseTask {
	class VerCount : public uSeqable {
	  public:
		int version;
		uCluster *cluster;
	}; // VerCount

	uDuration deadline;
	uSequence<VerCount> verCountSeq;					// list of scheduler version counts with associated cluster
  protected:
	uTime firstActivateTime;
	uEvent firstActivateEvent;
	uTime endTime;
  public:
	uRealTimeBaseTask( uCluster &cluster = uThisCluster() );
	uRealTimeBaseTask( uTime firstActivateTask_, uTime endTime_, uDuration deadline_, uCluster &cluster = uThisCluster() );
	uRealTimeBaseTask( uEvent firstActivateEvent_, uTime endTime_, uDuration deadline_, uCluster &cluster = uThisCluster() );
	uRealTimeBaseTask( uTime firstActivateTask_, uEvent firstActivateEvent_, uTime endTime_, uDuration deadline_, uCluster &cluster = uThisCluster() );
	~uRealTimeBaseTask();
	virtual uDuration getDeadline() const;
	virtual uDuration setDeadline( uDuration deadline_ );

	// These members should be private but cannot be because they are
	// referenced from user code.

	virtual int getVersion( uCluster &cluster );
	virtual int setVersion( uCluster &cluster, int version );
}; // uRealTimeBaseTask


//######################### uPeriodicBaseTask #########################


class uPeriodicBaseTask : public uRealTimeBaseTask {
  protected:
	uDuration period;
  public:
	uPeriodicBaseTask( uDuration period_, uCluster &cluster = uThisCluster() );
	uPeriodicBaseTask( uDuration period_, uTime firstActivateTask_, uTime endTime_, uDuration deadline_, uCluster &cluster = uThisCluster() );
	uPeriodicBaseTask( uDuration period_, uEvent firstActivateEvent_, uTime endTime_, uDuration deadline_, uCluster &cluster = uThisCluster() );
	uPeriodicBaseTask( uDuration period_, uTime firstActivateTask_, uEvent firstActivateEvent_, uTime endTime_, uDuration deadline_, uCluster &cluster = uThisCluster() );
	uDuration getPeriod() const;
	uDuration setPeriod( uDuration period_ );
}; // uPeriodicBaseTask


//######################### uSporadicBaseTask #########################


class uSporadicBaseTask : public uRealTimeBaseTask {
  protected:
	uDuration frame;
  public:
	uSporadicBaseTask( uDuration frame_, uCluster &cluster = uThisCluster() );
	uSporadicBaseTask( uDuration frame_, uTime firstActivateTask_, uTime endTime_, uDuration deadline_, uCluster &cluster = uThisCluster() );
	uSporadicBaseTask( uDuration frame_, uEvent firstActivateEvent_, uTime endTime_, uDuration deadline_, uCluster &cluster = uThisCluster() );
	uSporadicBaseTask( uDuration frame_, uTime firstActivateTask_, uEvent firstActivateEvent_, uTime endTime_, uDuration deadline_, uCluster &cluster = uThisCluster() );
	uDuration getFrame() const;
	uDuration setFrame( uDuration frame_ );
}; // uSporadicBaseTask


//######################### uRealTimeCluster #########################


class uRealTimeCluster : public uCluster {
  public:
	uRealTimeCluster( uBaseSchedule<uBaseTaskDL> &rq, int size = uDefaultStackSize(), const char *name = "uRealTimeCluster" );
	uRealTimeCluster( uBaseSchedule<uBaseTaskDL> &rq, const char *name );
	~uRealTimeCluster();
}; // uRealTimeCluster


// Local Variables: //
// compile-command: "make install" //
// End: //
