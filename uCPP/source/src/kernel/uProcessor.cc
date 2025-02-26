//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uProcessor.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Mar 14 17:39:15 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Jan  3 09:48:30 2025
// Update Count     : 2216
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
#define __U_PROFILE__
#define __U_PROFILEABLE_ONLY__

#include <uC++.h>
#ifdef __U_PROFILER__
#include <uProfiler.h>
#endif // __U_PROFILER__
#include <uProcessor.h>

#include <uDebug.h>										// access: uDebugWrite
#undef __U_DEBUG_H__									// turn off debug prints

#include <cstring>										// strerror
#include <cerrno>
#include <unistd.h>										// getpid

#include <limits.h>										// PTHREAD_STACK_MIN

#include <sys/syscall.h>								// SYS_exit


using namespace UPP;


uNoCtor<uEventList, false> uProcessor::events;

#if ! defined( __U_MULTI__ )
uEventNode * uProcessor::contextEvent = nullptr;
uCxtSwtchHndlr * uProcessor::contextSwitchHandler = nullptr;

#ifdef __U_PROFILER__
uProfileProcessorSampler * uProcessor::profileProcessorSamplerInstance = nullptr;
#endif // __U_PROFILER__
#endif // __U_MULTI__

#ifdef __U_DEBUG__
#if __U_LOCALDEBUGGER_H__
enum { MinPreemption = 1000 };							// 1 second (milliseconds)
#endif // __U_LOCALDEBUGGER_H__
#endif // __U_DEBUG__


//######################### uProcessorTask #########################


// Safe to make direct accesses through TLS pointer because interrupts disabled in uProcessorKernel::main before
// scheduling uProcessorTask.
void uProcessorTask::main() {
	assert( uKernelModule::uKernelModuleBoot.disableInt && uKernelModule::uKernelModuleBoot.disableIntCnt > 0 );

	// Not a race because both sides (parent/child) set the processor pid, i.e., it is set twice with the same value.

	processor.pid =										// set pid for processor
		#if defined( __U_MULTI__ )
		RealRtn::pthread_self();
		#else
		getpid();
		#endif // __U_MULTI__

	uDEBUGPRT( uDebugPrt( "(uProcessorTask &)%p.main, starting pid:%lu\n", this, processor.pid ); );

	// Although the signal handlers are inherited by each child process, the alarm setting is not.

	processor.setContextSwitchEvent( processor.getPreemption() );

	#if __U_LOCALDEBUGGER_H__
	if ( uLocalDebugger::uLocalDebuggerActive ) {
		uLocalDebugger::uLocalDebuggerInstance->checkPoint();
		uLocalDebugger::uLocalDebuggerInstance->createKernelThread( processor, uThisCluster() );
	} // if
	#endif // __U_LOCALDEBUGGER_H__

	#if defined( __U_MULTI__ )
	#ifdef __U_PROFILER__
	// uniprocessor calls done in uProfilerBoot
	if ( uProfiler::uProfiler_builtinRegisterProcessor ) {
		(*uProfiler::uProfiler_builtinRegisterProcessor)( uProfiler::profilerInstance, bound );
	} // if
	#endif // __U_PROFILER__
	#endif // __U_MULTI_H__

	for ( ;; ) {
		assert( uKernelModule::uKernelModuleBoot.disableInt && uKernelModule::uKernelModuleBoot.disableIntCnt > 0 );
		_Accept( ~uProcessorTask ) {
			uDEBUGPRT( uDebugPrt( "(uProcessorTask &)%p.main, ~uProcessorTask\n", this ); );
			break;
		} or _Accept( setPreemption ) {
			uDEBUGPRT( uDebugPrt( "(uProcessorTask &)%p.main, setPreemption( %d )\n", this, preemption ); );
			processor.setContextSwitchEvent( preemption ); // use it to set the alarm for this processor
			processor.preemption = preemption;
		} or _Accept( setCluster ) {
			uDEBUGPRT( uDebugPrt( "(uProcessorTask &)%p.main, setCluster %p %p\n", this, &processor.getCluster(), cluster ); );

			#if __U_LOCALDEBUGGER_H__
			if ( uLocalDebugger::uLocalDebuggerActive ) uLocalDebugger::uLocalDebuggerInstance->checkPoint();
			#endif // __U_LOCALDEBUGGER_H__

			// Remove the processor from the list of processor that live on this cluster, and add it to the list of
			// processors that live on the new cluster. The move has to be done by the processor itself when it is in a
			// stable state. As well, the processor's task has to be moved to the new cluster but it does not have to be
			// migrated there since it is a bound task.

			// change processor's notion of which cluster it is executing on
			uCluster & prevCluster = processor.getCluster();

			#ifdef __U_PROFILER__
			if ( uProfiler::uProfiler_registerProcessorMigrate ) { // task registered for profiling ?			  
				(*uProfiler::uProfiler_registerProcessorMigrate)( uProfiler::profilerInstance, processor, prevCluster, *cluster );
			} // if
			#endif // __U_PROFILER__

			prevCluster.processorRemove( processor );
			processor.currCluster_ = cluster;
			uKernelModule::uKernelModuleBoot.activeCluster = cluster;
			cluster->processorAdd( processor );
			currCluster_ = cluster;						// change task's notion of which cluster it is executing on

			#if __U_LOCALDEBUGGER_H__
			if ( uLocalDebugger::uLocalDebuggerActive ) uLocalDebugger::uLocalDebuggerInstance->migrateKernelThread( processor, *cluster );
			#endif // __U_LOCALDEBUGGER_H__

			result.signalBlock();
		} // _Accept
	} // for

	#if defined( __U_MULTI__ )
	processor.setContextSwitchEvent( 0 );				// clear the alarm on this processor
	assert( ! processor.contextEvent->listed() );

	#ifdef __U_PROFILER__
	// uniprocessor calls done in uProfilerBoot
	if ( uProfiler::uProfiler_builtinDeregisterProcessor ) {
		(*uProfiler::uProfiler_builtinDeregisterProcessor)( uProfiler::profilerInstance, bound );
	} // if
	#endif // __U_PROFILER__
	#endif // __U_MULTI__

	#if __U_LOCALDEBUGGER_H__
	if ( uLocalDebugger::uLocalDebuggerActive ) uLocalDebugger::uLocalDebuggerInstance->destroyKernelThread( processor );
	#endif // __U_LOCALDEBUGGER_H__

	processor.terminated = true;
} // uProcessorTask::main


