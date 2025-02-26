//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// Bench.cc -- Timing benchmarks for the basic features in uC++.
// 
// Author           : Peter A. Buhr
// Created On       : Thu Feb 15 22:03:16 1990
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Sep 30 15:35:18 2022
// Update Count     : 495
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


#include <iostream>
using std::cerr;
using std::osacquire;
using std::endl;

unsigned int uDefaultPreemption() {
	return 0;
} // uDefaultPreemption


//=======================================
// time class
//=======================================

class ClassDummy {
	static volatile int i;								// prevent dead-code removal
  public:
	ClassDummy() __attribute__(( noinline )) { i = 1; }
	int bidirectional( volatile int a, int, int, int ) __attribute__(( noinline )) {
		return a;
	} // ClassDummy::bidirectional
}; // ClassDummy
volatile int ClassDummy::i;

void BlockClassCreateDelete( int N ) {
	uTime start = uClock::getCPUTime();
	for ( int i = 0; i < N; i += 1 ) {
		ClassDummy dummy;
	} // for
	osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
} // BlockClassCreateDelete

void DynamicClassCreateDelete( int N ) {
	uTime start = uClock::getCPUTime();
	for ( int i = 0; i < N; i += 1 ) {
		ClassDummy *dummy = new ClassDummy;
		delete dummy;
	} // for
	osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
} // DynamicClassCreateDelete

void ClassBidirectional( int N ) {
	ClassDummy dummy;
	volatile int rv __attribute__(( unused ));

	uTime start = uClock::getCPUTime();
	for ( int i = 0; i < N; i += 1 ) {
		rv = dummy.bidirectional( 1, 2, 3, 4 ); 
	} // for
	osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
} // ClassBidirectional

//=======================================
// time coroutine
//=======================================

_Coroutine CoroutineDummy {
	void main() {
	} // CoroutineDummy::main
  public:
	int bidirectional( volatile int a, int, int, int ) __attribute__(( noinline )) {
		return a;
	} // CoroutineDummy::bidirectional
}; // CoroutineDummy

void BlockCoroutineCreateDelete( int N ) {
	uTime start = uClock::getCPUTime();
	for ( int i = 0; i < N; i += 1 ) {
		CoroutineDummy dummy;
	} // for
	osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
} // BlockCoroutineCreateDelete

void DynamicCoroutineCreateDelete( int N ) {
	uTime start = uClock::getCPUTime();
	for ( int i = 0; i < N; i += 1 ) {
		CoroutineDummy *dummy = new CoroutineDummy;
		delete dummy;
	} // for
	osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
} // DynamicCoroutineCreateDelete

void CoroutineBidirectional( int N ) {
	CoroutineDummy dummy;
	volatile int rv __attribute__(( unused ));

	uTime start = uClock::getCPUTime();
	for ( int i = 0; i < N; i += 1 ) {
		rv = dummy.bidirectional( 1, 2, 3, 4 ); 
	} // for
	osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
} // CoroutineBidirectional

_Coroutine CoroutineResume {
	int N;

	void main() {
		for ( int i = 1; i <= N; i += 1 ) {
			suspend();
		} // for
	} // CoroutineResume::main
  public:
	CoroutineResume( int N ) {
		CoroutineResume::N = N;
	} // CoroutineResume::CoroutineResume

	void resumer() {
		uTime start = uClock::getCPUTime();
		for ( int i = 1; i <= N; i += 1 ) {
			resume();
		} // for
		osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
	} // CoroutineResume::resumer
}; // CoroutineResume

//=======================================
// time monitor
//=======================================

_Mutex class MonitorDummy {
  public:
	int bidirectional( volatile int a, int, int, int ) __attribute__(( noinline )) {
		return a;
	} // MonitorDummy::bidirectional
}; // MonitorDummy

void BlockMonitorCreateDelete( int N ) {
	uTime start = uClock::getCPUTime();
	for ( int i = 0; i < N; i += 1 ) {
		MonitorDummy dummy;
	} // for
	osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
} // BlockMonitorCreateDelete

void DynamicMonitorCreateDelete( int N ) {
	uTime start = uClock::getCPUTime();
	for ( int i = 0; i < N; i += 1 ) {
		MonitorDummy *dummy = new MonitorDummy;
		delete dummy;
	} // for
	osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
} // DynamicMonitorCreateDelete

void MonitorBidirectional( int N ) {
	MonitorDummy dummy;
	volatile int rv __attribute__(( unused ));

	uTime start = uClock::getCPUTime();
	for ( int i = 0; i < N; i += 1 ) {
		rv = dummy.bidirectional( 1, 2, 3, 4 ); 
	} // for
	osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
} // MonitorBidirectional

