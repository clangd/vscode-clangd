//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uCluster.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Mar 14 17:34:24 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu May 11 21:09:04 2023
// Update Count     : 639
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
#include <uIOcntl.h>
#ifdef __U_PROFILER__
#include <uProfiler.h>
#endif // __U_PROFILER__
//#include <uDebug.h>


using namespace UPP;


//######################### uCluster #########################


void uCluster::wakeProcessor( uPid_t pid __attribute__(( unused )) ) {
	uDEBUGPRT( uDebugPrt( "uCluster::wakeProcessor: waking processor %lu\n", (unsigned long)pid ); );

	#ifdef __U_STATISTICS__
	uFetchAdd( UPP::Statistics::wake_processor, 1 );
	#endif // __U_STATISTICS__

	#if defined( __U_MULTI__ )
	RealRtn::pthread_kill( pid, SIGUSR1 );
	#else // UNIPROCESSOR
	// Only one kernel thread so no need to send it a signal.
	#endif // __U_MULTI__
} // uCluster::wakeProcessor


// Safe to make direct accesses through TLS pointer because only called from preemption-safe locations:
// uProcessorKernel::main
void uCluster::processorPause() {
	assert( uKernelModule::uKernelModuleBoot.disableInt && uKernelModule::uKernelModuleBoot.disableIntCnt > 0 );

	if ( uThisProcessor().getPreemption() != 0 ) {		// optimize out UNIX call if possible
		uThisProcessor().setContextSwitchEvent( 0 );	// turn off preemption or it keeps waking the UNIX processor
	} // if

	// Check the ready queue to make sure that no task managed to slip onto the queue since the processor last checked.

	readyIdleTaskLock.acquire();

	if ( ! readyQueueEmpty() || ! uThisProcessor().external.empty() ) {
		readyIdleTaskLock.release();
		uDEBUGPRT( uDebugPrt( "(uCluster &)%p.processorPause, found work\n", this ); );
	} else {
		// Block any SIGALRM/SIGUSR1 signals from arriving.
		sigset_t new_mask, old_mask;
		sigemptyset( &new_mask );
		sigemptyset( &old_mask );
		sigaddset( &new_mask, SIGALRM );
		sigaddset( &new_mask, SIGUSR1 );
		if ( sigprocmask( SIG_BLOCK, &new_mask, &old_mask ) == -1 ) {
			abort( "internal error, sigprocmask" );
		} // if

		if ( ! uKernelModule::uKernelModuleBoot.RFinprogress && uKernelModule::uKernelModuleBoot.RFpending ) { // need to start roll forward ?
			readyIdleTaskLock.release();

			if ( sigprocmask( SIG_SETMASK, &old_mask, nullptr ) == -1 ) { // restored old signal mask over new one
				abort( "internal error, sigprocmask" );
			} // if
			uDEBUGPRT( uDebugPrt( "(uCluster &)%p.processorPause, found roll forward %d %d %d\n",
								  this, uKernelModule::uKernelModuleBoot.RFinprogress, uKernelModule::uKernelModuleBoot.RFpending, uKernelModule::uKernelModuleBoot.disableIntSpin ); );
		} else {
			makeProcessorIdle( uThisProcessor() );
			readyIdleTaskLock.release();

			uDEBUGPRT( uDebugPrt( "(uCluster &)%p.processorPause, before sigpause\n", this ); );

	#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::kernel_thread_pause, 1 );
	#endif // __U_STATISTICS__

			sigsuspend( &old_mask );					// install old signal mask over new one and wait for signal to arrive

			if ( sigprocmask( SIG_SETMASK, &old_mask, nullptr ) == -1 ) { // new mask restored so install old signal mask over new one
				abort( "internal error, sigprocmask" );
			} // if

			uDEBUGPRT( uDebugPrt( "(uCluster &)%p.processorPause, after sigpause\n", this ); );

			makeProcessorActive( uThisProcessor() );
		} // if
	} // if

	if ( uThisProcessor().getPreemption() != 0 ) {		// optimize out UNIX call if possible
		uThisProcessor().setContextSwitchEvent( uThisProcessor().getPreemption() ); // reset processor preemption time
	} // if

	uDEBUGPRT( uDebugPrt( "(uCluster &)%p.processorPause, reset timeslice:%d\n", this, uThisProcessor().getPreemption() ); );

	// rollForward is called by uProcessorKernel::main explicitly during spinning or implicitly by enableInterrupts on
	// the backside of the next scheduled task.

	assert( uKernelModule::uKernelModuleBoot.disableInt && uKernelModule::uKernelModuleBoot.disableIntCnt > 0 );
} // uCluster::processorPause


