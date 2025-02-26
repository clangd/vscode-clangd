//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Philipp E. Lim and Ashif S. Harji 1995, 1997
// 
// uAlarm.cc -- 
// 
// Author           : Philipp E. Lim
// Created On       : Thu Jan  4 17:34:00 1996
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri May 12 07:02:56 2023
// Update Count     : 325
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


// Posix manual, 2.4.1 Signal Generation and Delivery
//
// During the time between the generation of a signal and its delivery or acceptance, the signal is said to be
// "pending". Ordinarily, this interval cannot be detected by an application. However, a signal can be "blocked" from
// delivery to a thread. If the action associated with a blocked signal is anything other than to ignore the signal, and
// if that signal is generated for the thread, the signal shall remain pending until it is unblocked, it is accepted
// when it is selected and returned by a call to the sigwait( ) function, or the action associated with it is set to
// ignore the signal.  Signals generated for the process shall be delivered to exactly one of those threads within the
// process which is in a call to a sigwait( ) function selecting that signal or has not blocked delivery of the
// signal. If there are no threads in a call to a sigwait( ) function selecting that signal, and if all threads within
// the process block delivery of the signal, the signal shall remain pending on the process until a thread calls a
// sigwait( ) function selecting that signal, a thread unblocks delivery of the signal, or the action associated with
// the signal is set to ignore the signal. If the action associated with a blocked signal is to ignore the signal and if
// that signal is generated for the process, it is unspecified whether the signal is discarded immediately upon
// generation or remains pending.


// Events are only processed in uKernelModule::rollForward and rollForward is only called when preemption is turned off,
// so safe to make direct accesses through TLS pointer.

#define __U_KERNEL__
#include <uC++.h>
#include <unistd.h>										// access: getpid
//#include <uDebug.h>


using namespace UPP;


//######################### uEventNode #########################


void uEventNode::createEventNode( uBaseTask *task, uSignalHandler *sig, uTime alarm, uDuration period ) {
#ifdef __U_STATISTICS__
	uFetchAdd( Statistics::events, 1 );
#endif // __U_STATISTICS__
	uEventNode::alarm = alarm;
	uEventNode::period = period;
	uEventNode::task = task;
	sigHandler = sig;
	executeLocked = false;
} // uEventNode::createEventNode


uEventNode::uEventNode( uBaseTask &task, uSignalHandler &sig, uTime alarm, uDuration period ) {
	createEventNode( &task, &sig, alarm, period );
} // uEventNode::uEventNode


uEventNode::uEventNode( uSignalHandler &sig ) {
	createEventNode( nullptr, &sig, uTime(), 0 );
} // uEventNode::uEventNode


uEventNode::uEventNode() {
	createEventNode( nullptr, nullptr, uTime(), 0 );
} // uEventNode::uEventNode


void uEventNode::add( bool block ) {
	uDEBUGPRT(
		char buf[1024];
		uDebugPrtBuf( buf, "(uEventNode &)%p.add( %d ) alarm:%lld period:%lld\n", this, block, alarm.nanoseconds(), period.nanoseconds() );
		);
	uProcessor::events->addEvent( *this, block );
} // uEventNode::add


void uEventNode::remove() {
	uDEBUGPRT(
		char buf[1024];
		uDebugPrtBuf( buf, "(uEventNode &)%p.remove alarm:%lld period:%lld\n", this, alarm.nanoseconds(), period.nanoseconds() );
		);
	uProcessor::events->removeEvent( *this );
} // uEventNode::remove


//######################### uEventList #########################


void uEventList::addEvent( uEventNode &newEvent, bool block ) {
	uDEBUGPRT(
		char buf[1024];
		uDebugPrtBuf( buf, "(uEventList &)%p.addEvent, newEvent:%p\n", this, &newEvent );
		);
	eventLock.acquire();

	uEventNode *event;
	for ( uSeqIter<uEventNode> iter( eventlist ); iter >> event && event->alarm <= newEvent.alarm; ); // find position to insert
	eventlist.insertBef( &newEvent, event );			// insert in list
	if ( eventlist.head() == &newEvent ) {				// inserted at front ?
		setTimer( newEvent.alarm );						// reset alarm
	} // if

	if ( block ) {
		uProcessorKernel::schedule( &eventLock );		// used by sleep
	} else {
		eventLock.release();
	} // if
} // uEventList::addEvent