void uProcessorTask::setPreemption( unsigned int ms ) {
	preemption = ms;
} // uProcessorTask::setPreemption


void uProcessorTask::setCluster( uCluster & cluster ) {
	uProcessorTask::cluster = &cluster;					// copy arguments
	result.wait();										// wait for result
} // uProcessorTask::setCluster


uProcessorTask::uProcessorTask( uCluster & cluster, uProcessor & processor ) : uBaseTask( cluster, processor ), processor( processor ) {
} // uProcessorTask::uProcessorTask


uProcessorTask::~uProcessorTask() {
	#ifdef __U_MULTI__
	// do not wait for systemProcessor KT as it must return to the shell
  if ( &processor == &uKernelModule::systemProcessor ) return;

	uPid_t pid = processor.getPid();					// pid of underlying KT
	int code;
	for ( ;; ) {
		code = RealRtn::pthread_join( pid, nullptr );	// wait for termination of KT
	  if ( code == 0 ) break;
	  if ( code != EINTR ) break;						// timer interrupt ?
	} // for
	if ( code != 0 ) {
		abort( "(uProcessor &)%p.~uProcessor() : internal error, wait failed for kernel thread %ld, error(%d) %s.",
			   this, (long int)pid, code, strerror( code ) );
	} // if
	#endif // __U_MULTI__
} // uProcessorTask::~uProcessorTask


//######################### uProcessorKernel #########################


extern void heapManagerCtor();
extern void heapManagerDtor();

// Safe to make direct accesses through TLS pointer because Kernel thread just started so no concurrency.
void * uKernelModule::startThread( void * p __attribute__(( unused )) ) {
	#if defined(__U_MULTI__)
	uKernelModuleBoot.ctor();

	// NO DEBUG PRINTS BEFORE THE THREAD REFERENCE IS SET IN CTOR.

	uDEBUGPRT( uDebugPrt( "startthread, child started\n" ); );

	uProcessor & processor = *(uProcessor *)p;
	activeProcessorKernel = &processor.processorKer;

	// initialize thread members

	uKernelModule::uKernelModuleBoot.activeProcessor = &processor;
	uCluster * currCluster = uKernelModule::uKernelModuleBoot.activeProcessor->currCluster_;
	uKernelModule::uKernelModuleBoot.activeCluster = currCluster;
	
	assert( uKernelModule::uKernelModuleBoot.disableInt && uKernelModule::uKernelModuleBoot.disableIntCnt == 1 );

	uKernelModule::uKernelModuleData::disableInterrupts();

	heapManagerCtor();									// initialize heap

	uMachContext::invokeCoroutine( *activeProcessorKernel );

	heapManagerDtor();									// de-initialize heap
	#endif // __U_MULTI__

	uDEBUGPRT( uDebugPrt( "(uKernelModule &).startThread, exiting\n" ); );
	// This line is never reached, but pthreads allows the thread function to return a value, and gcc warns otherwise.
	return nullptr;
} // uKernelModule::startThread


// Safe to make direct accesses through TLS pointer because SCHEDULE_BODY disabled interrupts.
inline void uProcessorKernel::taskIsBlocking() {
	uBaseTask & task = uThisTask();						// optimization
	if ( task.getState() != uBaseTask::Terminate ) {
		task.setState( uBaseTask::Blocked );
	} // if
} // uProcessorKernel::taskIsBlocking


void uProcessorKernel::scheduleInternal() {
	assert( ! uThisTask().readyRef_.listed() );
	assert( ! uKernelModule::uKernelModuleBoot.disableIntSpin );

	taskIsBlocking();

	kind = 0;
	taskCxtSw();										// not resume because entering kernel
} // uProcessorKernel::scheduleInternal


void uProcessorKernel::scheduleInternal( uSpinLock * lock ) {
	assert( ! uThisTask().readyRef_.listed() );
	assert( uKernelModule::uKernelModuleBoot.disableIntSpinCnt == 1 );

	taskIsBlocking();

	kind = 1;
	prevLock = lock;
	taskCxtSw();										// not resume because entering kernel
} // uProcessorKernel::scheduleInternal


void uProcessorKernel::scheduleInternal( uBaseTask * task ) {
	// SKULLDUGGERY: uBootTask is on ready queue for first entry into the kernel.
	assert( &uThisTask() != (uBaseTask *)uKernelModule::bootTask ? ! uThisTask().readyRef_.listed() : true );
	assert( ! uKernelModule::uKernelModuleBoot.disableIntSpin );

	if ( task != &uThisTask() ) {
		taskIsBlocking();
	} // if

	kind = 2;
	nextTask = task;
	taskCxtSw();										// not resume because entering kernel
} // uProcessorKernel::scheduleInternal


void uProcessorKernel::scheduleInternal( uSpinLock * lock, uBaseTask * task ) {
	assert( ! uThisTask().readyRef_.listed() );
	assert( uKernelModule::uKernelModuleBoot.disableIntSpinCnt == 1 );

	taskIsBlocking();

	kind = 3;
	prevLock = lock;
	nextTask = task;
	taskCxtSw();										// not resume because entering kernel
} // uProcessorKernel::scheduleInternal


#define SCHEDULE_BODY(parm...) \
	uKernelModule::uKernelModuleData::disableInterrupts(); \
	activeProcessorKernel->scheduleInternal( parm ); \
	uKernelModule::uKernelModuleData::enableInterrupts();

#ifdef __U_PROFILER__
#define SCHEDULE_PROFILE() \
	/* Are uC++ kernel memory allocation hooks active? */ \
	if ( uThisTask().profileActive && uProfiler::uProfiler_postallocateMetricMemory ) { \
		(*uProfiler::uProfiler_postallocateMetricMemory)( uProfiler::profilerInstance, uThisTask() ); \
	}
