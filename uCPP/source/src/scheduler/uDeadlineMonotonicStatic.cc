//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Ashif S. Harji 2000
//
// uDeadlineMonotonicStatic.cc --
//
// Author           : Ashif S. Harji
// Created On       : Fri Oct 27 16:09:42 2000
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr  9 17:15:34 2022
// Update Count     : 44
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
#include <uDeadlineMonotonicStatic.h>

//#include <uDebug.h>


// Compare abstracts the comparison of two task's priorities.  The deadline is first checked.  If the deadlines are
// identical for two tasks, the period or frame field is checked.  Non-real-time tasks are always greater in deadline
// than real-time tasks.  Aperiodic tasks get lowest deadline or priority among all real-time tasks.  This compare
// function acts in the same way as strcmp in terms of return value.

int uDeadlineMonotonicStatic::compare( uBaseTask &task1, uBaseTask &task2 ) {
	uDuration temp;
	enum Codes { PS_PS, PS_NPS, NPS_PS, R_R, R_NR, NR_R, NR_NR }; // T1_T2
	Codes taskCodes[][4] = { { NR_NR,	NR_R,	NPS_PS,	NPS_PS	},
							 { R_NR,	R_R,	NPS_PS,	NPS_PS	},
							 { PS_NPS,  PS_NPS, PS_PS,	PS_PS	},
							 { PS_NPS,  PS_NPS, PS_PS,	PS_PS	}
	};

	uRealTimeBaseTask *rbtask1, *rbtask2;
	uPeriodicBaseTask *pbtask1, *pbtask2;
	uSporadicBaseTask *sbtask1 = nullptr, *sbtask2 = nullptr;
	int index1, index2;

	rbtask1 = dynamic_cast<uRealTimeBaseTask *>(&task1);
	if ( rbtask1 == nullptr ) {
		index1 = 0;
		pbtask1 = nullptr;
		sbtask1 = nullptr;
	} else {
		index1 = 1;

		if ( ( pbtask1 = dynamic_cast<uPeriodicBaseTask *>(&task1) ) != nullptr ) {
			index1 = 2;
		} else if ( ( sbtask1 = dynamic_cast<uSporadicBaseTask *>(&task1) ) != nullptr ) {
			index1 = 3;
		} // if
	} // if

	rbtask2 = dynamic_cast<uRealTimeBaseTask *>(&task2);
	if ( rbtask2 == nullptr ) {
		index2 = 0;
		pbtask2 = nullptr;
		sbtask2 = nullptr;
	} else {
		index2 = 1;

		if ( ( pbtask2 = dynamic_cast<uPeriodicBaseTask *>(&task2) ) != nullptr ) {
			index2 = 2;
		} else if ( ( sbtask2 = dynamic_cast<uSporadicBaseTask *>(&task2) ) != nullptr ) {
			index2 = 3;
		} // if
	} // if

	switch ( taskCodes[index1][index2] ){
	  case PS_PS:										// both task1 and task2 periodic or sporadic
		temp = rbtask1->getDeadline() - rbtask2->getDeadline();
		if ( temp > 0 ) {
			return 1;
		} else if ( temp < 0 ) {
			return -1;
		} else {										// real-time tasks have equal deadlines => check their periods or frames
			uDuration period1, period2;

			if ( pbtask1 != nullptr ) {					// periodic ?
				period1 = pbtask1->getPeriod();
			} else if ( sbtask1  != nullptr ) {			// sporadic ?
				period1 = sbtask1->getFrame();
			} else {
				abort( "(uDeadlineMonotonicStatic *)%p.compare : internal error.", this );
			} // if

			if ( pbtask2 != nullptr ) {					// periodic ?
				period2 = pbtask2->getPeriod();
			} else if ( sbtask2  != nullptr ) {			// sporadic ?
				period2 = sbtask2->getFrame();
			} else {
				abort( "(uDeadlineMonotonicStatic *)%p.compare : internal error.", this );
			} // if

			temp = period1 - period2;
			return (temp > 0) ? 1 : (temp < 0) ? -1 : 0;
		} // if
		break;

	  case PS_NPS:										// task1 periodic or sporadic and task2 is not ?
		return -1;
		break;

	  case NPS_PS:										// task1 is not and task2 periodic or sporadic ?
		return 1;
		break;

	  case R_R:											// both task1 and task2 aperiodic
		temp = rbtask1->getDeadline() - rbtask2->getDeadline();
		return (temp > 0) ? 1 : (temp < 0) ? -1 : 0;
		break;

	  case R_NR:										// task1 aperiodic and task2 is not ?
		return -1;
		break;

	  case NR_R:										// task1 is not and task2 is aperiodic ?
		return 1;
		break;

	  case NR_NR:										// both task1 and task1 are non-real-time
		return 0;
		break;

	  default:
		abort( "(uDeadlineMonotonicStatic *)%p.compare : internal error.", this );
		break;
	} // switch
} // uDeadlineMonotonicStatic::compare