void uCluster::makeProcessorIdle( uProcessor &processor ) {
	assert( readyIdleTaskLock.value != 0 );				// readyIdleTaskLock must be acquired
	idleProcessorsCnt += 1;
	idleProcessors.addHead( &(processor.idleRef) );		// stack to keep processors warm
} // uCluster::makeProcessorIdle


void uCluster::makeProcessorActive( uProcessor &processor ) {
	uDEBUGPRT( uDebugPrt( "(uCluster &)%p.makeProcessorActive(1)\n", this ); );
	if ( processor.idle() ) {							// processor on idle queue ?
		readyIdleTaskLock.acquire();
		if ( processor.idle() ) {						// double check
			idleProcessorsCnt -= 1;
			idleProcessors.remove( &(processor.idleRef) );
		} // if
		readyIdleTaskLock.release();
	} // if
} // uCluster::makeProcessorActive


void uCluster::makeProcessorActive() {
	uDEBUGPRT( uDebugPrt( "(uCluster &)%p.makeProcessorActive(2)\n", this ); );
	readyIdleTaskLock.acquire();
	if ( ! readyQueue->empty() && ! idleProcessors.empty() ) {
		uPid_t pid = idleProcessors.dropHead()->processor().pid;
		idleProcessorsCnt -= 1;
		readyIdleTaskLock.release();					// don't hold lock while sending SIGALRM
		wakeProcessor( pid );
	} else {
		readyIdleTaskLock.release();
	} // if
} // uCluster::makeProcessorActive


void uCluster::makeTaskReady( uBaseTask &readyTask ) {
	readyIdleTaskLock.acquire();
	if ( std::addressof(readyTask.bound_) != nullptr ) { // task bound to a specific processor ?
		uDEBUGPRT( uDebugPrt( "(uCluster &)%p.makeTaskReady(1): task %.256s (%p) makes task %.256s (%p) ready\n",
							  this, uThisTask().getName(), &uThisTask(), readyTask.getName(), &readyTask ); );
		uProcessor *p = &readyTask.bound_;				// optimization

		p->external.addTail( &(readyTask.readyRef_) );	// add task to end of special ready queue
	#ifdef __U_MULTI__
		if ( p->idle() ) {								// processor on idle queue ?
			idleProcessors.remove( &(p->idleRef) );
			idleProcessorsCnt -= 1;
			uPid_t pid = p->pid;
			readyIdleTaskLock.release();				// don't hold lock while sending SIGALRM
			wakeProcessor( pid );
		} else {
			readyIdleTaskLock.release();
		} // if
	#else
		readyIdleTaskLock.release();
	#endif // __U_MULTI__
	} else {
		uDEBUGPRT( uDebugPrt( "(uCluster &)%p.makeTaskReady(2): task %.256s (%p) makes task %.256s (%p) ready\n",
							  this, uThisTask().getName(), &uThisTask(), readyTask.getName(), &readyTask ); );
		readyQueue->add( &(readyTask.readyRef_) );		// add task to end of cluster ready queue
	#ifdef __U_MULTI__
		// Wake up an idle processor if the ready task is migrating to another cluster with idle processors or if the
		// ready task is on the same cluster but the ready queue of that cluster is not empty. This check prevents a
		// single task on a cluster, which does a yield, from unnecessarily waking up a processor that has no work to
		// do.

		if ( ! idleProcessors.empty() && ( &uThisCluster() != this || ! readyQueue->empty() ) ) {
			uPid_t pid = idleProcessors.dropHead()->processor().pid;
			idleProcessorsCnt -= 1;
			readyIdleTaskLock.release();				// don't hold lock while sending SIGALRM
			wakeProcessor( pid );
		} else {
			readyIdleTaskLock.release();
		} // if
	#else
		readyIdleTaskLock.release();
	#endif // __U_MULTI__
	} // if
} // uCluster::makeTaskReady