#else
#define SCHEDULE_PROFILE()
#endif // __U_PROFILER__

void uProcessorKernel::schedule() {
	SCHEDULE_BODY();
	SCHEDULE_PROFILE();
} // uProcessorKernel::schedule


void uProcessorKernel::schedule( uSpinLock * lock ) {
	SCHEDULE_BODY( lock );
	SCHEDULE_PROFILE();
} // uProcessorKernel::schedule


void uProcessorKernel::schedule( uBaseTask * task ) {
	SCHEDULE_BODY( task );
	SCHEDULE_PROFILE();
} // uProcessorKernel::schedule


void uProcessorKernel::schedule( uSpinLock * lock, uBaseTask * task ) {
	SCHEDULE_BODY( lock, task );
	SCHEDULE_PROFILE();
} // uProcessorKernel::schedule


void uProcessorKernel::onBehalfOfUser() {
	switch( kind ) {
	  case 0:
		break;
	  case 1:
		prevLock->release();
		break;
	  case 2:
		nextTask->wake();
		break;
	  case 3:
		prevLock->release();
		nextTask->wake();
		break;
	  default:
		abort( "(uProcessorKernel &)%p.onBehalfOfUser : internal error, schedule kind:%d.", this, kind );
		break;
	} // switch
} // uProcessorKernel::onBehalfOfUser


void uProcessorKernel::setTimer( uDuration dur ) {
	uDEBUGPRT(
		char buffer[256];
		uDebugPrtBuf( buffer, "uProcessorKernel::setTimer1, dur:%lld\n", dur.nanoseconds() );
	);

  if ( dur <= 0 ) return;								// if duration is zero or negative, it has already past

	// For now, write only code for non-posix timer. When posix timer is available use timer_create and timer_settimer.

	timeval conv = dur;
	// avoid rounding to zero for small nanosecond durations to prevent disabling the timer
	if ( conv.tv_sec == 0 && conv.tv_usec == 0 && dur.nanoseconds() != 0 ) conv.tv_usec = 1;
	itimerval it;
	it.it_value = conv;									// fill in the value to the next expiry
	it.it_interval.tv_sec = 0;							// not periodic
	it.it_interval.tv_usec = 0;
	#ifdef __U_STATISTICS__
	uFetchAdd( Statistics::setitimer, 1 );
	#endif // __U_STATISTICS__
	setitimer( ITIMER_REAL, &it, nullptr );				// set the alarm clock to go off
} // uProcessorKernel::setTimer


void uProcessorKernel::setTimer( uTime time ) {
	uDEBUGPRT(
		char buffer[256];
		uDebugPrtBuf( buffer, "uProcessorKernel::setTimer2, time:%lld\n", time.nanoseconds() );
	);
  if ( time <= uTime() ) return;						// if time is zero or negative, it is invalid

	// The time parameter is always in real-time (not virtual time)

	uDuration dur = time - uClock::currTime();
  if ( dur <= 0 ) return;								// if duration is zero or negative, it has already past
	setTimer( dur );
} // uProcessorKernel::setTimer


#if ! defined( __U_MULTI__ )
// Safe to call setContextSwitchEvent, which makes direct accesses through TLS pointer, because only called from
// preemption-safe locations: uProcessorKernel::main
void uProcessorKernel::nextProcessor( uProcessorDL *& currProc, uProcessorDL * cycleStart ) {
	// Get next processor to execute.

	unsigned int uPrevPreemption = uThisProcessor().getPreemption(); // remember previous preemption value
	uDEBUGPRT( uDebugPrt( "(uProcessorKernel &)%p.nextProcessor, from processor %p on cluster %.256s (%p) with time slice %d\n",
						  this, &uThisProcessor(), uThisProcessor().currCluster->getName(), uThisProcessor().currCluster, uThisProcessor().getPreemption() ); );
	do {												// ignore deleted processors
		currProc = uKernelModule::globalProcessors->succ( currProc );
		if ( currProc == nullptr ) {					// make list appear circular
			currProc = uKernelModule::globalProcessors->head(); // restart at beginning of list
		} // if
	} while ( currProc != cycleStart &&					// stop searching if all processors in the cycle have been checked
			  // ignore a processor if it is terminated or has no tasks to execute on either of its ready queues
			  ( currProc->processor().terminated || ( currProc->processor().external.empty() && currProc->processor().currCluster_->readyQueueEmpty() ) ) );

	if ( currProc->processor().terminated ) {
		currProc = &(uKernelModule::systemProcessor->globalRef);
	} // if

	uKernelModule::uKernelModuleBoot.activeProcessor = &(currProc->processor());
	uCluster * currCluster = uKernelModule::uKernelModuleBoot.activeProcessor->currCluster_;
	uKernelModule::uKernelModuleBoot.activeCluster = currCluster;
	uDEBUGPRT( uDebugPrt( "(uProcessorKernel &)%p.nextProcessor, to processor %p on cluster %.256s (%p) with time slice %d\n",
						  this, &uThisProcessor(), uThisProcessor().currCluster->getName(), uThisProcessor().currCluster, uThisProcessor().getPreemption() ); );

	// The time slice must be reset or some programs do not work.

	if ( uThisProcessor().getPreemption() != uPrevPreemption ) {
		uThisProcessor().setContextSwitchEvent( uThisProcessor().getPreemption() );
	} // if
} // nextProcessor
#endif // ! __U_MULTI__


