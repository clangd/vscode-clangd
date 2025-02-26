//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1999
//
// uRealTime.cc --
//
// Author           : Peter A. Buhr
// Created On       : Mon Feb  1 15:06:12 1999
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr  9 17:08:02 2022
// Update Count     : 86
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
#include <uRealTime.h>
//#include <uDebug.h>


//######################### uRealTimeBaseTask #########################


uRealTimeBaseTask::uRealTimeBaseTask( uCluster &cluster ) : uBaseTask( cluster ) {};

uRealTimeBaseTask::uRealTimeBaseTask( uTime firstActivateTask_, uTime endTime_, uDuration deadline_, uCluster &cluster ) : uBaseTask( cluster ) {
	if ( deadline_ < 0 ) {
		abort( "Attempt to create real time task with deadline less than 0." );
	} // if
	firstActivateTime = firstActivateTask_;
	endTime = endTime_;
	deadline = deadline_;
} // uRealTimeBaseTask::uRealTimeBaseTask

uRealTimeBaseTask::uRealTimeBaseTask( uEvent firstActivateEvent_, uTime endTime_, uDuration deadline_, uCluster &cluster ) : uBaseTask( cluster ) {
	if ( deadline_ < 0 ) {
		abort( "Attempt to create real time task with deadline less than 0." );
	} // if
	firstActivateEvent = firstActivateEvent_;
	firstActivateTime = uTime();
	endTime = endTime_;
	deadline = deadline_;
} // uRealTimeBaseTask::uRealTimeBaseTask

uRealTimeBaseTask::uRealTimeBaseTask( uTime firstActivateTask_, uEvent firstActivateEvent_, uTime endTime_, uDuration deadline_, uCluster &cluster ) : uBaseTask( cluster ) {
	if ( deadline_ < 0 ) {
		abort( "Attempt to create real time task with deadline less than 0." );
	} // if
	firstActivateTime = firstActivateTask_;
	firstActivateEvent = firstActivateEvent_;
	endTime = endTime_;
	deadline = deadline_;
} // uRealTimeBaseTask::uRealTimeBaseTask

uRealTimeBaseTask::~uRealTimeBaseTask() {
	while ( ! verCountSeq.empty() ) {					// remove list of version counts
		delete verCountSeq.dropHead();
	} // while
} // uRealTimeBaseTask::uRealTimeBaseTask

uDuration uRealTimeBaseTask::getDeadline() const {
	return deadline;
} // uRealTimeBaseTask::getDeadline

uDuration uRealTimeBaseTask::setDeadline( uDuration deadline_ ) {
#ifdef __U_DEBUG__
	if ( this != &uThisTask() ) {
		abort( "Attempt to change the deadline of task %.256s (%p).\n"
			   "A task can only change its own deadline.\n",
			   getName(), this );
	} // if
#endif // __U_DEBUG__

	// A simple optimization: changing the deadline of a task to its existing
	// value does not require a recalculation of priorities.

	if ( deadline_ == deadline ) return deadline;

	uDuration temp = deadline;
	deadline = deadline_;
	uThisCluster().taskReschedule( *this );
	yield();
	return temp;
} // uRealTimeBaseTask::setDeadline

int uRealTimeBaseTask::getVersion( uCluster &cluster ) {
	uSeqIter<VerCount> iter;
	VerCount *node = nullptr;

	for ( iter.over( verCountSeq ); iter >> node; ) {
		if ( node->cluster == &cluster ) {
			return node->version;
		} // if
	} // for

	return -1;
} // uRealTimeBaseTask::getVersion

int uRealTimeBaseTask::setVersion( uCluster &cluster, int version ) {
	uSeqIter<VerCount> iter;
	VerCount *ref = nullptr, *node = nullptr, *prev = nullptr;
	int temp;

	for ( iter.over(verCountSeq), prev = nullptr; iter >> node ; prev = node ) { // find place in the list to insert
	   if ( &cluster < node->cluster ) break;
	} // for

	if ( prev != nullptr && prev->cluster == &cluster ) {
		temp = prev->version;
		prev->version = version;
	} else {
		temp = -1;
		ref = new VerCount;
		ref->cluster = &cluster;
		ref->version = version;
		verCountSeq.insertBef( ref, node );
	} // if

	return temp;
} // uRealTimeBaseTask::setVersion


//######################### uPeriodicBaseTask #########################


uPeriodicBaseTask::uPeriodicBaseTask( uDuration period_, uCluster &cluster ) : uRealTimeBaseTask( uTime(), uTime(), uDuration(), cluster ) {
	period = period_;
} // uPeriodicBaseTask::uPeriodicBaseTask

