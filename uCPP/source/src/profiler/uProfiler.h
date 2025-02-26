//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Robert Denda 1997
// 
// uProfiler.cc -- 
// 
// Author           : Robert Denda
// Created On       : Tue Jul 16 16:45:16 1996
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 19 11:21:33 2022
// Update Count     : 724
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


#pragma once


#include <uC++.h>


class ProfilerAnalyze;									// forward declarations
class SymbolTable;
class uProfileTaskSampler;
class uExecutionMonitor;
class uExecutionMonitorNode;
class MMInfoEntry;


// The maximum number of metrics that are allowed to request memory allocation in the uC++ kernel
#define U_MAX_METRICS 8


//######################### uCreateMetricFunctions #########################


struct uCreateMetricFunctions : public uSeqable {
	char *className;
	void *createWidgetFunction;                         // address of function to create metric startup widget in Profiler startup window
	void *createMonitorFunction;

	uCreateMetricFunctions(char *name);
	~uCreateMetricFunctions();
}; // uCreateMetricFunctions


//######################### uProfileEvent #########################


class uProfileEvent : public uSeqable {
	friend _Task uProfiler;								// only profiler can access member routines
	friend class uProfileMonitorSet;

	uExecutionMonitorNode &monitor;
	uTime time;
  public:
	uProfileEvent( uExecutionMonitorNode &monitor );
}; // uProfileEvent


//######################### uProfileMonitorSet #########################


class uProfileMonitorSet : private uSequence<uProfileEvent> {
  public:
	using uSequence<uProfileEvent>::head;				// pass through from uSequence
	using uSequence<uProfileEvent>::succ;
	using uSequence<uProfileEvent>::remove;

	void orderedInsert( uProfileEvent *np );
	void reorder( uProfileEvent *np );
}; // uProfileMonitorSet


//######################### uProfiler ##########################