// Safe to make direct accesses through TLS pointer because SCHEDULE_BODY disabled interrupts.
void uProcessorKernel::main() {
	uDEBUGPRT( uDebugPrt( "(uProcessorKernel &)%p.main, child is born\n", this ); );

	#if ! defined( __U_MULTI__ )
	// SKULLDUGGERY: The system processor in not on the global list until the processor task runs, so explicitly set the
	// current processor to the system processor.

	uProcessorDL * currProc = &(uKernelModule::systemProcessor->globalRef);
	uProcessorDL * cycleStart = nullptr;
	bool & okToSelect = uCluster::NBIO.okToSelect;
	uBaseTask *& IOPoller = uCluster::NBIO.IOPoller;
	#endif // ! __U_MULTI__

	#if defined( __U_MULTI__ )
	// Optimize out many TLS calls to get the current processor. Uniprocessor does not use TLS.
	uProcessor * processor = &uThisProcessor();			// multiprocessor: processor and kernel are 1-to-1
	processor->procTask->currCoroutine_ = this;
	#else // UNIPROCESSOR
	uThisProcessor().procTask->currCoroutine_ = this;
	#endif // __U_MULTI__

	uBaseTask * readyTask;

	for ( unsigned int spin = 0;; ) {
		#if ! defined( __U_MULTI__ )
		uProcessor * processor = &uThisProcessor();		// uniprocessor: processor and kernel are N-to-1
		#endif // ! __U_MULTI__

		// Advance the spin counter now to detect if a task is executed.

		spin += 1;

		if ( ! processor->external.empty() ) {			// check processor specific ready queue
			// Only this processor removes from this ready queue so no other processor can remove this task after it has
			// been seen.

			readyTask = &processor->external.dropHead()->task();
			uKernelModule::uKernelModuleBoot.activeTask = readyTask;
			readyTask->currCoroutine_ = readyTask;		// manually reset current coroutine

			#ifdef __U_DEBUG__
			void * SP = ((uContext_t *)readyTask->context_)->SP;
			#endif // __U_DEBUG__
			uDEBUGPRT(
				uDebugPrt( "(uProcessorKernel &)%p.main, scheduling(1) bef: task %.256s (%p) (limit:%p,stack:%p,base:%p) from cluster:%.256s (%p) on processor:%p, %d,%d,%d,%d,%d,%d\n",
						   this, readyTask->currCoroutine->name, readyTask->currCoroutine,
						   readyTask->limit, SP, readyTask->base,
						   processor->currCluster->getName(), processor->currCluster, &processor,
						   uKernelModule::uKernelModuleBoot.disableInt,
						   uKernelModule::uKernelModuleBoot.disableIntCnt,
						   uKernelModule::uKernelModuleBoot.disableIntSpin,
						   uKernelModule::uKernelModuleBoot.disableIntSpinCnt,
						   uKernelModule::uKernelModuleBoot.RFpending,
						   uKernelModule::uKernelModuleBoot.RFinprogress
					);
			);
			assert( readyTask->limit_ < SP && SP < readyTask->base_ );

			// SKULLDUGGERY: The processor task is part of the kernel, and therefore, must execute as uninterruptible
			// code. By incrementing the interrupt counter here, the decrement when the processor task is scheduled
			// leaves the processor task's execution in the kernel with regard to interrupts. This assumes the
			// disableInt flag is set already.

			assert( uKernelModule::uKernelModuleBoot.disableInt && uKernelModule::uKernelModuleBoot.disableIntCnt > 0 );
			uKernelModule::uKernelModuleData::disableInterrupts();

			#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::user_context_switches, 1 );
			#endif // __U_STATISTICS__

			uSwitch( context_, readyTask->currCoroutine_->context_ );

			uKernelModule::uKernelModuleData::enableInterrupts();
			assert( uKernelModule::uKernelModuleBoot.disableInt && uKernelModule::uKernelModuleBoot.disableIntCnt > 0 );

			readyTask->currCoroutine_ = this;			// manually reset current coroutine
			assert( limit_ <= stackPointer() && stackPointer() <= base_ ); // checks uProcessorKernel

			uDEBUGPRT(
				uDebugPrt( "(uProcessorKernel &)%p.main, scheduling(1) aft: task %.256s (%p) (limit:%p,stack:%p,base:%p) from cluster:%.256s (%p) on processor:%p, %d,%d,%d,%d,%d,%d\n",
						   this, readyTask->currCoroutine->name, readyTask->currCoroutine,
						   readyTask->limit, readyTask->stackPointer(), readyTask->base,
						   processor->currCluster->getName(), processor->currCluster, &processor,
						   uKernelModule::uKernelModuleBoot.disableInt,
						   uKernelModule::uKernelModuleBoot.disableIntCnt,
						   uKernelModule::uKernelModuleBoot.disableIntSpin,
						   uKernelModule::uKernelModuleBoot.disableIntSpinCnt,
						   uKernelModule::uKernelModuleBoot.RFpending,
						   uKernelModule::uKernelModuleBoot.RFinprogress
				);
			);

			spin = 0;									// set number of spins back to zero
			onBehalfOfUser();							// execute code on scheduler stack on behalf of user

			if ( processor->terminated ) {
				#ifdef __U_MULTI__
				if ( processor != &uKernelModule::systemProcessor ) break;
				// If control reaches here, the boot task must be the only task on the system-cluster ready-queue, and
				// it must be restarted to finish the close down.
				#else
				uDEBUGPRT( uDebugPrt( "(uProcessorKernel &)%p.main termination, currProc:%p, uThisProcessor:%p\n", this, &(currProc->processor()), &processor ); );
				// In the uniprocessor case, only terminate the processor kernel when the processor task for the system
				// processor is deleted; otherwise the program stops when the first processor is deleted.

				if ( processor != &uKernelModule::systemProcessor ) {
					// Get next processor to execute because the current one just terminated.

					nextProcessor( currProc, cycleStart );
				} // if
				#endif // __U_MULTI__
			} // if
		} // if

		readyTask = &(processor->currCluster_->readyQueueTryRemove());

		if ( readyTask != nullptr ) {					// ready queue not empty, schedule that task

			assert( ! readyTask->readyRef_.listed() );
			uKernelModule::uKernelModuleBoot.activeTask = readyTask;

			#ifdef __U_DEBUG__
			void * SP = ((uContext_t *)readyTask->currCoroutine_->context_)->SP;
			#endif // __U_DEBUG__
			uDEBUGPRT(
				uDebugPrt( "(uProcessorKernel &)%p.main, scheduling(2) bef: task %.256s (%p) (limit:%p,stack:%p,base:%p) from cluster:%.256s (%p) on processor:%p, %d,%d,%d,%d,%d,%d\n",
						   this, readyTask->currCoroutine->name, readyTask->currCoroutine,
						   readyTask->currCoroutine->limit, SP, readyTask->currCoroutine->base,
						   processor->currCluster->getName(), processor->currCluster, &processor,
						   uKernelModule::uKernelModuleBoot.disableInt,
						   uKernelModule::uKernelModuleBoot.disableIntCnt,
						   uKernelModule::uKernelModuleBoot.disableIntSpin,
						   uKernelModule::uKernelModuleBoot.disableIntSpinCnt,
						   uKernelModule::uKernelModuleBoot.RFpending,
						   uKernelModule::uKernelModuleBoot.RFinprogress
				);
			);
			assert( readyTask == (uBaseTask *)uKernelModule::bootTask ? true : readyTask->currCoroutine_->limit_ < SP && SP < readyTask->currCoroutine_->base_ );
			assert( uKernelModule::uKernelModuleBoot.disableInt && uKernelModule::uKernelModuleBoot.disableIntCnt > 0 );

			#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::user_context_switches, 1 );
			#endif // __U_STATISTICS__

			uSwitch( context_, readyTask->currCoroutine_->context_ );

			assert( uKernelModule::uKernelModuleBoot.disableInt && uKernelModule::uKernelModuleBoot.disableIntCnt > 0 );
			// activeTask is set to the uProcessorTask and MUST stay set until another task is selected to ensure that
			// errno works correctly should a SIGALRM occurs while in the kernel.
			processor = &uThisProcessor();				// processor may have migrated
			uKernelModule::uKernelModuleBoot.activeTask = processor->procTask;
			assert( limit_ <= stackPointer() && stackPointer() <= base_ ); // checks uProcessorKernel

			uDEBUGPRT(
				uDebugPrt( "(uProcessorKernel &)%p.main, scheduling(2) aft: task %.256s (%p) (limit:%p,stack:%p,base:%p) from cluster:%.256s (%p) on processor:%p, %d,%d,%d,%d,%d,%d\n",
						   this, readyTask->currCoroutine->name, readyTask->currCoroutine,
						   readyTask->currCoroutine->limit, readyTask->currCoroutine->stackPointer(), readyTask->currCoroutine->base,
						   processor->currCluster->getName(), processor->currCluster, &processor,
						   uKernelModule::uKernelModuleBoot.disableInt,
						   uKernelModule::uKernelModuleBoot.disableIntCnt,
						   uKernelModule::uKernelModuleBoot.disableIntSpin,
						   uKernelModule::uKernelModuleBoot.disableIntSpinCnt,
						   uKernelModule::uKernelModuleBoot.RFpending,
						   uKernelModule::uKernelModuleBoot.RFinprogress
				);
			);

			#ifdef __U_MULTI__
			spin = 0;									// set number of spins back to zero
			#else
			// Poller task does not count as an executed task, if its last execution found no I/O and this processor's
			// ready queue is empty. Check before calling onBehalfOfUser, because IOPoller may put itself back on the
			// ready queue, which makes the ready queue appear non-empty.

			if ( readyTask != IOPoller || uCluster::NBIO.descriptors != 0 || ! processor->currCluster_->readyQueueEmpty() ) {
				spin = 0;								// set number of spins back to zero
			} // if
			#endif // __U_MULTI__
			onBehalfOfUser();							// execute code on scheduler stack on behalf of user
		} // if

		#ifdef __U_MULTI__
		if ( ! uKernelModule::uKernelModuleBoot.RFinprogress && uKernelModule::uKernelModuleBoot.RFpending ) { // run roll forward ?
			uKernelModule::rollForward( true );
		} // if

		if ( uThisCluster().numProcessors > 1 ) {		// only perform if there is processor competition
			for ( volatile unsigned int d = 0; d <		// delay so not pounding on ready-queue lock 
				  #if defined( __i386__ ) || defined( __x86_64__ )
				  100;
				  #else
				  10000;
				  #endif // __i386__ || __x86_64__
				  d += 1 ) {
				uPause();
			} // for
		} // if

		if ( spin > processor->getSpin() ) {			// spin expired ?
			processor->currCluster_->processorPause();	// put processor to sleep

			if ( processor != &uKernelModule::systemProcessor ) {
				uKernelModule::uKernelModuleBoot.RFpending = false;	// no pending roll forward
			} // if
			spin = 0;									// set number of spins back to zero
		} // if

// 		if ( spin % 200 == 0 ) {
// 			sched_yield();								// release CPU so someone else can execute
// #ifdef __U_STATISTICS__
// 			uFetchAdd( Statistics::kernel_thread_yields, 1 );
// #endif // __U_STATISTICS__
// 		} // if

#else // UNIPROCESSOR

		// A cycle starts when a processor executes no tasks. If the cycle completes and no task has executed, deadlock
		// has occurred unless there are pending I/O tasks. If there are pending I/O tasks, the I/O poller task pauses
		// the UNIX process at the "select".

		uDEBUGPRT( uDebugPrt( "(uProcessorKernel &)%p.main, cycleStart:%p, currProc:%.256s (%p), spin:%d, readyTask:%p, IOPoller:%p\n",
							  this, cycleStart, currProc->processor().currCluster->getName(), currProc, spin, readyTask, IOPoller ); );

		if ( cycleStart == currProc && spin != 0 ) {
			#if __U_LOCALDEBUGGER_H__
			uDEBUGPRT(
				uDebugPrt( "(uProcessorKernel &)%p.main, uLocalDebuggerInstance:%p, IOPoller: %.256s (%p), dispatcher:%p, debugger_blocked_tasks:%d, uPendingIO.head:%p, uPendingIO.tail:%p\n",
						   this, uLocalDebugger::uLocalDebuggerInstance,
						   IOPoller != nullptr ? IOPoller->getName() : "no I/O poller task", IOPoller,
						   uLocalDebugger::uLocalDebuggerInstance != nullptr ? uLocalDebugger::uLocalDebuggerInstance->dispatcher : nullptr,
						   uLocalDebugger::uLocalDebuggerInstance != nullptr ? uLocalDebugger::uLocalDebuggerInstance->debugger_blocked_tasks : 0,
						   uCluster::NBIO->uPendingIO.head(), uCluster::NBIO->uPendingIO.tail()
				);
			);
			#endif // __U_LOCALDEBUGGER_H__
			if ( IOPoller != nullptr					// I/O poller task ?
				 #if __U_LOCALDEBUGGER_H__
				 && (
					 uLocalDebugger::uLocalDebuggerInstance == nullptr || // local debugger initialized ?
					 IOPoller != (uBaseTask *)uLocalDebugger::uLocalDebuggerInstance->dispatcher || // I/O poller the local debugger reader ?
					 uLocalDebugger::uLocalDebuggerInstance->debugger_blocked_tasks != 0 || // any tasks debugger blocked ?
					 uCluster::NBIO->uPendingIO.head() != uCluster::NBIO->uPendingIO.tail() // any other tasks waiting for I/O ?
					 )
				 #endif // __U_LOCALDEBUGGER_H__
				) {
				uDEBUGPRT( uDebugPrt( "(uProcessorKernel &)%p.main, poller blocking\n", this ); );
				okToSelect = true;			// tell poller it is ok to call UNIX select, reset in uPollIO
			} else if ( processor->events->userEventPresent() ) { // tasks sleeping, except system task  ?
				uDEBUGPRT( uDebugPrt( "(uProcessorKernel &)%p.main, sleeping with pending events\n", this ); );
				if ( ! uKernelModule::uKernelModuleBoot.RFinprogress && uKernelModule::uKernelModuleBoot.RFpending ) { // need to start roll forward ?
					uKernelModule::rollForward( true );
				} else {
					processor->currCluster_->processorPause(); // put processor to sleep
				} // if
			} else {
				// locking is unnecessary here
				uDebugPrt2( "Clusters and tasks present at deadlock:\n" );
				uSeqIter<uClusterDL> ci;
				uClusterDL * cr;
				for ( ci.over( *uKernelModule::globalClusters ); ci >> cr; ) {
					uCluster * cluster = &cr->cluster();
					uDebugPrt2( "%.256s (%p)\n", cluster->getName(), cluster );
					uDebugPrt2( "\ttasks:\n" );
					uBaseTaskDL * bt;
					for ( uSeqIter<uBaseTaskDL> iter( cluster->tasksOnCluster ); iter >> bt; ) {
						uBaseTask * task = &bt->task();
						uDebugPrt2( "\t\t %.256s (%p)\n", task->getName(), task );
					} // for
				} // for
				abort( "No ready or pending tasks.\n"
					   "Possible cause is tasks are in a synchronization or mutual exclusion deadlock." );
			} // if
		} // if

		if ( spin == 0 ) {								// task executed ?
			cycleStart = currProc;						// mark new start for cycle
		} // if

		// Get next processor to execute.

		nextProcessor( currProc, cycleStart );
		#endif // __U_MULTI__
	} // for

	// ACCESS NO KERNEL DATA STRUCTURES AFTER THIS POINT BECAUSE THEY MAY NO LONGER EXIST.

	uDEBUGPRT( uDebugPrt( "(uProcessorKernel &)%p.main, exiting\n", this ); );

	// If available, wake another processor on this cluster, as this one is terminating.
	uThisCluster().makeProcessorActive();

//#if defined( __U_MULTI__ )
//	// Cannot call RealRtn::pthread_exit( nullptr ) because it performs a handler cleanup that raises an exception on
//	// Linux. The exception attempt to acquire a pthread_mutex_lock that calls a uOwnerLock, which cannot be called from
//	// within the kernel.
//	RealRtn::pthread_exit( nullptr );
//	assert( false );
//	syscall( SYS_exit, 0 );								// terminate kernel thread not process
//#endif // __U_MULTI__
} // uProcessorKernel::main