_Monitor Monitor {
	uCondition condA, condB;
  public:
	volatile int here;

	Monitor() {
		here = 0;
	}; // Monitor::Monitor

	int caller( int a ) {
		return a;
	} // Monitor::caller

	void acceptor( int N ) {
		here = 1;										// indicate that the acceptor is in place

		uTime start = uClock::getCPUTime();
		for ( int i = 1; i <= N; i += 1 ) {
			_Accept( caller );
		} // for
		osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
	} // Monitor::acceptor

	void sigwaiterA( int N ) {
		uTime start = uClock::getCPUTime();
		for ( int i = 1;; i += 1 ) {
			condA.signal();
		  if ( i > N ) break;
			condB.wait();
		} // for
		osacquire( cerr ) << "\t " << ( ( (uClock::getCPUTime() - start).nanoseconds() ) / N );
	} // Monitor::sigwaiterA

	void sigwaiterB( int N ) {
		for ( int i = 1;; i += 1 ) {
			condB.signal();
		  if ( i > N ) break;
			condA.wait();
		} // for
	} // Monitor::sigwaiterB
}; // Monitor

_Task MonitorAcceptorPartner {
	int N;
	Monitor &m;

	void main() {
		m.acceptor( N );
	} // MonitorAcceptorPartner::main
  public:
	MonitorAcceptorPartner( int N, Monitor &m ) : m( m ) {
		MonitorAcceptorPartner::N = N;
	} // MonitorAcceptorPartner
}; // MonitorAcceptorPartner

_Task MonitorAcceptor {
	int N;

	void main() {
		Monitor m;
		MonitorAcceptorPartner partner( N, m );
		volatile int rv __attribute__(( unused ));

		while ( m.here == 0 ) yield();					// wait until acceptor is in monitor

		for ( int i = 1; i <= N; i += 1 ) {
			rv = m.caller( 1 );							// nonblocking call (i.e., no context switch) as call accepted
			yield();									// now force context switch to acceptor to accept next call
		} // for
	} // MonitorAcceptor::main
  public:
	MonitorAcceptor( int NoOfTimes ) {
		N = NoOfTimes;
	} // MonitorAcceptor
}; // MonitorAcceptor

_Task MonitorSignallerPartner {
	int N;
	Monitor &m;

	void main() {
		m.sigwaiterB( N );
	} // MonitorSignallerPartner::main
  public:
	MonitorSignallerPartner( int N, Monitor &m ) : m( m ) {
		MonitorSignallerPartner::N = N;
	} // MonitorSignallerPartner
}; // MonitorSignallerPartner

_Task MonitorSignaller {
	int N;

	void main() {
		Monitor m;
		MonitorSignallerPartner partner( N, m );

		m.sigwaiterA( N );
	} // MonitorSignaller::main
  public:
	MonitorSignaller( int NoOfTimes ) {
		N = NoOfTimes;
	} // MonitorSignaller
}; // MonitorSignaller

//=======================================
// time coroutine-monitor
//=======================================

_Mutex _Coroutine CoroutineMonitorDummy {
	void main() {}
  public:
	int bidirectional( volatile int a, int, int, int ) __attribute__(( noinline )) {
		return a;
	} // CoroutineMonitorDummy::bidirectional
}; // CoroutineMonitorDummy

void BlockCoroutineMonitorCreateDelete( int N ) {
	uTime start = uClock::getCPUTime();
	for ( int i = 0; i < N; i += 1 ) {
		CoroutineMonitorDummy dummy;
	} // for
	osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
} // BlockCoroutineMonitorCreateDelete

void DynamicCoroutineMonitorCreateDelete( int N ) {
	uTime start = uClock::getCPUTime();
	for ( int i = 0; i < N; i += 1 ) {
		CoroutineMonitorDummy *dummy = new CoroutineMonitorDummy;
		delete dummy;
	} // for
	osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
} // DynamicCoroutineMonitorCreateDelete

void CoroutineMonitorBidirectional( int N ) {
	CoroutineMonitorDummy dummy;
	volatile int rv __attribute__(( unused ));

	uTime start = uClock::getCPUTime();
	for ( int i = 0; i < N; i += 1 ) {
		rv = dummy.bidirectional( 1, 2, 3, 4 );
	} // for
	osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
} // CoroutineMonitorBidirectional