void uEventList::removeEvent( uEventNode &event ) {
	uDEBUGPRT(
		char buf[1024];
		uDebugPrtBuf( buf, "(uEventList &)%p.removeEvent, event:%p\n", this, &event );
		)
		eventLock.acquire();

	// If a task is trying to remove an event at the same time the event expires, both the task and roll forward race to
	// remove the event.  One succeeds and the other finds the node not listed.
	if ( ! event.listed() ) {							// node already removed ?
		eventLock.release();
		return;
	} // if

	uEventNode *head = eventlist.head();
	eventlist.remove( &event );

	if ( head == &event ) {								// remove at head ? => reset alarm
		if ( eventlist.empty() ) {						// list empty ?
			setTimer( uDuration( 0 ) );					// cancel alarm
		} else {
			setTimer( eventlist.head()->alarm );		// reset alarm
		} // if
	} // if

	eventLock.release();
} // uEventList::removeEvent


#if ! defined( __U_MULTI__ )
bool uEventList::userEventPresent() {
	eventLock.acquire();

	uEventNode *event;
	uSeqIter<uEventNode> iter(eventlist);

	// Only one context-switch event in uniprocessor as there is only one real processor and the other processors are
	// simulated. Now check for any task waiting other than system task. Loop executes at most 3 iterations
	for ( int i = 0; iter >> event && ( uProcessor::contextSwitchHandler == event->sigHandler // ignore context switch event
										|| event->task == (uBaseTask *)uKernelModule::systemTask ); // ignore system task
		  i += 1
		) {
		assert( i < 3 );
	} // for
	eventLock.release();

	return event != nullptr;
} // uEventList::userEventPresent
#endif // ! __U_MULTI__


void uEventList::setTimer( uDuration duration ) {
	uDEBUGPRT(
		char buf[1024];
		uDebugPrtBuf( buf, "(uEventList &)%p.setTimer, duration %lld\n", this, duration.nanoseconds() );
		);
	activeProcessorKernel->setTimer( duration );
} // uEventList::setTimer


void uEventList::setTimer( uTime time ) {				// time parameter is real-time (not virtual time)
	uDEBUGPRT(
		char buffer[256];
		uDebugPrtBuf( buffer, "uEventList::setTimer, time:%lld\n", time.nanoseconds() );
		);
  if ( time == uTime() ) return;						// zero time is invalid

	uDuration dur = time - uClock::currTime();

	if ( dur <= 0 ) {					// if duration is zero or negative (it has already past)
		uDEBUGPRT( uDebugPrtBuf( buffer, "uEventList::setTimer, kill time:%lld currtime:%lld dur:%lld\n",
								 time.nanoseconds(), currtime.nanoseconds(), dur.nanoseconds() ); );
		//kill( getpid(), SIGALRM );						// send SIGALRM immediately, works uni and multi processor
		uKernelModule::uKernelModuleBoot.RFpending = true; // force rollForward to be called
	} else {
		setTimer( dur );
	} // if
} // uEventList::setTimer


//######################### uEventListPop #########################


void uEventListPop::over( uEventList &events, bool inKernel ) {
	uEventListPop::events = &events;
	currTime = uClock::currTime();
	cxtSwHandler = nullptr;
	uEventListPop::inKernel = inKernel;
	assert( ! uKernelModule::uKernelModuleBoot.RFinprogress ); // should not be on
	uKernelModule::uKernelModuleBoot.RFinprogress = true; // starting roll forward
	uKernelModule::uKernelModuleBoot.RFpending = false;	// no pending roll forward
} // uEventListPop::over


uEventListPop::uEventListPop( uEventList &events, bool inKernel ) {
#if defined( __U_MULTI__ )
	assert( &uThisProcessor() == &uKernelModule::systemProcessor );
#endif // __U_MULTI__
	over( events, inKernel );
} // uEventListPop::uEventListPop