uProcessorKernel::uProcessorKernel() : uBaseCoroutine( PTHREAD_STACK_MIN > __U_DEFAULT_STACK_SIZE__ ? (unsigned int)PTHREAD_STACK_MIN : __U_DEFAULT_STACK_SIZE__ ) {
} // uProcessorKernel::uProcessorKernel

uProcessorKernel::~uProcessorKernel() {
	uDEBUGPRT( uDebugPrt( "(uProcessorKernel &)%p.~uProcessorKernel, exiting\n", this ); );
} // uProcessorKernel::~uProcessorKernel


//######################### uProcessor #########################


void uProcessor::createProcessor( uCluster & cluster, bool detached, int ms, int spin ) {
	uDEBUGPRT( uDebugPrt( "(uProcessor &)%p.createProcessor, on cluster %.256s (%p)\n", this, cluster.getName(), &cluster ); );

	#ifdef __U_DEBUG__
	#if __U_LOCALDEBUGGER_H__
	if ( ms == 0 ) {									// 0 => infinity, reset to minimum preemption second for local debugger
		ms = MinPreemption;								// approximate infinity
	} // if
	#ifdef __U_MULTI__
	debugIgnore = false;
	#else
	debugIgnore = true;
	#endif // __U_MULTI__
	#endif // __U_LOCALDEBUGGER_H__
	#endif // __U_DEBUG__

	currCluster_ = &cluster;
	uProcessor::detached = detached;
	preemption = ms;
	uProcessor::spin = spin;

	#ifdef __U_MULTI__
	contextSwitchHandler = new uCxtSwtchHndlr( *this );
	contextEvent = new uEventNode( *contextSwitchHandler );

	#ifdef __U_PROFILER__
	profileProcessorSamplerInstance = nullptr;
	#endif // __U_PROFILER__
	#endif // __U_MULTI__

	terminated = false;
	currCluster_->processorAdd( *this );

	uKernelModule::globalProcessorLock->acquire();		// add processor to global processor list.
	uKernelModule::globalProcessors->addTail( &(globalRef) );
	uKernelModule::globalProcessorLock->release();

	procTask = new uProcessorTask( cluster, *this );
} // uProcessor::createProcessor