void uCluster::makeTaskReady( uBaseTaskSeq &newTasks, unsigned int n __attribute__(( unused )) ) {
	readyIdleTaskLock.acquire();
	// cannot be bound task as all tasks come from RW lock
	uDEBUGPRT( uDebugPrt( "(uCluster &)%p.makeTaskReady(2): task %.256s (%p) tasks ready\n",
						  this, uThisTask().getName(), &uThisTask() ); );

	#ifdef __U_STATISTICS__
	uFetchAdd( UPP::Statistics::ready_queue, n );
	#endif // __U_STATISTICS__
	readyQueue->transfer( newTasks );					// add task(s) to end of cluster ready queue

	#ifdef __U_MULTI__
	// Wake up an idle processor if the ready task is migrating to another cluster with idle processors or if the ready
	// task is on the same cluster but the ready queue of that cluster is not empty. This check prevents a single task
	// on a cluster, which does a yield, from unnecessarily waking up a processor that has no work to do.

	if ( ! idleProcessors.empty() && ( &uThisCluster() != this || ! readyQueue->empty() ) ) {
		uProcessorSeq restart;
		for ( unsigned int i = 0; i < n && ! idleProcessors.empty(); i += 1 ) {
			restart.addTail( idleProcessors.dropHead() );
			idleProcessorsCnt -= 1;
		} // for
		readyIdleTaskLock.release();					// don't hold lock while sending SIGALRM
		for ( ; ! restart.empty(); ) {
			uPid_t pid = restart.dropHead()->processor().pid;
			wakeProcessor( pid );
		} // for
	} else {
		readyIdleTaskLock.release();
	} // if
	#else
	readyIdleTaskLock.release();
	#endif // __U_MULTI__
} // uCluster::makeTaskReady


void uCluster::readyQueueRemove( uBaseTaskDL *node ) {
	readyIdleTaskLock.acquire();
	readyQueue->remove( node );
	readyIdleTaskLock.release();
} // uCluster::readyQueueRemove


uBaseTask &uCluster::readyQueueTryRemove() {
	// Select a task from the ready queue of this cluster if there are no ready tasks, return the nil pointer.

	uBaseTask *task;

	readyIdleTaskLock.acquire();
	if ( ! readyQueueEmpty() ) {
		task = &(readyQueue->drop()->task());
	} else {
		task = nullptr;
	} // if
	readyIdleTaskLock.release();
	return *task;
} // uCluster::readyQueueTryRemove


void uCluster::taskAdd( uBaseTask &task ) {
	readyIdleTaskLock.acquire();
	tasksOnCluster.addTail( &(task.clusterRef_) );
	if ( std::addressof(task.bound_) == nullptr ) readyQueue->addInitialize( tasksOnCluster ); // processor task is not part of normal initialization
	readyIdleTaskLock.release();
} // uCluster::taskAdd


void uCluster::taskRemove( uBaseTask &task ) {
	readyIdleTaskLock.acquire();
	tasksOnCluster.remove( &(task.clusterRef_) );
	if ( std::addressof(task.bound_) == nullptr ) readyQueue->removeInitialize( tasksOnCluster ); // processor task is not part of normal initialization
	readyIdleTaskLock.release();
} // uCluster::taskRemove