_Mutex _Coroutine CoroutineMonitorA {
	int N;
  public:
	volatile int here;

	CoroutineMonitorA() {
		here = 0;
	} // CoroutineMonitorA::CoroutineMonitorA

	int caller( int a ) {
		return a;
	} // CoroutineMonitorA::caller

	void acceptor( int N ) {
		CoroutineMonitorA::N = N;
		resume();
	} // CoroutineMonitorA::acceptor
  private:
	void main() {
		here = 1;

		uTime start = uClock::getCPUTime();
		for ( int i = 1; i <= N; i += 1 ) {
			_Accept( caller );
		} // for
		osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
	}; // CoroutineMonitorA::main
}; // CoroutineMonitorA

_Task CoroutineMonitorAcceptorPartner {
	int N;
	CoroutineMonitorA &cm;

	void main() {
		cm.acceptor( N );
	} // CoroutineMonitorAcceptorPartner::main
  public:
	CoroutineMonitorAcceptorPartner( int N, CoroutineMonitorA &cm ) : cm( cm ) {
		CoroutineMonitorAcceptorPartner::N = N;
	} // CoroutineMonitorAcceptorPartner
}; // CoroutineMonitorAcceptorPartner

_Task CoroutineMonitorAcceptor {
	int N;
	void main();
  public:
	CoroutineMonitorAcceptor( int NoOfTimes ) {
		N = NoOfTimes;
	} // CoroutineMonitorAcceptor
}; // CoroutineMonitorAcceptor

void CoroutineMonitorAcceptor::main() {
	CoroutineMonitorA cm;
	CoroutineMonitorAcceptorPartner partner( N, cm );
	volatile int rv __attribute__(( unused ));

	while ( cm.here == 0 ) yield();
	
	for ( int i = 1; i <= N; i += 1 ) {
		rv = cm.caller( 1 ); 							// nonblocking call (i.e., no context switch) as call accepted
		yield();										// now force context switch to acceptor to accept next call
	} // for
} // CoroutineMonitorAcceptor::main

_Mutex _Coroutine CoroutineMonitorResume {
	int N;

	void main() {
		for ( int i = 1; i <= N; i += 1 ) {
			suspend();
		} // for
	}; // CoroutineMonitorResume::main
  public:
	CoroutineMonitorResume( int N ) {
		CoroutineMonitorResume::N = N;
	} // CoroutineMonitorResume::CoroutineMonitorResume

	void resumer() {
		uTime start = uClock::getCPUTime();
		for ( int i = 1; i <= N; i += 1 ) {
			resume();
		} // for
		osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
	}; // CoroutineMonitorResume::resumer
}; // CoroutineMonitorResume

_Mutex _Coroutine CoroutineMonitorB {
	int N;
	uCondition condA, condB;

	void main() {
		uTime start = uClock::getCPUTime();
		for ( int i = 1;; i += 1 ) {
			condA.signal();
		  if ( i > N ) break;
			condB.wait();
		} // for
		osacquire( cerr ) << "\t " << ( ( (uClock::getCPUTime() - start).nanoseconds() ) / N );
	}; // CoroutineMonitorB::main
  public:
	void sigwaiterA( int N ) {
		CoroutineMonitorB::N = N;
		resume();
	} // CoroutineMonitorB::sigwaiterA

	void sigwaiterB( int N ) {
		for ( int i = 1;; i += 1 ) {
			condB.signal();
		  if ( i > N ) break;
			condA.wait();
		} // for
	} // CoroutineMonitorB::sigwaiterB
}; // CoroutineMonitorB

_Task CoroutineMonitorSignallerPartner {
	int N;
	CoroutineMonitorB &cm;

	void main() {
		cm.sigwaiterB( N );
	} // CoroutineMonitorSignallerPartner::main
  public:
	CoroutineMonitorSignallerPartner( int N, CoroutineMonitorB &cm ) : cm( cm ) {
		CoroutineMonitorSignallerPartner::N = N;
	} // CoroutineMonitorSignallerPartner
}; // CoroutineMonitorSignallerPartner

_Task CoroutineMonitorSignaller {
	int N;

	void main() {
		CoroutineMonitorB cm;
		CoroutineMonitorSignallerPartner partner( N, cm );

		cm.sigwaiterA( N );
	} // CoroutineMonitorSignaller::main
  public:
	CoroutineMonitorSignaller( int NoOfTimes ) {
		N = NoOfTimes;
	} // CoroutineMonitorSignaller
}; // CoroutineMonitorSignaller


//=======================================
// time task
//=======================================

_Task TaskDummy {
	void main() {
	} // TaskDummy::main
}; // TaskDummy