uProcessor::uProcessor( uCluster & cluster, double ) : idleRef( *this ), processorRef( *this ), globalRef( *this ) {
	createProcessor( cluster, false, 0, 0 );			// no preemption or spinning on the system processor
} // uProcessor::uProcessor


uProcessor::uProcessor( unsigned int ms, unsigned int spin ) : idleRef( *this ), processorRef( *this ), globalRef( *this ) {
	createProcessor( uThisCluster(), false, ms, spin );
	#if defined( __U_MULTI__ )
	#ifdef __U_PROFILER__
	// Register the processor before creating the processor task because the processor task uses the profiler
	// sampler. Uniprocessor calls done in uProfilerBoot.
	if ( uProfiler::uProfiler_registerProcessor ) {
		(*uProfiler::uProfiler_registerProcessor)( uProfiler::profilerInstance, *this );
	} // if
	#endif // __U_PROFILER__
	#endif // __U_MULTI_H__
	uThisProcessor().fork( this );						// processor executing this declaration forks the UNIX process
} // uProcessor::uProcessor


uProcessor::uProcessor( bool detached, unsigned int ms, unsigned int spin ) : idleRef( *this ), processorRef( *this ), globalRef( *this ) {
	createProcessor( uThisCluster(), detached, ms, spin );
	
	#if defined( __U_MULTI__ )
	#ifdef __U_PROFILER__
	// see above
	if ( uProfiler::uProfiler_registerProcessor ) {
		(*uProfiler::uProfiler_registerProcessor)( uProfiler::profilerInstance, *this );
	} // if
	#endif // __U_PROFILER__
	#endif // __U_MULTI_H__
	uThisProcessor().fork( this );						// processor executing this declaration forks the UNIX process
} // uProcessor::uProcessor


