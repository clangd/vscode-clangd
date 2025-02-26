//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Philipp E. Lim and Ashif S. Harji 1996
// 
// RealTimePhilosophers.cc -- 
// 
// Author           : Philipp E. Lim and Ashif S. Harji
// Created On       : Tue Jul 23 14:58:56 1996
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Apr 24 18:22:32 2022
// Update Count     : 87
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


uRealTimeMonitor RestRoom {
  public:
  void Toilet( int id, uDuration C1 ) {
	  uTime starttime, delay, currtime, endtime;

	  starttime = uClock::currTime();
	  osacquire( cout ) << setw(3) << starttime - ::Start << "\t" << id << " goes to TOILET (priority " <<
	  uThisTask().getActivePriority() << ")" << endl;

	  // The loop below must deal with asynchronous advancing of variable "delay" during context switches. A problem
	  // occurs when "delay" is loaded into a register for the comparison, a context switch occurs, and "delay" is
	  // advanced, but the old value of "delay" is used in a comparison with the current time. This situation causes a
	  // premature exit from the loop because the old delay value is less than the current time.  To solve the problem,
	  // the current time value is saved *before* the comparison.  Thus, whether the delay value is the old or new
	  // (i.e., becomes larger) value, the comparision does not cause a premature exit.

	  delay = uClock::currTime() + C1;
	  {
		  AcrossCxtSw acrossCxtSw( delay );
		  do {
			  for ( int i = 0; i < Delay; i += 1 );		// do not spend too much time in non-interruptible clock routine
			  currtime = uClock::currTime();
		  } while ( delay > currtime );
	  }

	  endtime = uClock::currTime();
	  osacquire( cout ) << setw(3) << endtime - ::Start << "\t" << id << " leaves TOILET " <<
	  setw(3) << endtime - starttime << " seconds later (priority " << uThisTask().getActivePriority() << ")" << endl;
  } // Toilet::Toilet

  _Nomutex void Wash( int id, uDuration C2 ) {
	  uTime starttime, delay, currtime, endtime;

	  starttime = uClock::currTime();
	  osacquire( cout ) << setw(3) << starttime - ::Start << "\t" << id << " goes to WASH (priority " <<
	  uThisTask().getActivePriority() << ")" << endl;

	  // The loop below must deal with asynchronous advancing of variable "delay" during context switches. A problem
	  // occurs when "delay" is loaded into a register for the comparison, a context switch occurs, and "delay" is
	  // advanced, but the old value of "delay" is used in a comparison with the current time. This situation causes a
	  // premature exit from the loop because the old delay value is less than the current time.  To solve the problem,
	  // the current time value is saved *before* the comparison.  Thus, whether the delay value is the old or new
	  // (i.e., becomes larger) value, the comparision does not cause a premature exit.

	  delay = uClock::currTime() + C2;
	  {
		  AcrossCxtSw acrossCxtSw( delay );
		  do {
			  for ( int i = 0; i < Delay; i += 1 );		// do not spend too much time in non-interruptible clock routine
			  currtime = uClock::currTime();
		  } while ( delay > currtime );
	  }

	  endtime = uClock::currTime();
	  osacquire( cout ) << setw(3) << endtime - ::Start << "\t" << id << " finished with WASH " <<
	  setw(3) << endtime - starttime << " seconds later (priority " << uThisTask().getActivePriority() << ")" << endl;
  } // Toilet::Wash
}; // RestRoom

RestRoom restroom;

_PeriodicTask Philosopher {
	uDuration C1, C2;
	int id;

	void main() {
		restroom.Toilet( id, C1 );
		restroom.Wash( id, C2 );
	} // Philosopher::main
  public:
	Philosopher( int id, uDuration period, uDuration toilet, uDuration wash, uCluster &clust ) :
	uPeriodicBaseTask( period, uTime(), uClock::currTime() + 90, period, clust ),
//	    uPeriodicBaseTask( period, uTime(), uTime(), period, clust ),
	C1( toilet ), C2( wash ), id( id ) {
	} // Philosopher::Philosopher
}; // Philosopher

int main() {
	uDeadlineMonotonic rq ;								// create real-time scheduler
	uRealTimeCluster rtCluster( rq );					// create real-time cluster with scheduler
	uProcessor *processor;
	{
		Philosopher t1( 1, 20, 2, 2, rtCluster );
		Philosopher t2( 2, 30, 4, 3, rtCluster );
		Philosopher t3( 3, 40, 6, 4, rtCluster );
		Philosopher t4( 4, 50, 8, 5, rtCluster );

		osacquire( cout ) << "Time  \t\tPhilosopher" << endl;

		::Start = uClock::currTime();
		processor = new uProcessor( rtCluster );		// now create the processor to do the work
	}
	delete processor;
	osacquire( cout ) << "successful completion" << endl;
} // main
