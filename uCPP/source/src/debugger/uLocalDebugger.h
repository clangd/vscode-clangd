//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Martin Karsten 1995
// 
// uLocalDebugger.h -- 
// 
// Author           : Martin Karsten
// Created On       : Thu Apr 20 21:32:37 1995
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr  5 08:10:12 2022
// Update Count     : 224
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


#ifndef __U_LOCALDEBUGGER_H__
// Set preprocessor variable if debugger works for that CPU/OS.
#if defined( __U_DEBUG__ ) && defined( __i386__ )
#define __U_LOCALDEBUGGER_H__ 0							// TURN OFF FOR NOW
#else
#define __U_LOCALDEBUGGER_H__ 0
#endif


// Don't include uC++.h. Either this file is included in uC++.h or uC++.h is
// included, before this file is included

_Task uLocalDebuggerReader;								// forward declaration
class uDebuggerProtocolUnit;							// forward declaration
class uSocketClient;									// forward declaration
class uBaseTask;										// forward declaration
class uBConditionList;									// forward declaration
class uBConditionEval;									// forward declaration
struct MinimalRegisterSet;								// forward declaration


//######################### uLocalDebugger #########################


_Task uLocalDebugger {
	friend _Task uSystemTask;							// access: uLocalDebuggerInstance, uLocalDebugger
	friend class uBaseTask;								// access: uLocalDebuggerInstance, uLocalDebuggerActive, checkPoint, migrateULThread
	friend _Coroutine UPP::uProcessorKernel;			// access: uLocalDebuggerInstance, dispatcher, debugger_blocked_tasks
	friend _Task uProcessorTask;						// access: uLocalDebuggerInstance, uLocalDebuggerActive, checkPoint, createKernelThread, destroyKernelThread, migrateKernelThread
	friend class uCluster;								// access: uLocalDebuggerInstance, uLocalDebuggerActive, checkPoint, createCluster, destroyCluster
	friend class uLocalDebuggerBoot;					// access: uLocalDebuggerInstance
	friend class uLocalDebuggerHandler;					// access: uLocalDebuggerInstance, breakpointHandler
	friend _Task uLocalDebuggerReader;					// access: finish, performAtomicOperation, setConditionMask, resetConditionMask
	friend void abort( const char *fmt, ... );			// access: uLocalDebuggerInstance, uLocalDebuggerActive, abortApplication
	friend _Task uProfiler;								// access: dispatcher
	friend class uMachContext;							// uLocalDebuggerInstance, uLocalDebuggerActive, createULThread, destroyULThread

	static bool uGlobalDebuggerActive;					// true => active, false => inactive (global debugger gone)
	static bool uLocalDebuggerActive;					// true => active, false => inactive (local debugger gone)
	static uLocalDebugger *uLocalDebuggerInstance;		// unique local debugger

	bool attaching;										// communication
	int port;
	const char *machine, *name;

	uSocketClient		*sockClient;					// socket client object for communication with global debugger
	uLocalDebuggerReader *dispatcher;					// the dispatcher task
	uBConditionList		*bpc_list;						// list of handler associated conditions
	int 				debugger_blocked_tasks;			// number of debugger blocked tasks
	uDebuggerProtocolUnit &pdu;							// communication

	// This flag is set to true by the cont_handler, when the global debugger
	// continues the application at the end of an abort.
	static bool			abort_confirmed;

	// This signal handler is installed, if the target application aborts to
	// catch the last continue signal by the global debugger.
	static void	cont_handler( __U_SIGPARMS__ );

	struct blockedNode : public uSeqable {
		uBaseTask *key;
		uCondition wait;
		uDebuggerProtocolUnit &pdu;
		blockedNode( uBaseTask *key, uDebuggerProtocolUnit &pdu );
	};

	uSequence<blockedNode> blockedTasks;

	static void uLocalDebuggerBeginCode();
	static void uLocalDebuggerEndCode();
	// TEMPORARY: should be a static free routine but has to be a friend, too.
	static void readPDU( uSocketClient &sockClient, uDebuggerProtocolUnit &pdu );

	void setConditionMask( int no, void *ul_thread_id );
	void resetConditionMask( int no, void *ul_thread_id );

	void main();
	void deregister( );									// deregister from the global debugger

	void uCreateLocalDebugger( int port, const char *machine, const char *fullpath );
	uLocalDebugger( int port );							// dynamically initialize global debugger
	uLocalDebugger( const char *port, const char *machine, const char *name );
	~uLocalDebugger();

	void send( uDebuggerProtocolUnit& pdu );			// send data to the global debugger
	void receive();										// receive data from the global debugger / task is put to sleep
	bool receive( uDebuggerProtocolUnit& pdu );			// receive data from the global debugger / task is put to sleep

	_Mutex void unblockTask( uBaseTask *ul_thread, uDebuggerProtocolUnit &pdu );
	_Mutex int breakpointHandler( int no );
	void checkPoint();									// called before modifying kernel lists
	_Mutex void checkPointMX();
	_Mutex void performAtomicOperation();				// executing in a certain code range (called by uLocalDebuggerReader)
	_Mutex void attachULThread( uBaseTask* this_task, uCluster* this_cluster );
	void createULThread();
	_Mutex void createULThreadMX( MinimalRegisterSet &regs );
	void destroyULThread();
	_Mutex void destroyULThreadMX();
	_Mutex void createCluster( uCluster &cluster_address );
	_Mutex void destroyCluster( uCluster &cluster_address );
	_Mutex void createKernelThread( uProcessor &process_address, uCluster &cluster_address );
	_Mutex void destroyKernelThread( uProcessor &process_address );
	_Mutex void migrateKernelThread( uProcessor &process_address, uCluster &to_address );
	void migrateULThread( uCluster &to_address );
	_Mutex void migrateULThreadMX( uCluster &to_address );
	_Mutex void finish();
	_Mutex void abortApplication();
}; // uLocalDebugger


//######################### uLocalDebuggerBoot #########################


class uLocalDebuggerBoot {
	static int uCount;
  public:
	uLocalDebuggerBoot();
	~uLocalDebuggerBoot();
}; // uLocalDebuggerBoot


#endif // __U_LOCALDEBUGGER_H__


// Local Variables: //
// tab-width: 4 //
// compile-command: "make install" //
// End: //