uProcessor::uProcessor( uCluster & clus, unsigned int ms, unsigned int spin ) : idleRef( *this ), processorRef( *this ), globalRef( *this ) {
	createProcessor( clus, false, ms, spin );
	#if defined( __U_MULTI__ )
	#ifdef __U_PROFILER__
	// see above
	if ( uProfiler::uProfiler_registerProcessor ) {
		(*uProfiler::uProfiler_registerProcessor)( uProfiler::profilerInstance, *this );
	} // if
	#endif // __U_PROFILER__
	#endif // __U_MULTI_H__
	uThisProcessor().fork( this );						// processor executing this declaration forks the UNIX process
} // uProcessor::uProcessor


uProcessor::uProcessor( uCluster & clus, bool detached, unsigned int ms, unsigned int spin ) : idleRef( *this ), processorRef( *this ), globalRef( *this ) {
	createProcessor( clus, detached, ms, spin );
	#if defined( __U_MULTI__ )
	#ifdef __U_PROFILER__
	// see above
	if ( uProfiler::uProfiler_registerProcessor ) {
		(*uProfiler::uProfiler_registerProcessor)( uProfiler::profilerInstance, *this );
	} // if
	#endif // __U_PROFILER__
	#endif // __U_MULTI_H__
	uThisProcessor().fork( this );						// processor executing this declaration forks the UNIX process
} // uProcessor::uProcessor


uProcessor::~uProcessor() {
	uDEBUGPRT( uDebugPrt( "(uProcessor &)%p.~uProcessor\n", this ); );

	delete procTask;

	#if defined( __U_MULTI__ )
	#ifdef __U_PROFILER__
	// Deregister the processor after deleting the processor task because the processor task uses the profiler sampler.
	if ( uProfiler::uProfiler_deregisterProcessor ) {
		(*uProfiler::uProfiler_deregisterProcessor)( uProfiler::profilerInstance, *this );
	} // if
	#endif // __U_PROFILER__
	#endif // __U_MULTI_H__

	// Remove processor from global processor list. It is removed here because the next action after this is the
	// termination of the UNIX process in uProcessorKernel. Therefore, even if the application is aborted and the
	// process is not on the list of UNIX processes for this application, the current UNIX process will terminate
	// itself.  It cannot be removed in uProcessKernel because the uProcessor storage may be freed already by another
	// processor.

	uKernelModule::globalProcessorLock->acquire();		// remove processor from global processor list.
	uKernelModule::globalProcessors->remove( &(globalRef) );
	uKernelModule::globalProcessorLock->release();

	currCluster_->processorRemove( *this );
	#ifdef __U_MULTI__
	delete contextEvent;
	delete contextSwitchHandler;
	if ( uKernelModule::systemTask == nullptr ) {
		events.dtor();
	} // if
	#endif // __U_MULTI__
} // uProcessor::~uProcessor