uPeriodicBaseTask::uPeriodicBaseTask( uDuration period_, uTime firstActivateTask_, uTime endTime_, uDuration deadline_, uCluster &cluster ) : uRealTimeBaseTask( firstActivateTask_, endTime_, deadline_, cluster ) {
	period = period_;
} // uPeriodicBaseTask::uPeriodicBaseTask

uPeriodicBaseTask::uPeriodicBaseTask( uDuration period_, uEvent firstActivateEvent_, uTime endTime_, uDuration deadline_, uCluster &cluster ) : uRealTimeBaseTask( firstActivateEvent_, endTime_, deadline_, cluster ) {
	period = period_;
} // uPeriodicBaseTask::uPeriodicBaseTask

uPeriodicBaseTask::uPeriodicBaseTask( uDuration period_, uTime firstActivateTask_, uEvent firstActivateEvent_, uTime endTime_, uDuration deadline_, uCluster &cluster ) : uRealTimeBaseTask( firstActivateTask_, firstActivateEvent_, endTime_, deadline_, cluster ) {
	period = period_;
} // uPeriodicBaseTask::uPeriodicBaseTask

uDuration uPeriodicBaseTask::getPeriod() const {
	return period;
} // uPeriodicBaseTask::getPeriod

uDuration uPeriodicBaseTask::setPeriod( uDuration period_ ) {
#ifdef __U_DEBUG__
	if ( this != &uThisTask() ) {
		abort( "Attempt to change the period of task %.256s (%p).\n"
			   "A task can only change its own period.\n",
			   getName(), this );
	} // if
#endif // __U_DEBUG__

	// A simple optimization: changing the period of a task to its existing value does not require a recalculation of
	// priorities.

	if ( period_ == period ) return period;

	uDuration temp = period;
	period = period_;
	uThisCluster().taskReschedule( *this );
	yield();
	return temp;
} // uPeriodicBaseTask::setPeriod


//######################### uSporadicBaseTask #########################


uSporadicBaseTask::uSporadicBaseTask( uDuration frame_, uCluster &cluster ) : uRealTimeBaseTask( uTime(), uTime(), uDuration(), cluster ) {
	frame = frame_;
} // uSporadicBaseTask::uSporadicBaseTask

uSporadicBaseTask::uSporadicBaseTask( uDuration frame_, uTime firstActivateTask_, uTime endTime_, uDuration deadline_, uCluster &cluster ) : uRealTimeBaseTask( firstActivateTask_, endTime_, deadline_, cluster ) {
	frame = frame_;
} // uSporadicBaseTask::uSporadicBaseTask

uSporadicBaseTask::uSporadicBaseTask( uDuration frame_, uEvent firstActivateEvent_, uTime endTime_, uDuration deadline_, uCluster &cluster ) : uRealTimeBaseTask( firstActivateEvent_, endTime_, deadline_, cluster ) {
	frame = frame_;
} // uSporadicBaseTask::uSporadicBaseTask

uSporadicBaseTask::uSporadicBaseTask( uDuration frame_, uTime firstActivateTask_, uEvent firstActivateEvent_, uTime endTime_, uDuration deadline_, uCluster &cluster ) : uRealTimeBaseTask( firstActivateTask_, firstActivateEvent_, endTime_, deadline_, cluster ) {
	frame = frame_;
} // uSporadicBaseTask::uSporadicBaseTask

uDuration uSporadicBaseTask::getFrame() const {
	return frame;
} // uSporadicBaseTask::getFrame

uDuration uSporadicBaseTask::setFrame( uDuration frame_ ) {
#ifdef __U_DEBUG__
	if ( this != &uThisTask() ) {
		abort( "Attempt to change the frame of task %.256s (%p).\n"
			   "A task can only change its own frame.\n",
			   getName(), this );
	} // if
#endif // __U_DEBUG__

	// A simple optimization: changing the frame of a task to its existing value does not require a recalculation of
	// priorities.

	if ( frame_ == frame ) return frame;

	uDuration temp = frame;
	frame = frame_;
	uThisCluster().taskReschedule( *this );
	yield();
	return temp;
} // uSporadicBaseTask::setFrame


//######################### uRealTimeCluster #########################


uRealTimeCluster::uRealTimeCluster( uBaseSchedule<uBaseTaskDL> &rq, int size, const char *name ) : uCluster( rq, size, name ) {};
uRealTimeCluster::uRealTimeCluster( uBaseSchedule<uBaseTaskDL> &rq, const char *name ) : uCluster( rq, name ) {};
uRealTimeCluster::~uRealTimeCluster() {};


// Local Variables: //
// compile-command: "make install" //
// End: //