void BlockTaskCreateDelete( int N ) {
	uTime start = uClock::getCPUTime();
	for ( int i = 0; i < N; i += 1 ) {
		TaskDummy dummy;
	} // for
	osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
} // BlockTaskCreateDelete

void DynamicTaskCreateDelete( int N ) {
	uTime start = uClock::getCPUTime();
	for ( int i = 0; i < N; i += 1 ) {
		TaskDummy *dummy = new TaskDummy;
		delete dummy;
	} // for
	osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
} // DynamicTaskCreateDelete

_Task TaskAcceptorPartner {
	int N;
  public:
	int caller( int a ) {
		return a;
	} // TaskAcceptorPartner::caller

	TaskAcceptorPartner( int N ) {
		TaskAcceptorPartner::N = N;
	} // TaskAcceptorPartner
  private:
	void main() {
		uTime start = uClock::getCPUTime();
		for ( int i = 1; i <= N; i += 1 ) {
			_Accept( caller );
		} // for
		osacquire( cerr ) << "\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
	} // TaskAcceptorPartner::main
}; // TaskAcceptorPartner

_Task TaskAcceptor {
	int N;

	void main() {
		TaskAcceptorPartner partner( N );
		volatile int rv __attribute__(( unused ));

		for ( int i = 1; i <= N; i += 1 ) {
			rv = partner.caller( 1 );					// nonblocking call (i.e., no context switch) as call accepted
			yield();									// now force context switch to acceptor to accept next call
		} // for
	} // TaskAcceptor::main
  public:
	TaskAcceptor( int NoOfTimes ) {
		N = NoOfTimes;
	} // TaskAcceptor
}; // TaskAcceptor

_Task TaskSignallerPartner {
	int N;
	uCondition condA, condB;
  public:
	void sigwaiter( int N ) {
		for ( int i = 1;; i += 1 ) {
			condB.signal();
		  if ( i > N ) break;
			condA.wait();
		} // for
	} // TaskSignallerPartner::sigwaiter

	TaskSignallerPartner( int N ) {
		TaskSignallerPartner::N = N;
	} // TaskSignallerPartner
  private:
	void main() {
		_Accept( sigwaiter );

		uTime start = uClock::getCPUTime();
		for ( int i = 1;; i += 1 ) {
			condA.signal();
		  if ( i > N ) break;
			condB.wait();
		} // for
		osacquire( cerr ) << "\t " << ( ( (uClock::getCPUTime() - start).nanoseconds() ) / N );
	} // TaskSignallerPartner::main
}; // TaskSignallerPartner

_Task TaskSignaller {
	int N;

	void main() {
		TaskSignallerPartner partner( N );

		partner.sigwaiter( N );
	} // TaskSignaller::main
  public:
	TaskSignaller( int NoOfTimes ) {
		N = NoOfTimes;
	} // TaskSignaller
}; // TaskSignaller

//=======================================
// time context switch
//=======================================

_Task ContextSwitch {
	int N;

	void main() {	
		uTime start = uClock::getCPUTime();
		for ( int i = 1; i <= N; i += 1 ) {
			uYieldNoPoll();
		} // for
		osacquire( cerr ) << "\t\t\t " << ( (uClock::getCPUTime() - start).nanoseconds() ) / N;
	} // ContextSwitch::main
  public:
	ContextSwitch( int N ) {
		ContextSwitch::N = N;
	} // ContextSwitch
}; // ContextSwitch

//=======================================
// benchmark driver
//=======================================

// uC++ has 4 modes:
// 
// 1. uniprocessor, where user threads are executed by a single kernel thread and time slicing provides
//    nondeterminism. Useful for teaching, testing, and embedded systems on a single processor, but no parallelism.
// 
// 2. multiprocessor, where user threads are executed by multiple kernel threads and time-slicing provides extra
//    nondeterminism. Provides full concurrency model and parallelism.
// 
// 3. debug, where uC++ inserts LOTS of runtime checks for errors. Useful for teaching and debugging.
// 
// 4. nodebug, where almost all the runtime checking are removed. Useful for high-performance production code.
// 
// The benchmark runs these combination: uniprocessor/debug, uniprocessor/nodebug, multiprocessor/debug, and
// multiprocessor/nodebug. Some debug results are significantly higher than nodebug because a guard page is inserted at
// the end of the user-thread stack and the memory allocator scrubs memory after each allocation, both of which are
// expensive.  The multiprocessor timings are higher than their uniprocessor counterparts because of the extra
// synchronization code necessary on multiprocessor machines.
// 
// uC++ has 5 kinds of objects: class, coroutine, monitor, coroutine/monitor, task.  Class is a regular C++ class, and
// provides a control measurement because all the other object-kinds are built on top of a class. Coroutine is a class
// with a stack. Monitor is a class with mutual exclusion. Cormonitor is a coroutine with mutual exclusion. Task is a
// cormonitor with a thread. There is a line in the benchmark for each kind of object, plus the cost of a context-switch
// cycle from a thread to itself at the end.
// 
// The first two columns show the cost to create an object on the stack and in the heap. The next two columns are the
// cost of a blocking cycle, which involves two (direct) or four context switches (indirect through runtime-kernel). The
// last two columns are the cost to call an object member passing in 16 bytes of arguments and returning a 4 byte
// results.