void uProcessor::fork( uProcessor * processor ) {
	#ifdef __U_MULTI__
	#ifdef __U_DEBUG__
	uKernelModule::initialized = false;
	#endif // __U_DEBUG__

	int ret;
	pthread_attr_t attr;
	// SIGALRM must only be caught by the system processor
	sigset_t old_mask;
	if ( &uThisCluster() == &uKernelModule::systemCluster ) {
		// Child kernel-thread inherits the signal mask from the parent kernel-thread. So one special case for the
		// system processor creating the user processor => toggle the blocking SIGALRM on system processor, create user
		// processor, and toggle back (below) previous signal mask of the system processor.

		sigset_t new_mask;
		sigemptyset( &new_mask );
		sigemptyset( &old_mask );
		sigaddset( &new_mask, SIGALRM );
		#ifdef __U_PROFILER__
		sigaddset( &new_mask, SIGVTALRM );
		#if defined( __U_HW_OVFL_SIG__ )
		sigaddset( &mask, __U_HW_OVFL_SIG__ );
		#endif // __U_HW_OVFL_SIG__
		#endif // __U_PROFILER__
		if ( sigprocmask( SIG_BLOCK, &new_mask, &old_mask ) == -1 ) {
			abort( "internal error, sigprocmask" );
		} // if
		assert( ! sigismember( &old_mask, SIGALRM ) );
	} // if

	ret = RealRtn::pthread_attr_init( &attr );
	if ( ret ) {
		abort( "(uProcessorKernel &)%p.fork() : internal error, pthread_attr_init failed, error(%d) %s.", this, ret, strerror( ret ) );
	} // if
	assert( processor->processorKer.stackSize() >= PTHREAD_STACK_MIN );
	ret = RealRtn::pthread_attr_setstack( &attr, processor->processorKer.limit_, processor->processorKer.stackSize() );
	if ( ret ) {
		abort( "(uProcessorKernel &)%p.fork() : internal error, pthread_attr_setstack failed, error(%d) %s.", this, ret, strerror( ret ) );
	} // if
	ret = RealRtn::pthread_create( &processor->pid, &attr, uKernelModule::startThread, processor );
	if ( ret ) {
		abort( "(uProcessorKernel &)%p.fork() : internal error, pthread_create failed, error(%d) %s.", this, ret, strerror( ret ) );
	} // if

	// Toggle back previous signal mask of system processor.
	if ( &uThisCluster() == &uKernelModule::systemCluster ) {
		if ( sigprocmask( SIG_SETMASK, &old_mask, nullptr ) == -1 ) {
			abort( "internal error, sigprocmask" );
		} // if
	} // if

	#ifdef __U_DEBUG__
	uKernelModule::initialized = true;
	#endif // __U_DEBUG__

	#else
	processor->pid = pid;
	#endif // __U_MULTI__
} // uProcessor::fork


// Safe to make direct accesses through TLS pointer because only called from preemption-safe locations:
// uCluster::processorPause, uNBIO::pollIO, uProcessorTask::main, 
void uProcessor::setContextSwitchEvent( uDuration duration ) {
	assert( uKernelModule::uKernelModuleBoot.disableInt && uKernelModule::uKernelModuleBoot.disableIntCnt == 1 );
	assert( duration >= 0 );

	if ( ! contextEvent->listed() && duration != 0 ) {	// first context switch event ?
		contextEvent->alarm = uClock::currTime() + duration;
		contextEvent->period = duration;
		contextEvent->add();
	} else if ( duration > 0 && contextEvent->period != duration ) { // if event is different from previous ? change it
		contextEvent->remove();
		contextEvent->alarm = uClock::currTime() + duration;
		contextEvent->period = duration;
		contextEvent->add();
	} else if ( duration == 0 && contextEvent->alarm != uTime() ) { // zero duration and current CS is nonzero ?
		contextEvent->remove();
		contextEvent->alarm = uTime();
		contextEvent->period = 0;
	} else {
		// => no preemption, and event not added to event list.
	} // if
}; // uProcessor::setContextSwitchEvent


void uProcessor::setContextSwitchEvent( int msecs ) {
	setContextSwitchEvent( uDuration( msecs / 1000L, msecs % 1000L * ( TIMEGRAN / 1000L ) ) ); // convert msecs to uDuration type
} // uProcessor::setContextSwitchEvent


uCluster & uProcessor::setCluster( uCluster & cluster ) {
  if ( &cluster == &this->getCluster() ) return cluster; // trivial case

	uCluster &prev = cluster;
	procTask->setCluster( cluster );					// operation must be done by the processor itself
	return prev;
} // uProcessor::setCluster


unsigned int uProcessor::setPreemption( unsigned int ms ) {
	#ifdef __U_DEBUG__
	#if __U_LOCALDEBUGGER_H__
	if ( ms == 0 ) {									// 0 => infinity, reset to minimum preemption second for local debugger
		ms = MinPreemption;								// approximate infinity
	} // if
	#endif // __U_LOCALDEBUGGER_H__
	#endif // __U_DEBUG__

	int prev = preemption;
	procTask->setPreemption( ms );

	// Asynchronous with regard to other processors, synchronous with regard to this processor.

	if ( &uThisProcessor() == this ) {
		uThisTask().yield();
	} // if
	return prev;
} // uProcessor::setPreemption


#if defined( __U_AFFINITY__ )
void uProcessor::setAffinity( const cpu_set_t & mask ) {
	#if defined( __U_MULTI__ )
	int rcode = RealRtn::pthread_setaffinity_np( pid, sizeof(cpu_set_t), &mask );
	if ( rcode != 0 ) {
		errno = rcode;
	#else
	if ( sched_setaffinity( pid, sizeof(cpu_set_t), &mask ) != 0 ) {
	#endif // __U_MULTI__
		abort( "(uProcessor &)%p.setAffinity() : internal error, could not set processor affinity, error(%d) %s.", this, errno, strerror( errno ) );
	} // if
} // uProcessor::setAffinity

void uProcessor::setAffinity( unsigned int cpu ) {
	cpu_set_t mask;
	CPU_ZERO( &mask );
	CPU_SET( cpu, &mask );
	setAffinity( mask );
} // uProcessor::setAffinity


void uProcessor::getAffinity( cpu_set_t & mask ) {
	#if defined( __U_MULTI__ )
	int rcode = RealRtn::pthread_getaffinity_np( pid, sizeof(cpu_set_t), &mask );
	if ( rcode ) {
		errno = rcode;
	#else
		if ( sched_getaffinity( pid, sizeof(cpu_set_t), &mask ) != 0 ) {
	#endif // __U_MULTI__
			abort( "(uProcessor &)%p.getAffinity() : internal error, could not set processor affinity, error(%d) %s.", this, errno, strerror( errno ) );
		} // if
} // uProcessor::getAffinity

int uProcessor::getAffinity() {
	cpu_set_t mask;
	getAffinity( mask );
	// There should only be one bit set as this routine matches with setting a single CPU.
	for ( unsigned int i = 0;; i += 1 ) {
		if ( i >= CPU_SETSIZE ) return -1;
		if ( CPU_ISSET( i, &mask ) ) return i;
	} // for
} // uProcessor::getAffinity
#endif // __U_AFFINITY__


// Local Variables: //
// compile-command: "make install" //
// End: //