void uCluster::taskReschedule( uBaseTask &task ) {
	readyIdleTaskLock.acquire();
	readyQueue->rescheduleTask( &(task.clusterRef_), tasksOnCluster );
	readyIdleTaskLock.release();
} // uCluster::taskReschedule


void uCluster::processorAdd( uProcessor &processor ) {
	processorsOnClusterLock.acquire();
	numProcessors += 1;
	processorsOnCluster.addTail( &(processor.processorRef) );
	processorsOnClusterLock.release();
} // uCluster::processorAdd


void uCluster::processorRemove( uProcessor &processor ) {
	processorsOnClusterLock.acquire();
	numProcessors -= 1;
	processorsOnCluster.remove( &(processor.processorRef) );
	processorsOnClusterLock.release();
} // uCluster::processorRemove


#if defined( __U_MULTI__ )
void uCluster::processorPoke() {
	processorsOnClusterLock.acquire();
	uProcessorDL *head = processorsOnCluster.head();
	if ( head != nullptr ) {
		// freebsd has hidden locks in pthread routines. To prevent deadlock, disable interrupts so a context switch
		// cannot occur to another user thread that makes the same call. As well, freebsd returns and undocumented EINTR
		// from pthread_kill.
		RealRtn::pthread_kill( head->processor().getPid(), SIGUSR1 );
	} // if
	processorsOnClusterLock.release();
} // uCluster::processorRemove
#endif // __U_MULTI__


void uCluster::createCluster( unsigned int stackSize, const char *name ) {
	uDEBUGPRT( uDebugPrt( "(uCluster &)%p.createCluster\n", this ); );

	#ifdef __U_DEBUG__
	#if __U_LOCALDEBUGGER_H__
	#ifdef __U_MULTI__
	debugIgnore = false;
	#else
	debugIgnore = true;
	#endif // __U_MULTI__
	#endif // __U_LOCALDEBUGGER_H__
	#endif // __U_DEBUG__

	numProcessors = 0;
	idleProcessorsCnt = 0;

	setName( name );
	setStackSize( stackSize );

	#if __U_LOCALDEBUGGER_H__
	if ( uLocalDebugger::uLocalDebuggerActive ) uLocalDebugger::uLocalDebuggerInstance->checkPoint();
	#endif // __U_LOCALDEBUGGER_H__

	uKernelModule::globalClusterLock->acquire();
	uKernelModule::globalClusters->addTail( &globalRef );
	uKernelModule::globalClusterLock->release();

	#if __U_LOCALDEBUGGER_H__
	if ( uLocalDebugger::uLocalDebuggerActive ) uLocalDebugger::uLocalDebuggerInstance->createCluster( *this );
	#endif // __U_LOCALDEBUGGER_H__

	if ( readyQueue == nullptr ) {
		readyQueue = new uDefaultScheduler;
		defaultReadyQueue = true;
	} else {
		defaultReadyQueue = false;
	} // if

	#ifdef __U_PROFILER__
	if ( uProfiler::uProfiler_registerCluster ) {
		(*uProfiler::uProfiler_registerCluster)( uProfiler::profilerInstance, *this );
	} // if
	#endif // __U_PROFILER__
} // uCluster::createCluster


uCluster::uCluster( unsigned int stackSize, const char *name ) : globalRef( *this ), readyQueue( nullptr ), wakeupList( *this ) {
	createCluster( stackSize, name );
} // uCluster::uCluster


uCluster::uCluster( const char *name ) : globalRef( *this ), readyQueue( nullptr ), wakeupList( *this ) {
	createCluster( uDefaultStackSize(), name );
} // uCluster::uCluster


uCluster::uCluster( uBaseSchedule<uBaseTaskDL> &ReadyQueue, unsigned int stackSize, const char *name ) : globalRef( *this ), readyQueue( &ReadyQueue ), wakeupList( *this ) {
	createCluster( stackSize, name );
} // uCluster::uCluster


uCluster::uCluster( uBaseSchedule<uBaseTaskDL> &ReadyQueue, const char *name ) : globalRef( *this ), readyQueue( &ReadyQueue ), wakeupList( *this ) {
	createCluster( uDefaultStackSize(), name );
} // uCluster::uCluster


