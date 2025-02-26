//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2012
// 
// TimeoutPoke.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Thu Apr  5 08:08:27 2012
// Last Modified By : Peter A. Buhr
// Last Modified On : Wed Apr 20 23:00:08 2022
// Update Count     : 82
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

#include <uRealTime.h>
#include <uDeadlineMonotonic.h>
#include <iostream>
using namespace std;

// Check poke of cluster processor when there is no preemption and high-priority task unblocks after timeout.

unsigned int uDefaultPreemption() {
	return 0;
} // uDefaultPreemption


_Task TestTask {
	static const unsigned int Times = 10;
	int id, priority;
	volatile bool &flag;

	void main() {
		if ( priority == 0 ) {							// high priority
			for ( unsigned int i = 0; i < Times; i += 1 ) {
				sleep( uDuration( 1 ) );
				flag = true;
			} // for
		} else {										// low priority
			for ( unsigned int i = 0, c = 0; i < Times; c += 1 ) {
				if ( flag ) {
					flag = false;
					i += 1;
				} // if
#if ! defined( __U_MULTI__ )
// In uniprocessor, the program cannot work without some preemptions of the low-priority tasks, which is why this yield
// is there. The obvious problem is that when the sleep expires, both high-priority tasks may run because they set their
// timeout at almost exactly the same time. After the two high-priority tasks run, only one of the low-priority tasks
// runs until the next timeout. As a result, each low-priority task alternates on each timeout.  So instead of executing
// N times, which is needed to terminate, the low-priority tasks execute N/2 times, and the last one running just
// spins. We tried to fix this problem by having the low-priority task yield when its companion high-priority flag
// changes so both low-priority tasks execute each time a timeout occurs. However, this hack does not work because the
// timeouts can get out of sync, and hence, two SIGUSR1s are sent. As a result, a signal can arrive just after the yield
// to a low-priority task, which cause a schedule back to the other low-priority task before the first one can update
// that it has seen its companion high-priority flag's change. Basically, the yield has to occur far enough away from
// the SIGUSR1 window that a yield is guaranteed to switch to the other low-priority task.
//
// In multiprocessor, the previous problem does not occur because the other low-priority task is spinning on the other
// processor, and hence, both low-priority tasks see their companion high-priority flag change.
				if ( c % 1000000 == 0 ) yield();
#endif // __U_MULTI__
			} // for
		} // if
		osacquire( cerr ) << "end " << getName() << endl;
	} // TestTask::main
  public:
	TestTask( int id, int priority, volatile bool &flag, uRealTimeCluster &clust ) : uBaseTask( clust ), id( id ), priority( priority ), flag( flag ) {
		setName( priority == 0 ? "high" : "low" );
		setActivePriority( priority );
		setBasePriority( priority );
	} // TestTask::TestTask
}; // TestTask

int main() {
	volatile bool flag1 = false, flag2 = false;
	uPriorityScheduleSeq<uBaseTaskSeq, uBaseTaskDL> rq;
	uRealTimeCluster rtCluster( rq );					// create real-time cluster with scheduler
	uProcessor *processor1, *processor2;
	{
		processor1 = new uProcessor( rtCluster );		// now create the processor to do the work
		processor2 = new uProcessor( rtCluster );		// now create the processor to do the work
		TestTask high1( 0, 0, flag1, rtCluster ), high2( 1, 0, flag2, rtCluster );
		TestTask low1( 0, 1, flag1, rtCluster ), low2( 1, 1, flag2, rtCluster );
	}
	delete processor1;
	delete processor2;
	cerr << "successful completion" << endl;
} // main

// Local Variables: //
// compile-command: "u++ TimeoutPoke.cc" //
// End: //
