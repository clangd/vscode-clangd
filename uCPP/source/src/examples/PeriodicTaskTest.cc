//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Philipp E. Lim and Ashif S. Harji 1996
// 
// PeriodicTaskTest.cc -- 
// 
// Author           : Philipp E. Lim and Ashif S. Harji
// Created On       : Tue Jul 23 14:48:37 1996
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 26 15:04:27 2022
// Update Count     : 133
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
using std::cout;
using std::osacquire;
using std::endl;
#include <iomanip>
using std::setw;

const int Delay = 100000;
uTime Start;											// global start time for all tasks


class AcrossCxtSw : public uContext {
	uTime &clock, beginCxtSw;
  public:
	AcrossCxtSw( uTime &clock ) : clock( clock ) {
	} // AcrossCxtSw::AcrossCxtSw

	void save(){
		beginCxtSw = uClock::currTime();
	} // AcrossCxtSw::save

	void restore(){
		clock += uClock::currTime() - beginCxtSw;
	} // AcrossCxtSw::restore
}; // AcrossCxtSw


// This real-time task is periodic with a specific duration and computation time (C). However, due to pre-emption from
// tasks with higher priority and time slicing, a direct calculation to simulate computation time does not work because
// time spent executing other tasks is not excluded from the calculation of C.  In order to compensate for this, a
// function is called during each context switch to calculate the amount of time spend outside the task.  By adding this
// time to the task's calculated stop time after each context switch, an accurate simulation of the computation time, C,
// is possible.

_PeriodicTask TestTask {
	uDuration C;
	int id;

	void main() {
		uTime starttime, delay, currtime, endtime;

		starttime = uClock::currTime();
		osacquire( cout ) << setw(3) << starttime - ::Start << "\t" << id << " Beginning." << endl;

		// The loop below must deal with asynchronous advancing of variable "delay" during context switches. A problem
		// occurs when "delay" is loaded into a register for the comparison, a context switch occurs, and "delay" is
		// advanced, but the old value of "delay" is used in a comparison with the current time. This situation causes a
		// premature exit from the loop because the old delay value is less than the current time.  To solve the
		// problem, the current time value is saved *before* the comparison.  Thus, whether the delay value is the old
		// or new (i.e., becomes larger) value, the comparision does not cause a premature exit.

		delay = uClock::currTime() + C;
		{
			AcrossCxtSw acrossCxtSw( delay );			// cause delay to advance across a context switch interval
			do {
				for ( int i = 0; i < Delay; i += 1 );	// do not spend too much time in non-interruptible clock routine
				currtime = uClock::currTime();
			} while ( delay > currtime );
		}
		endtime = uClock::currTime();
		osacquire( cout ) << setw(3) << endtime - ::Start << "\t" << id << " Ending " <<
			setw(3) << endtime - starttime << " seconds later" << endl;
	} // TestTask::main
  public:
	TestTask( int id, uDuration period, uDuration deadline, uCluster &clust ) :
	uPeriodicBaseTask( period, uTime(), uClock::currTime() + 90, deadline, clust ),
	C( deadline ), id( id ) {
	} // TestTask::TestTask
}; // TestTask

int main() {
	uDeadlineMonotonic rq;								// create real-time scheduler
	uRealTimeCluster rtCluster( rq );					// create real-time cluster with scheduler
	uProcessor *processor;
	{
		TestTask t1( 1, 15, 5, rtCluster );
		TestTask t2( 2, 30, 9, rtCluster );
		TestTask t3( 3, 60, 19, rtCluster );

		osacquire( cout ) << "Time  \t\tTaskID" << endl;

		::Start = uClock::currTime();
		processor = new uProcessor( rtCluster );		// now create the processor to do the work
	} // wait for t1, t2, t3 to finish
	delete processor;
	cout << "successful completion" << endl;
} // main