uCluster::~uCluster() {
	uDEBUGPRT( uDebugPrt( "(uCluster &)%p.~uCluster\n", this ); );

	#ifdef __U_PROFILER__
	if ( uProfiler::uProfiler_deregisterCluster ) {
		(*uProfiler::uProfiler_deregisterCluster)( uProfiler::profilerInstance, *this );
	} // if
	#endif // __U_PROFILER__

	uProcessorDL *pr;
	for ( uSeqIter<uProcessorDL> iter(processorsOnCluster); iter >> pr; ) {
		if ( pr->processor().getDetach() ) {			// detached ?
			delete &pr->processor();					// delete detached processor
	#ifdef __U_DEBUG__
		} else {
			// Must check for processors before tasks because each processor has a processor task, and hence, there is
			// always a task on the cluster.
			abort( "Attempt to delete cluster %.256s (%p) with processor %p still on it.\n"
				   "Possible cause is the processor has not been deleted.",
				   getName(), this, &(pr->processor()) );
	#endif // __U_DEBUG__
		} // if
	} // for

	#ifdef __U_DEBUG__
	uBaseTaskDL *tr;
	tr = tasksOnCluster.head();
	if ( tr != nullptr ) {
		abort( "Attempt to delete cluster %.256s (%p) with task %.256s (%p) still on it.\n"
			   "Possible cause is the task has not been deleted.",
			   getName(), this, tr->task().getName(), &(tr->task()) );
	} // if
	#endif // __U_DEBUG__

	if ( defaultReadyQueue ) {							// delete if cluster allocated it
		delete readyQueue;
	} // if

	#if __U_LOCALDEBUGGER_H__
	if ( uLocalDebugger::uLocalDebuggerActive ) uLocalDebugger::uLocalDebuggerInstance->destroyCluster( *this );
	#endif // __U_LOCALDEBUGGER_H__

	uKernelModule::globalClusterLock->acquire();
	uKernelModule::globalClusters->remove( &globalRef );
	uKernelModule::globalClusterLock->release();
} // uCluster::~uCluster


void uCluster::taskResetPriority( uBaseTask &owner, uBaseTask &calling ) { // TEMPORARY
	uDEBUGPRT( uDebugPrt( "(uCluster &)%p.taskResetPriority, owner:%p, calling:%p, owner's cluster:%p\n", this, &owner, &calling, owner.currCluster ); );
	readyIdleTaskLock.acquire();
	if ( &uThisCluster() == owner.currCluster_ ) {
		if ( readyQueue->checkPriority( owner.readyRef_, calling.readyRef_ ) ) {
			readyQueue->resetPriority( owner.readyRef_, calling.readyRef_ );
		} // if
	} // if
	readyIdleTaskLock.release();
} // uCluster::taskResetPriority


void uCluster::taskSetPriority( uBaseTask &owner, uBaseTask &calling ) {
	uDEBUGPRT( uDebugPrt( "(uCluster &)%p.taskSetPriority, owner:%p, calling:%p, owner's cluster:%p\n", this, &owner, &calling, owner.currCluster ); );
	readyIdleTaskLock.acquire();
	readyQueue->resetPriority( owner.readyRef_, calling.readyRef_ );
	readyIdleTaskLock.release();
} // uCluster::taskSetPriority


int uCluster::select( int fd, int rwe, timeval *timeout ) {
	int retcode;										// create fake structures for null closure

	uIOaccess access;
	access.fd = fd;
	access.poll.setStatus( uPoll::NeverPoll );

	struct Select : public uIOClosure {
		int action() { return 0; }
		Select( uIOaccess &access, int &retcode ) : uIOClosure( access, retcode ) {}
	} selectClosure( access, retcode );

	return NBIO.select( selectClosure, rwe, timeout );
} // uCluster::select


// Local Variables: //
// compile-command: "make install" //
// End: //