uEventListPop::~uEventListPop() {
	uDEBUGPRT(
		char buf[1024];
		uDebugPrtBuf( buf, "(uEventListPop &)%p.~uEventListPop\n", this );
		);
#if defined( __U_MULTI__ )
	assert( &uThisProcessor() == &uKernelModule::systemProcessor );
#endif // __U_MULTI__

	events->eventLock.acquire_( true );
	uEventNode *head = events->eventlist.head();		// optimization
	if ( head != nullptr && ! uKernelModule::uKernelModuleBoot.RFpending ) { // reset timer to next available event
		events->setTimer( head->alarm );
	} // if
	uKernelModule::uKernelModuleBoot.RFinprogress = false;
	events->eventLock.release();						// triggers new rollForward if RFpending

#if defined( __U_MULTI__ )
	// should work without locking
	uClusterDL *clus;
	for ( uSeqIter<uClusterDL> iter( wakeupList ); iter >> clus; ) {
		wakeupList.remove( clus );
		clus->cluster().processorPoke();
	} // for
#endif // __U_MULTI__

	if ( ! inKernel ) {									// not in kernel ?
#if defined( __U_MULTI__ )
		if ( cxtSwHandler )								// context-switch event ?
#endif // __U_MULTI__
			// No need to send SIGUSR1 to system processor via context-switch handler just do the context switch.
			uThisTask().uYieldInvoluntary();
	} else {
		// Roll forward is only called from the kernel while spinning before pausing and after waking up from pausing.
		// Therefore, yielding is unnecessary, if going to pause or have just paused.

		// A preemption event for the system processor may be ignored because a reschedule is just about to occur,
		// anyway, when the kernel reschedules the next task.
	} // if
} // uEventListPop::~uEventListPop


bool uEventListPop::operator>>( uEventNode *&node ) {
	uDEBUGPRT(
		char buf[1024];
		uDebugPrtBuf( buf, "(uEventListPop &)%p.>>, event list %p\n", this, events );
		);
	events->eventLock.acquire_( true );

	node = events->eventlist.head();					// get event at the start of the list with the shortest time delay

	if ( ! node ) {										// no events ?
		events->eventLock.release_( true );
		return false;
	} // if

	uDEBUGPRT( uDebugPrtBuf( buf, "(uEventListPop &)%p.>>, currTime:%lld node:%p %s alarm:%lld period:%lld\n",
							 this, currTime.nanoseconds(), node, node->task != nullptr ? node->task->getName() : "*noname*", node->alarm.nanoseconds(), node->period.nanoseconds() ); );

  if ( node->alarm > currTime ) {						// event time delay greater than the start time for the iteration
		events->eventLock.release_( true );
		return false;
	} // if

	events->eventlist.remove( node );

	// If the popped event is periodic, reinsert for next period.
	if ( node->period != 0 ) {
		node->alarm = currTime + node->period;			// reset time for next alarm

		uEventNode *event;
		// May have to order identical timed elements by priority (to keep up the real-time spirit)
		for ( uSeqIter<uEventNode> iter( events->eventlist ); iter >> event && event->alarm <= node->alarm; );
		events->eventlist.insertBef( node, event );		// insert into sorted position in list
	} // if

	uCxtSwtchHndlr * cxtSwEvent = dynamic_cast<uCxtSwtchHndlr *>(node->sigHandler);
	if ( cxtSwEvent != nullptr ) {						// ContextSwitch event ?
#if defined( __U_MULTI__ )
		if ( &cxtSwEvent->processor == &uKernelModule::systemProcessor ) {
#endif // ! __U_MULTI__
			// Defer ContextSwitch for system processor until after all events processed so SIGUSR1 not delivered during
			// event processing.
			assert( cxtSwHandler == nullptr );
			cxtSwHandler = node->sigHandler;
			events->eventLock.release_( true );
#if defined( __U_MULTI__ )
		} else {
			uEventNode temp = *node;					// copy node as processor can terminate and delete cxt-sw event
			events->eventLock.release_( true );
			temp.sigHandler->handler();					// send SIGUSR1 to processor
		} // if
#endif // __U_MULTI__
	} else {
		// Force a reschedule on this cluster as a higher-priority task may have become ready and is now at the front of
		// the ready queue. Unfortunately, this only deals with lower-priority tasks that are waiting and NOT
		// lower-priority tasks execuing on processors in the cluster.  As a side effect, if the current task has the
		// same priority as the task on the front of the ready queue, the current task is preempted even if it has only
		// just started execution.
#if defined( __U_MULTI__ )
		if ( dynamic_cast<uWakeupHndlr *>(node->sigHandler) != nullptr ) { // uWakeupHndlr event ?
			assert( node->task != nullptr );
			if ( ! node->task->currCluster_->wakeupList.listed() ) { // already on list ?
				wakeupList.addTail( &(node->task->currCluster_->wakeupList) ); // defer until destructor
			} // if
		} // if
#endif // __U_MULTI__

		if ( node->executeLocked ) {					// should handler be called with lock
			node->sigHandler->handler();				// invoke the handler code for the event
			events->eventLock.release_( true );
		} else {
			events->eventLock.release_( true );
			node->sigHandler->handler();				// invoke the handler code for the event
		} // if
	} // if

	return true;
} // uEventListPop::operator>>


// Local Variables: //
// compile-command: "make install" //
// End: //