_Task uProfiler {
	// hooks
	friend class UPP::uSerial;							// access: profilerInstance
	friend class UPP::uSerialConstructor;				// access: profilerInstance
	friend class UPP::uSerialDestructor;				// access: profilerInstance
	friend class UPP::uSerialMember;					// access: profilerInstance
	friend class UPP::uMachContext;						// access: profilerInstance
	friend class uBaseCoroutine;						// access: profilerInstance
	friend class uBaseTask;								// access: profilerInstance
	friend class uCluster;								// access: profilerInstance
	friend class uProcessor;							// access: profilerInstance
	friend _Task uProcessorTask;						// access: profilerInstance
	friend class UPP::uHeapManager;						// access: profilerInstance
	friend void *malloc( size_t size ) __THROW;			// access: profilerInstance
	friend void *memalign( size_t alignment, size_t size ) __THROW; // access: profilerInstance
	friend class uCondition;							// access: profilerInstance
	friend _Coroutine UPP::uProcessorKernel;			// access: profilerInstance

	// profiling
	friend class uProfilerBoot;
	friend class SymbolTable;
	friend class StartMenuWindow;
	friend class uExecutionMonitor;
	friend class uMemoryExecutionMonitor;               // access: getMetricMemoryIndex()
	friend class MMMonitor;								// activate uProfiler hooks in distributed monitoring mode
	friend class HWMonitor;								// activate uProfiler hooks in distributed monitoring mode
	friend class HWFunctionMonitor;						// activate uProfiler hooks in distributed monitoring mode
	friend class uProfileTaskSampler;					// access: addFuncCallInfo, memory allocation hooks
	friend class ProfilerAnalyze;						// access: symbolTable, InfoList
	friend void __cyg_profile_func_enter( void *pcCurrentFunction, void *pcCallingFunction );
	friend void __cyg_profile_func_exit( void *pcCurrentFunction, void *pcCallingFunction );

	static uProfiler *profilerInstance;					// pointer to profiler

	uSequence<uCreateMetricFunctions> *metricList;      // list of metrics, used to create the profiler's startup window
	unsigned int numMetrics;
	int numMemoryMetrics;								// current index into the metric memory array

	uProfileMonitorSet monitorSet;						// list of poll events for samplers
	uCluster           cluster;							// own cluster
	uProcessor         processor;						// own processor
	SymbolTable       *symbolTable;						// symbol table of the executable
	uClock            &processorClock;					// clock used to get current time
	bool               finish;							// end of gathering data

	uCondition delay;									// TEMPORARY: don't even ask

	// communication
	const uBaseTask	 *taskToProcess;
	uBaseTask::State      taskState;
	const uBaseCoroutine *coroutineToProcess;
	const uBaseCoroutine *coroutineFromProcess;
	const uCluster	 *clusterToCluster;
	const uProcessor	 *processorToProcess;
	const uCluster	 *cluster1;
	const uCluster	 *cluster2;
	const UPP::uSerial	 *serial;
	const char           *objectName;
	const uCondition	 *condition;
	void		 *currentFunction;
	void		 *callingFunction;
	void		 *firstArg;
	void                 *memAddress;
	size_t                memSizeRequested, memSizeAllocated;

	// monitoring modules
	mutable uSequence<uExecutionMonitorNode> *monitorList;
	mutable uSequence<uExecutionMonitorNode> *registerTaskDecentrList; // list of monitors to be informed upon task registration
	mutable uSequence<uExecutionMonitorNode> *registerTaskMonitorList;
	mutable uSequence<uExecutionMonitorNode> *deregisterTaskDecentrList; // list of monitors to be informed upon task deregistration
	mutable uSequence<uExecutionMonitorNode> *deregisterTaskMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerTaskStartExecutionDecentrList; // list of monitors to be infor. upon task registration
	mutable uSequence<uExecutionMonitorNode> *registerTaskStartExecutionMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerTaskEndExecutionDecentrList; // list of monitors to be infor. upon task registration
	mutable uSequence<uExecutionMonitorNode> *registerTaskEndExecutionMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerClusterDecentrList; // list of monitors to be informed upon cluster registr.
	mutable uSequence<uExecutionMonitorNode> *registerClusterMonitorList;
	mutable uSequence<uExecutionMonitorNode> *deregisterClusterDecentrList;	// list of monitors to be informed upon cluster dereg.
	mutable uSequence<uExecutionMonitorNode> *deregisterClusterMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerProcessorDecentrList;	// list of monitors to be informed upon processor registr.
	mutable uSequence<uExecutionMonitorNode> *registerProcessorMonitorList;
	mutable uSequence<uExecutionMonitorNode> *deregisterProcessorDecentrList; // list of monitors to be informed upon processor dereg.
	mutable uSequence<uExecutionMonitorNode> *deregisterProcessorMonitorList;
	mutable uSequence<uExecutionMonitorNode> *migrateTaskDecentrList; // list of monitors to be informed upon task migration
	mutable uSequence<uExecutionMonitorNode> *migrateTaskMonitorList;
	mutable uSequence<uExecutionMonitorNode> *migrateProcessorDecentrList; // list of monitors to be informed upon processor migration
	mutable uSequence<uExecutionMonitorNode> *migrateProcessorMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerFunctionEntryDecentrList;	// list of monitors to be informed upon routine entry
	mutable uSequence<uExecutionMonitorNode> *registerFunctionEntryMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerFunctionExitDecentrList;  // list of monitors to be informed upon routine exit
	mutable uSequence<uExecutionMonitorNode> *registerFunctionExitMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerMutexFunctionEntryTryDecentrList;	// list of monitors to be informed upon mutex routine entry petition
	mutable uSequence<uExecutionMonitorNode> *registerMutexFunctionEntryTryMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerMutexFunctionEntryDoneDecentrList; // list of monitors to be informed upon mutex routine entry
	mutable uSequence<uExecutionMonitorNode> *registerMutexFunctionEntryDoneMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerMutexFunctionExitDecentrList; // list of monitors informed upon mutex routine exit
	mutable uSequence<uExecutionMonitorNode> *registerMutexFunctionExitMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerMonitorDecentrList; // list of monitors to be informed upon monitor creation
	mutable uSequence<uExecutionMonitorNode> *registerMonitorMonitorList;
	mutable uSequence<uExecutionMonitorNode> *deregisterMonitorDecentrList;	// list of monitors to be informed upon monitor creation
	mutable uSequence<uExecutionMonitorNode> *deregisterMonitorMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerAcceptStartDecentrList; // list of monitors to be informed upon accept start
	mutable uSequence<uExecutionMonitorNode> *registerAcceptStartMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerAcceptEndDecentrList;	// list of monitors to be informed upon accept end
	mutable uSequence<uExecutionMonitorNode> *registerAcceptEndMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerWaitDecentrList; // list of monitors to be informed upon wait on condition
	mutable uSequence<uExecutionMonitorNode> *registerWaitMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerReadyDecentrList;	// list of monitors to be informed upon wake up after wait on condition
	mutable uSequence<uExecutionMonitorNode> *registerReadyMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerSignalDecentrList; // list of monitors to be informed upon signal on condition
	mutable uSequence<uExecutionMonitorNode> *registerSignalMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerCoroutineDecentrList;	// list of monitors to be informed upon coroutine creation
	mutable uSequence<uExecutionMonitorNode> *registerCoroutineMonitorList;
	mutable uSequence<uExecutionMonitorNode> *deregisterCoroutineDecentrList; // list of monitors to be informed upon coroutine deletion
	mutable uSequence<uExecutionMonitorNode> *deregisterCoroutineMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerCoroutineBlockDecentrList; // list of monitors to be informed on coroutine inactivate
	mutable uSequence<uExecutionMonitorNode> *registerCoroutineBlockMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerCoroutineUnblockDecentrList; // list of monitors to be informed on coroutine activate
	mutable uSequence<uExecutionMonitorNode> *registerCoroutineUnblockMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerTaskExecStateDecentrList; // list of monitors to be informed on change of task state
	mutable uSequence<uExecutionMonitorNode> *registerTaskExecStateMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerSetNameDecentrList; //  monitors to be inf. on task/coroutine change of name
	mutable uSequence<uExecutionMonitorNode> *registerSetNameMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerMemoryAllocateDecentrList;
	mutable uSequence<uExecutionMonitorNode> *registerMemoryAllocateMonitorList;
	mutable uSequence<uExecutionMonitorNode> *registerMemoryDeallocateDecentrList;
	mutable uSequence<uExecutionMonitorNode> *registerMemoryDeallocateMonitorList;
	mutable uSequence<uExecutionMonitorNode> *pollDecentrList; // list of monitors to be periodically polled
	mutable uSequence<uExecutionMonitorNode> *pollMonitorList;
	mutable uSequence<uExecutionMonitorNode> *profileInactivateDecentrList;
	mutable uSequence<uExecutionMonitorNode> *profileInactivateMonitorList;

	// built in monitoring modules
	mutable uSequence<uExecutionMonitorNode> *builtinRegisterTaskBlockDecentrList; // list of monitors to be informed upon task block
	mutable uSequence<uExecutionMonitorNode> *builtinRegisterTaskUnblockDecentrList; // list of monitors to be informed upon task unblock
	mutable uSequence<uExecutionMonitorNode> *builtinRegisterFunctionEntryDecentrList;
	mutable uSequence<uExecutionMonitorNode> *builtinRegisterFunctionExitDecentrList;
	mutable uSequence<uExecutionMonitorNode> *builtinRegisterProcessorDecentrList;
	mutable uSequence<uExecutionMonitorNode> *builtinDeregisterProcessorDecentrList;
	mutable uSequence<uExecutionMonitorNode> *builtinRegisterTaskStartSpinDecentrList;
	mutable uSequence<uExecutionMonitorNode> *builtinRegisterTaskStopSpinDecentrList;

	// Statistical-profiling signal handlers
	static void sigVirtualAlrmHandler( __U_SIGPARMS__ );
	static void sigHWOverflowHandler( __U_SIGPARMS__ );

	void main();
	void checkMetricList( void );						// make sure all required info for creating metrics is provided

	// functions for handling dynamic memory allocation in uC++ kernel
	int  getMetricMemoryIndex();						// get an index into the metric memory array
	void preallocateMetricMemory( void **, const uBaseTask & ) const; // preallocate required metric memory for current task in provided array
	void postallocateMetricMemory( const uBaseTask & ) const; // postallocate required metric memory for current task
	void setMetricMemoryPointers( void **, const uBaseTask & ) const; // force profiler to use provided array for memory management
	void resetMetricMemoryPointers( const uBaseTask & ) const; // allow profiler to resume using its native array for memory management

	void addToMonitorList( uExecutionMonitor *mon ) const;

	_Mutex void registerTaskMutex( const uBaseTask &task, const UPP::uSerial &serial, const uBaseTask &parent );
	_Mutex void deregisterTaskMutex( const uBaseTask &task );
	_Mutex void registerTaskStartExecutionMutex( const uBaseTask &task );
	_Mutex void registerTaskEndExecutionMutex( const uBaseTask &task );
	_Mutex void migrateTaskMutex( const uBaseTask &task, const uCluster &fromCluster, const uCluster &toCluster );
	_Mutex void registerClusterMutex( const uCluster &cluster );
	_Mutex void deregisterClusterMutex( const uCluster &cluster );
	_Mutex void registerProcessorMutex( const uProcessor &processor );
	_Mutex void deregisterProcessorMutex( const uProcessor &processor );
	_Mutex void migrateProcessorMutex( const uProcessor &processor, const uCluster &fromCluster, const uCluster &toCluster );
	_Mutex void registerFunctionEntryMutex( uBaseTask *task, void *pcCallingFunction, void *pcCurrentFunction, void *pcFirstArg );
	_Mutex void registerFunctionExitMutex( uBaseTask *task, void *pcCallingFunction, void *pcCurrentFunction, void *pcFirstArg );
	_Mutex void registerMutexFunctionEntryTryMutex( const UPP::uSerial &serial, const uBaseTask &task );
	_Mutex void registerMutexFunctionEntryDoneMutex( const UPP::uSerial &serial, const uBaseTask &task );
	_Mutex void registerMutexFunctionExitMutex( const UPP::uSerial &serial, const uBaseTask &task );
	_Mutex void registerMonitorMutex( const UPP::uSerial &serial, const char *name, const uBaseTask &task );
	_Mutex void deregisterMonitorMutex( const UPP::uSerial &serial, const uBaseTask &task );
	_Mutex void registerAcceptStartMutex( const UPP::uSerial &serial, const uBaseTask &task );
	_Mutex void registerAcceptEndMutex( const UPP::uSerial &serial, const uBaseTask &task );
	_Mutex void registerWaitMutex( const uCondition &condition, const uBaseTask &task, const UPP::uSerial &serial );
	_Mutex void registerReadyMutex( const uCondition &condition, const uBaseTask &task, const UPP::uSerial &serial );
	_Mutex void registerSignalMutex( const uCondition &condition, const uBaseTask &task, const UPP::uSerial &serial );
	_Mutex void registerCoroutineMutex( const uBaseCoroutine &coroutine, const UPP::uSerial &serial );
	_Mutex void deregisterCoroutineMutex( const uBaseCoroutine &coroutine );
	_Mutex void registerCoroutineBlockMutex( const uBaseTask &task, const uBaseCoroutine &coroutine );
	_Mutex void registerCoroutineUnblockMutex( const uBaseTask &task );
	_Mutex void registerTaskExecStateMutex( const uBaseTask &task, uBaseTask::State state, void *function );
	_Mutex void registerSetNameMutex( const uBaseCoroutine &coroutine, const char *name );
	_Mutex void registerMemoryAllocateMutex( void *address, size_t sizeRequested, size_t sizeAllocated, const uBaseTask &task );
	_Mutex void registerMemoryDeallocateMutex( void *address, size_t requestedSize, const uBaseTask &task );
	_Mutex void profileInactivateMutex( const uBaseTask &task );

	_Mutex void Finish();
  public:
	uProfiler();
	~uProfiler();

	_Nomutex const SymbolTable *getSymbolTable() const;
	_Nomutex const ProfilerAnalyze *getAnalyzer() const;
	_Nomutex uCluster &getCluster() const { return const_cast<uCluster &>( cluster ); }

	uTime wakeUp();

	_Nomutex void registerTask( const uBaseTask &task, const UPP::uSerial &serial, const uBaseTask &parent );
	_Nomutex void deregisterTask( const uBaseTask &task );
	_Nomutex void registerTaskStartExecution( const uBaseTask &task );
	_Nomutex void registerTaskEndExecution( const uBaseTask &task );
	_Nomutex void registerCluster( const uCluster &cluster );
	_Nomutex void deregisterCluster( const uCluster &cluster );
	_Nomutex void registerProcessor( const uProcessor &processor );
	_Nomutex void deregisterProcessor( const uProcessor &processor );
	_Nomutex void migrateTask( const uBaseTask &task, const uCluster &fromCluster, const uCluster &toCluster );
	_Nomutex void migrateProcessor( const uProcessor &processor, const uCluster &fromCluster, const uCluster &toCluster );
	_Nomutex void registerFunctionEntry( uBaseTask *task, void *pcCallingFunction, void *pcCurrentFunction, void *pcFirstArg );
	_Nomutex void registerFunctionExit( uBaseTask *task, void *pcCallingFunction, void *pcCurrentFunction, void *pcFirstArg  );
	_Nomutex void registerMutexFunctionEntryTry( const UPP::uSerial &serial, const uBaseTask &task );
	_Nomutex void registerMutexFunctionEntryDone( const UPP::uSerial &serial, const uBaseTask &task );
	_Nomutex void registerMutexFunctionExit( const UPP::uSerial &serial, const uBaseTask &task );
	_Nomutex void registerMonitor( const UPP::uSerial &serial, const char *name, const uBaseTask &task );
	_Nomutex void deregisterMonitor( const UPP::uSerial &serial, const uBaseTask &task );
	_Nomutex void registerAcceptStart( const UPP::uSerial &serial, const uBaseTask &task );
	_Nomutex void registerAcceptEnd( const UPP::uSerial &serial, const uBaseTask &task );
	_Nomutex void registerWait( const uCondition &condition, const uBaseTask &task, const UPP::uSerial &serial );
	_Nomutex void registerReady( const uCondition &condition, const uBaseTask &task, const UPP::uSerial &serial );
	_Nomutex void registerSignal( const uCondition &condition, const uBaseTask &task, const UPP::uSerial &serial );
	_Nomutex void registerCoroutine( const uBaseCoroutine &coroutine, const UPP::uSerial &serial );
	_Nomutex void deregisterCoroutine( const uBaseCoroutine &coroutine );
	_Nomutex void registerCoroutineBlock( const uBaseTask &task, const uBaseCoroutine &coroutine );
	_Nomutex void registerCoroutineUnblock( const uBaseTask &task );
	_Nomutex void registerTaskExecState( const uBaseTask &task, uBaseTask::State state );
	_Nomutex void registerSetName( const uBaseCoroutine &coroutine, const char *name );
	_Nomutex MMInfoEntry *registerMemoryAllocate( void *address, size_t sizeRequested, size_t sizeAllocated );
	_Nomutex void registerMemoryDeallocate( void *address, size_t requestedSize, MMInfoEntry *entry );
	_Nomutex void poll();
	_Nomutex void profileInactivate( const uBaseTask &task );

	// built-in modules (do not have corresponding mutex members => cannot be used by user metrics)
	_Nomutex void builtinRegisterTaskBlock( const uBaseTask &task ); // not allowed to block ( hook inside context switch routine )
	_Nomutex void builtinRegisterTaskUnblock( const uBaseTask &task ); // not allowed to block  ( hook inside context switch routine )
	_Nomutex void builtinRegisterFunctionEntry();
	_Nomutex void builtinRegisterFunctionExit();
	_Nomutex void builtinRegisterProcessor( const uProcessor & ); // not allowed to block (hook inside uC++ kernel)
	_Nomutex void builtinDeregisterProcessor( const uProcessor & );	// not allowed to block (hook inside uC++ kernel)
	_Nomutex void builtinRegisterTaskStartSpin( const uBaseTask & ); // not allowed to block
	_Nomutex void builtinRegisterTaskStopSpin( const uBaseTask & );	// not allowed to block

	// routine pointers to member routines in uProfiler

	static void (* uProfiler_registerTask)(uProfiler *, const uBaseTask &, const UPP::uSerial &, const uBaseTask & );
	static void (* uProfiler_deregisterTask)(uProfiler *, const uBaseTask &);
	static void (* uProfiler_registerTaskStartExecution)(uProfiler *, const uBaseTask & );
	static void (* uProfiler_registerTaskEndExecution)(uProfiler *, const uBaseTask & );
	static void (* uProfiler_registerCluster)(uProfiler *, const uCluster &);
	static void (* uProfiler_deregisterCluster)(uProfiler *, const uCluster &);
	static void (* uProfiler_registerProcessor)(uProfiler *, const uProcessor &);
	static void (* uProfiler_deregisterProcessor)(uProfiler *, const uProcessor &);
	static void (* uProfiler_registerTaskMigrate)(uProfiler *, const uBaseTask &, const uCluster &, const uCluster & );
	static void (* uProfiler_registerProcessorMigrate)(uProfiler *, const uProcessor &, const uCluster &, const uCluster & );
	static void (* uProfiler_registerFunctionEntry)(uProfiler *, uBaseTask *, unsigned int, unsigned int, unsigned int );
	static void (* uProfiler_registerFunctionExit)(uProfiler *, uBaseTask * );
	static void (* uProfiler_registerMutexFunctionEntryTry)(uProfiler *, const UPP::uSerial &, const uBaseTask & );
	static void (* uProfiler_registerMutexFunctionEntryDone)(uProfiler *, const UPP::uSerial &, const uBaseTask & );
	static void (* uProfiler_registerMutexFunctionExit)(uProfiler *, const UPP::uSerial &, const uBaseTask & );
	static void (* uProfiler_registerMonitor)(uProfiler *, const UPP::uSerial &, const char *, const uBaseTask &  );
	static void (* uProfiler_deregisterMonitor)(uProfiler *, const UPP::uSerial &, const uBaseTask & );
	static void (* uProfiler_registerAcceptStart)(uProfiler *, const UPP::uSerial &, const uBaseTask & );
	static void (* uProfiler_registerAcceptEnd)(uProfiler *, const UPP::uSerial &, const uBaseTask & );
	static void (* uProfiler_registerWait)(uProfiler *, const uCondition &, const uBaseTask &, const UPP::uSerial & );
	static void (* uProfiler_registerReady)(uProfiler *, const uCondition &, const uBaseTask &, const UPP::uSerial & );
	static void (* uProfiler_registerSignal)(uProfiler *, const uCondition &, const uBaseTask &, const UPP::uSerial & );
	static void (* uProfiler_registerCoroutine)(uProfiler *, const uBaseCoroutine &, const UPP::uSerial & );
	static void (* uProfiler_deregisterCoroutine)(uProfiler *, const uBaseCoroutine & );
	static void (* uProfiler_registerCoroutineBlock)(uProfiler *, const uBaseTask &, const uBaseCoroutine & );
	static void (* uProfiler_registerCoroutineUnblock)(uProfiler *, const uBaseTask & );
	static void (* uProfiler_registerTaskExecState)(uProfiler *, const uBaseTask &, uBaseTask::State );
	static void (* uProfiler_poll)(uProfiler * );
	static void (* uProfiler_registerSetName)(uProfiler *, const uBaseCoroutine &, const char * );
	static MMInfoEntry *(* uProfiler_registerMemoryAllocate)(uProfiler *, void *, size_t, size_t );
	static void (* uProfiler_registerMemoryDeallocate)(uProfiler *, void *, size_t, MMInfoEntry * );
	static void (* uProfiler_profileInactivate)(uProfiler *, const uBaseTask & );

	// hooks for built in metrics (cannot be used by user metrics because they do not notify execution monitors)

	static void (* uProfiler_builtinRegisterTaskBlock)(uProfiler *, const uBaseTask & );
	static void (* uProfiler_builtinRegisterTaskUnblock)(uProfiler *, const uBaseTask & );
	static void (* uProfiler_builtinRegisterFunctionEntry)(uProfiler * );
	static void (* uProfiler_builtinRegisterFunctionExit)(uProfiler * );
	static void (* uProfiler_builtinRegisterProcessor)(uProfiler *, const uProcessor & );
	static void (* uProfiler_builtinDeregisterProcessor)(uProfiler *, const uProcessor & );
	static void (* uProfiler_builtinRegisterTaskStartSpin)(uProfiler *, const uBaseTask & );
	static void (* uProfiler_builtinRegisterTaskStopSpin)(uProfiler *, const uBaseTask & );

	// dynamic memory allocation in uC++ kernel

	static void (* uProfiler_preallocateMetricMemory)(uProfiler *, void **, const uBaseTask & );
	static void (* uProfiler_postallocateMetricMemory)(uProfiler *, const uBaseTask & );
	static void (* uProfiler_setMetricMemoryPointers)(uProfiler *, void **, const uBaseTask & );
	static void (* uProfiler_resetMetricMemoryPointers)(uProfiler *, const uBaseTask & );

	// debugging hook
	static void (* uProfiler_printCallStack)(uProfileTaskSampler * );
}; // uProfiler


// Local variables: //
// compile-command: "make install" //
// End: //