void uDeadlineMonotonicStatic::addInitialize( uSequence<uBaseTaskDL> &taskList ) {
	uDEBUGPRT( uDebugPrt( "(uDeadlineMonotonicStatic &)%p.addInitialize: enter\n", this ); );
	uSeqIter<uBaseTaskDL> iter;
	uBaseTaskDL *ref = nullptr, *prev = nullptr, *node = nullptr;

	// The cluster's list of tasks is maintained in sorted order. This algorithm relies on the kernel code adding new
	// tasks to the end of the cluster's list of tasks.  (Perhaps better to combine adding tasks to a cluster list with
	// initializing newly scheduled tasks so only one call is made in uTaskAdd.)

	uRealTimeBaseTask *rtb = dynamic_cast<uRealTimeBaseTask *>(&(taskList.tail()->task()));

	if ( rtb == nullptr ) {
		uDEBUGPRT( uDebugPrt( "(uDeadlineMonotonicStatic &)%p.addInitialize: exit1\n", this ); );
		return;
	} // exit

	ref = taskList.dropTail();

	if ( ref == nullptr ) {								// necessary if addInitialize is called from removeInitialize
		uDEBUGPRT( uDebugPrt( "(uDeadlineMonotonicStatic &)%p.addInitialize: exit2\n", this ); );
		return;
	} // exit

	for ( iter.over(taskList), prev = nullptr; iter >> node ; prev = node ) { // find place in the list to insert
	  if ( compare( ref->task(), node->task() ) < 0 ) break;
	} // for
	taskList.insertBef( ref, node );

	// Find out if task was ever on cluster, if so compare verCount with verCount for cluster.  If different or first
	// visit recalculate otherwise, stop after putting task back into task list

	if ( verCount == (unsigned int)rtb->getVersion( rtb->getCluster() ) ) {
		uDEBUGPRT( uDebugPrt( "(uDeadlineMonotonicStatic &)%p.addInitialize: exit3\n", this ); );
		return;
	} // exit

	// either new task or verCounts differ, increment verCount and continue
	verCount += 1;

	uBaseTask &task = ref->task();
	uRealTimeBaseTask *rtask = dynamic_cast<uRealTimeBaseTask *>(&task);
	if ( rtask != nullptr ) {
		int pval;
		int diff;
		int firstIndex = 0;
		int lastIndex = __U_MAX_NUMBER_PRIORITIES__;

		if ( prev != nullptr ) {
			firstIndex = getBasePriority( prev->task() );
		} // if

		if ( node != nullptr ) {
			lastIndex = getBasePriority( node->task());
		} // if

		// Scheduling is static, so no dynamic reassignment of priorities occurs.  A unique priority is assigned in
		// between the priorities of the task's neighbours.
		diff = (lastIndex - firstIndex) / 2 ;
		if ( diff == 0 ) abort( "(uDeadlineMonotonicStatic *)%p.addInitialize : Cannot assign priority to task %p between existing priorities", this, &task );
		pval = firstIndex + diff;

		setBasePriority( *rtask, pval );
		rtask->setVersion( rtask->getCluster(), verCount );
		setActivePriority( *rtask, pval );
	} else {
		if ( getBasePriority( task ) == 0 ) {			// if first time, intialize priority for nonreal-time task
			setBasePriority( task, __U_MAX_NUMBER_PRIORITIES__ - 1);
		} // if
		setActivePriority( task, __U_MAX_NUMBER_PRIORITIES__ - 1 );
	} // if

	for ( iter.over( taskList ); iter >> node; ) {
		uRealTimeBaseTask *rtask1 = dynamic_cast<uRealTimeBaseTask *>(&node->task());
		if ( rtask1 != nullptr ) {
			rtask1->setVersion( rtask1->getCluster(), verCount );
		} // if
	} // for

	uDEBUGPRT(
// 	uSeqIter<uBaseTaskDL> j;
//	uBaseTaskDL *ptr = nullptr;
//      for (j.over(taskList); j >> ptr; ) {
//          fprintf(stderr, "%p Task with priority %d\n", &(ptr->task()), ptr->task().getActivePriority());
//      }
//      fprintf(stderr, "Leaving uInitialize! \n");
		uDebugPrt( "(uDeadlineMonotonicStatic &)%p.addInitialize: exit4\n", this );
	);
} // uDeadlineMonotonicStatic::addInitialize


void uDeadlineMonotonicStatic::removeInitialize( uSequence<uBaseTaskDL> & ) {
	// Although removing a task may leave a hole in the priorities, the hole should not affect the ability to schedule
	// the task or the order the tasks execute. Therefore, no rescheduling is performed.

//	addInitialize( taskList );
} // uDeadlineMonotonicStatic::removeInitialize


void uDeadlineMonotonicStatic::rescheduleTask( uBaseTaskDL *taskNode, uBaseTaskSeq &taskList ) {
	verCount += 1;
	taskList.remove(taskNode);
	taskList.addTail(taskNode);
	addInitialize(taskList);
} // uDeadlineMonotonicStatic::rescheduleTask


// Local Variables: //
// compile-command: "make install" //
// End: //
