//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Ashif Harji 2000
// 
// Disinherit1Static.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue Apr 18 17:56:52 2000
// Last Modified By : Peter A. Buhr
// Last Modified On : Wed Apr 20 23:03:37 2022
// Update Count     : 25
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

#include <uDeadlineMonotonicStatic.h>
#include <uStaticPriorityQ.h>
#include <uStaticPIQ.h>
#include <iostream>
using std::cout;
using std::osacquire;
using std::endl;
#include <iomanip>
using std::setw;

_Mutex<uStaticPriorityQ, uStaticPriorityQ> class Monitor2 {
  public:
	void call1( int id, uDuration delay ){
		for (int i = 0; i < 3; i+=1 ){
			osacquire( cout ) << setw(3) << id << " blocks in monitor 2 for " << delay << " at priority " <<
				uThisTask().getActivePriorityValue() << ", " << uThisTask().getActiveQueueValue() << endl;
	    
			_Timeout( delay );
	    
			osacquire( cout ) << setw(3) << id << " wakes up in monitor 2 " << " at priority " <<
				uThisTask().getActivePriorityValue() << ", " << uThisTask().getActiveQueueValue() << endl;
		} // for

		osacquire( cout ) << setw(3) << id << " leaves monitor 2 at priority " <<
			uThisTask().getActivePriorityValue() << ", " << uThisTask().getActiveQueueValue() << endl;
	} // call1

	void call2( int id, uDuration delay ) {
		osacquire( cout ) << setw(3) << id << " blocks in monitor 2 for " << delay << " at priority " <<
			uThisTask().getActivePriorityValue() << ", " << uThisTask().getActiveQueueValue() << endl;
	
		_Timeout( delay );
	
		osacquire( cout ) << setw(3) << id << " leaves monitor 2 at priority " <<
			uThisTask().getActivePriorityValue() << ", " << uThisTask().getActiveQueueValue() << endl;
	} // call2
}; // Monitor2


_Mutex<uStaticPriorityQ, uStaticPriorityQ> class Monitor1 {
  public:
	void call( int id, uDuration delay1, uDuration delay2, Monitor2 &m2 ) {
		osacquire( cout ) << setw(3) << id << " blocks in monitor 1 for " << delay1 << " at priority " <<
			uThisTask().getActivePriorityValue() << ", " << uThisTask().getActiveQueueValue() << endl;

		_Timeout(delay1);

		// call Monitor2
		if (id == 1 ) { 
			osacquire( cout ) << setw(3) << id << " calls monitor 2 at priority " <<
				uThisTask().getActivePriorityValue() << ", " << uThisTask().getActiveQueueValue() << endl;
			m2.call1(id, delay2);
		} else {
			osacquire( cout ) << setw(3) << id << " calls monitor 2 at priority " <<
				uThisTask().getActivePriorityValue() << ", " << uThisTask().getActiveQueueValue() << endl;
			m2.call2(id, delay2);
		} // if
	    
		osacquire( cout ) << setw(3) << id << " leaves monitor 1 at priority " <<
			uThisTask().getActivePriorityValue() << ", " << uThisTask().getActiveQueueValue() << endl;
	} // call
}; // Monitor1


Monitor1 monitor1;
Monitor2 monitor2;

_Mutex<uStaticPriorityQ, uStaticPriorityQ> _PeriodicTask<uStaticPIQ> task1 {
	uDuration D1, D2;
	int id;

	void main() {
		_Timeout(D1);
		osacquire( cout ) << setw(3) << id << " calls monitor 1 at priority " <<
			uThisTask().getActivePriorityValue() << ", " << uThisTask().getActiveQueueValue() << endl;
		monitor1.call( id, D1, D2, monitor2 );
	} // Philosopher::main
  public:
	task1( int id, uDuration period, uDuration delay1, uDuration delay2, uCluster &clust ) :
	    uPeriodicBaseTask( period, uTime(), uClock::currTime() + 90, period, clust ),
//	    uPeriodicBaseTask( period, uTime(), uTime(), period, clust ),
		D1( delay1 ), D2( delay2 ), id( id ) {
	} // task1::task1
}; // task1


_Mutex<uStaticPriorityQ, uStaticPriorityQ> _PeriodicTask<uStaticPIQ> task2 {
	uDuration D1;
	int id;

	void main() {
		_Timeout( D1 );
		osacquire( cout ) << setw(3) << id << " calls monitor 2 at priority " <<
			uThisTask().getActivePriorityValue() << ", " << uThisTask().getActiveQueueValue() << endl;
	    monitor2.call1( id, D1 );
	} // Philosopher::main
  public:
	task2( int id, uDuration period, uDuration delay, uCluster &clust ) :
	    uPeriodicBaseTask( period, uTime(), uClock::currTime() + 90, period, clust ),
//	    uPeriodicBaseTask( period, uTime(), uTime(), period, clust ),
		D1( delay ), id( id ) {
	} // task2::task2
}; // task2

int main() {
	uDeadlineMonotonicStatic rq ;						// create real-time scheduler
	uRealTimeCluster rtCluster( rq );					// create real-time cluster with scheduler
	uProcessor *processor;
	{
		task2 t1( 1, uDuration( 500 ), uDuration( 3 ), rtCluster );
		task1 t2( 2, uDuration( 400 ), uDuration( 3 ), uDuration( 0 ), rtCluster );
		task1 t3( 3, uDuration( 300 ), uDuration( 7 ), uDuration( 0 ), rtCluster );

		processor = new uProcessor( rtCluster );		// now create the processor to do the work
	}
	delete processor;
	osacquire( cout ) << "successful completion" << endl;
} // main

// Local Variables: //
// compile-command: "u++ Disinherit1Static.cc" //
// End: //