int main() {
	#if defined( __U_AFFINITY__ )
	// Prevent moving onto cold CPUs during benchmark.
	cpu_set_t mask;
	uThisProcessor().getAffinity( mask );				// get allowable CPU set
	int cpu;
	for ( cpu = CPU_SETSIZE;; cpu -= 1 ) {				// look for first available CPU
	  if ( cpu == -1 ) abort( "could not find available CPU to set affinity" );
	  if ( CPU_ISSET( cpu, &mask ) ) break;
	} // for
	uThisProcessor().setAffinity( cpu );				// execute benchmark on this CPU
	#endif // __U_AFFINITY__

	const int NoOfTimes =
		#if defined( __U_DEBUG__ )						// takes longer so run fewer iterations
		1000000;
		#else
		10000000;
		#endif // __U_DEBUG__

	osacquire( cerr ) << "\t\tcreate\tcreate\tresume/\tsignal/\t16i/4o\t16i/4o" << endl;
	osacquire( cerr ) << "(nsecs)";
	osacquire( cerr ) << "\t\tdelete/\tdelete/\tsuspend\twait\tbytes\tbytes" << endl;
	osacquire( cerr ) << "\t\tstack\theap\t2-cycle\t4-cycle\tcall\taccept" << endl;

	osacquire( cerr ) << "class\t";
	BlockClassCreateDelete( NoOfTimes );
	DynamicClassCreateDelete( NoOfTimes );
	osacquire( cerr ) << "\t N/A\t N/A";
	ClassBidirectional( NoOfTimes );
	osacquire( cerr ) << "\t N/A" << endl;

	osacquire( cerr ) << "coroutine";
	BlockCoroutineCreateDelete( NoOfTimes );
	DynamicCoroutineCreateDelete( NoOfTimes );
	{
		CoroutineResume resumer( NoOfTimes );
		resumer.resumer();
	}
	osacquire( cerr ) << "\t N/A";
	CoroutineBidirectional( NoOfTimes );
	osacquire( cerr ) << "\t N/A" << endl;

	osacquire( cerr ) << "monitor\t";
	BlockMonitorCreateDelete( NoOfTimes );
	DynamicMonitorCreateDelete( NoOfTimes );
	osacquire( cerr ) << "\t N/A";
	{
		MonitorSignaller m( NoOfTimes );
	}
	MonitorBidirectional( NoOfTimes );
	{
		MonitorAcceptor m( NoOfTimes );
	}
	osacquire( cerr ) << endl;

	osacquire( cerr ) << "cormonitor";
	BlockCoroutineMonitorCreateDelete( NoOfTimes );
	DynamicCoroutineMonitorCreateDelete( NoOfTimes );
	{
		CoroutineMonitorResume resumer( NoOfTimes );
		resumer.resumer();
	}
	{
		CoroutineMonitorSignaller cm( NoOfTimes );
	}
	CoroutineMonitorBidirectional( NoOfTimes );
	{
		CoroutineMonitorAcceptor cm( NoOfTimes );
	}
	osacquire( cerr ) << endl;

	osacquire( cerr ) << "task\t";
	BlockTaskCreateDelete( NoOfTimes );
	DynamicTaskCreateDelete( NoOfTimes );
	osacquire( cerr ) << "\t N/A";
	{
		TaskSignaller t( NoOfTimes );
	}
	osacquire( cerr ) << "\t N/A";
	{
		TaskAcceptor t( NoOfTimes );
	}
	osacquire( cerr ) << endl;

	osacquire( cerr ) << "cxt sw\t";
	{
		ContextSwitch dummy( NoOfTimes );				// context switch
	}
	osacquire( cerr ) << "\t" << endl;
} // main

// Local Variables: //
// compile-command: "../../bin/u++ -O2 -nodebug Bench.cc" //
// End: //
