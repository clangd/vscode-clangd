//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Martin Karsten 1995
// 
// uLocalDebugger.cc -- 
// 
// Author           : Martin Karsten
// Created On       : Thu Apr 20 21:34:48 1995
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 25 13:52:05 2023
// Update Count     : 727
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
#include <uSocket.h>
#include <uBootTask.h>
#include <uSystemTask.h>
#include <uProcessor.h>
//#include <uDebug.h>

#include <uLocalDebugger.h>

#include <csignal>
#include <unistd.h>										// gethostname

#include <uDebuggerProtocolUnit.h>						// must come after param.h
#include <uBConditionEval.h>

#ifndef FILENAME_MAX
#define FILENAME_MAX 1024
#endif

// for the moment...
#ifndef SIZE_OF_BREAKPOINT_FIELD
#define SIZE_OF_BREAKPOINT_FIELD	64
#endif

static const int max_no_of_breakpoints = SIZE_OF_BREAKPOINT_FIELD;

bool uLocalDebugger::uGlobalDebuggerActive = false;
bool uLocalDebugger::uLocalDebuggerActive = false;
uLocalDebugger *uLocalDebugger::uLocalDebuggerInstance = nullptr;
bool uLocalDebugger::abort_confirmed = false;

// KEEP CONSISTENT WITH THE CORRESPONDING FUNCTIONS IN uC++.h
// REASON : ACHIEVE OPTIMIZATION WITHOUT TURNING ON THE COMPILER'S
// OPTIMIZATION FLAG (-O) TO MAKE INLINE FUNCTIONS REALLY INLINE
#define U_THIS_TASK		TLS_GET( activeTask )			// uThisTask()
#define U_THIS_CLUSTER	TLS_GET( activeCluster )		// uThisCluster()

// include now, so that everything is already declared
#include <uLocalDebuggerHandler.h>


void uLocalDebugger::uLocalDebuggerBeginCode() {}		// marker beginning of debugger code


void uLocalDebugger::readPDU( uSocketClient &sockClient, uDebuggerProtocolUnit &pdu ) {
	uDEBUGPRT( uDebugPrt( "%d readPDU, called by task:%p\n", __LINE__, &uThisTask() ); );

	uDebuggerProtocolUnit::RequestType request_type;

	if ( sockClient.read( (char *)&request_type, sizeof(uDebuggerProtocolUnit::RequestType) ) != sizeof(uDebuggerProtocolUnit::RequestType) ) {
		uLocalDebugger::uGlobalDebuggerActive = false;
		exit( EXIT_FAILURE );							// TEMPORARY
		abort( "uLocalDebugger/readPDU : global debugger is gone" );
	} // if

	pdu.re_init( request_type );
	uDEBUGPRT( uDebugPrt( "%d uLocalDebugger/readPDU : got request %d\n", __LINE__, request_type ); );
	if ( pdu.data_size() ) {
		if ( sockClient.read( (char *)pdu.data_buffer(), pdu.data_size() ) != pdu.data_size() ) {
			uLocalDebugger::uGlobalDebuggerActive = false;
			exit( EXIT_FAILURE );						// TEMPORARY
			abort( "uLocalDebugger/readPDU : global debugger is gone" );
		} // if
	} // if

	uDEBUGPRT( uDebugPrt( "%d readPDU, done\n", __LINE__ ); );
} // uLocalDebugger::readPDU


//######################### uLocalDebuggerReader #########################


// This task is constantly waiting to read data from the global debugger.  Upon
// arrival of data, it's delivered to the appropriate member of the local
// debugger task.

_Task uLocalDebuggerReader {
	uLocalDebugger &debugger;
	uSocketClient &sockClient;
	uDebuggerProtocolUnit &pdu;

	void main();
  public:
	uLocalDebuggerReader( uLocalDebugger& debugger, uSocketClient& sockClient );
	~uLocalDebuggerReader();
}; // uLocalDebuggerReader


void uLocalDebuggerReader::main() {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebuggerReader &)%p.main, starts\n", __LINE__, this ); );

	uCluster *cluster;
	uProcessor *processor;
	bool ignore;

  endloop: for ( ;; ) {
		uLocalDebugger::readPDU( sockClient, pdu );		// read next pdu

		switch( pdu.getType() ) {
		  case uDebuggerProtocolUnit::AreplyAddress:
		  case uDebuggerProtocolUnit::OcontULThread:
		  case uDebuggerProtocolUnit::CgeneralConfirmation:
			debugger.unblockTask( pdu.readCgeneralConfirmation(), pdu );
			break;
		  case uDebuggerProtocolUnit::OshutdownConnection:
			debugger.finish();
			uDEBUGPRT( uDebugPrt( "%d (uLocalDebuggerReader &)%p.main, saw OshutdownConnection\n", __LINE__, this ); );
			break;
		  case uDebuggerProtocolUnit::CfinishLocalDebugger:
			uDEBUGPRT( uDebugPrt( "%d (uLocalDebuggerReader &)%p.main, saw CfinishLocalDebugger, bye\n", __LINE__, this ); );
			break endloop;
		  case uDebuggerProtocolUnit::OstartAtomicOperation:
			debugger.performAtomicOperation();
			break;
		  case uDebuggerProtocolUnit::OignoreClusterMigration:
			pdu.readOignoreClusterMigration( cluster, ignore );
			uDEBUGPRT( uDebugPrt( "%d (uLocalDebuggerReader &)%p.main, saw OignoreClusterMigration, cluster:%p, ignore:%d\n", __LINE__, this, cluster, ignore ); );
			cluster->debugIgnore = ignore;
			break;
		  case uDebuggerProtocolUnit::OignoreKernelThreadMigration:
			pdu.readOignoreKernelThreadMigration( processor, ignore );
			uDEBUGPRT( uDebugPrt( "%d (uLocalDebuggerReader &)%p.main, saw OignoreKernelThreadMigration, processor:%p, ignore:%d\n", __LINE__, this, processor, ignore ); );
			processor->debugIgnore = ignore;
			break;
		  case uDebuggerProtocolUnit::BbpMarkCondition:
			{
				int bp_no;
				ULThreadId ul_thread_id;
				pdu.readBbpMarkCondition( bp_no, ul_thread_id );
				debugger.setConditionMask( bp_no, ul_thread_id );
				uDEBUGPRT( uDebugPrt( "%d (uLocalDebuggerReader &)%p.main, read BbpMarkCondition %d\n", __LINE__, this, bp_no ); );
				break;
			}
		  case uDebuggerProtocolUnit::BbpClearCondition:
			{
				int bp_no;
				ULThreadId ul_thread_id;
				pdu.readBbpClearCondition( bp_no, ul_thread_id );
				debugger.resetConditionMask( bp_no, ul_thread_id );
				uDEBUGPRT( uDebugPrt( "%d (uLocalDebuggerReader &)%p.main, read BbpClearCondition %d\n", __LINE__, this, bp_no ); );
				break;
			}
		  default:
			abort( "(uLocalDebuggerReader &)%p.main : global debugger sent invalid request %d", this, pdu.getType() );
		} // switch
	} // for
} // uLocalDebuggerReader::main


uLocalDebuggerReader::uLocalDebuggerReader( uLocalDebugger &debugger, uSocketClient &sockClient ) :
		debugger(debugger), sockClient(sockClient), pdu( *new uDebuggerProtocolUnit ) {
} // uLocalDebuggerReader::uLocalDebuggerReader


uLocalDebuggerReader::~uLocalDebuggerReader() {
	delete &pdu;
} // uLocalDebuggerReader::~uLocalDebuggerReader


//######################### uLocalDebugger (cont'd) #########################


void uLocalDebugger::main() {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.main, starts\n", __LINE__, this ); );

	if ( ! attaching ) {
		char fullpath[FILENAME_MAX];
		if ( name[0] == '/' ) {
			fullpath[0] = 0;
		} else {
			if ( ! getcwd( fullpath, FILENAME_MAX - (strlen(name) + 1) ) ) {
				abort( "(uLocalDebugger &)%p.main : unable to obtain full pathname", this );
			} // if
			strcat( fullpath, "/" );
		} // if
		strcat( fullpath, name );
		uCreateLocalDebugger( port, machine, fullpath );
	} else {
		char machine[MAXHOSTNAMELEN + 1];
		if ( gethostname( machine, MAXHOSTNAMELEN + 1 ) == -1 ) {
			abort ( "could not get hostname, attach failed." ); // TEMPORARY: abort is a little harsh
		} // if
		const char *name = uDefAttachName;

		uCreateLocalDebugger( port, machine, name );

		uDebuggerProtocolUnit pdu;
		pdu.re_init( uDebuggerProtocolUnit::NoType );	// end of transfer
		pdu.createNapplicationAttached();
		send( pdu );
	} // if

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.main, port:%d, machine:%p, name:%p\n", __LINE__, this, port, machine, name ); );

	for ( ;; ) {
		uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.main, loop\n", __LINE__, this ); );
		_Accept( ~uLocalDebugger ) {
			break;
		} or _Accept( breakpointHandler ) {
		} or _Accept( unblockTask ) {
		} or _Accept( checkPointMX ) {
		} or _Accept( performAtomicOperation ) {
		} or _Accept( attachULThread ) {
			send( pdu );
		} or _Accept( createULThreadMX ) {
			send( pdu );
		} or _Accept( destroyULThreadMX ) {
			send( pdu );
		} or _Accept( createCluster ) {
			send( pdu );
		} or _Accept( destroyCluster ) {
			send( pdu );
		} or _Accept( createKernelThread ) {
			send( pdu );
		} or _Accept( destroyKernelThread ) {
			send( pdu );
		} or _Accept( migrateKernelThread ) {
			send( pdu );
		} or _Accept( migrateULThreadMX ) {
			send( pdu );
		} or _Accept( abortApplication ) {
		} or _Accept( finish ) {
			send( pdu );
			uLocalDebuggerActive = false;
		} // _Accept
	} // for

	deregister();										// finish local debugger

	delete [] bpc_list;

	// SKULLDUGGERY: uLocalDebuggerReader has to execute one more time to receive
	// the confirmation of the shutdown. However, when it executes, it appears
	// to the deadlock detection algorithm that the system is deadlocked
	// because there are no user tasks executing at this point in the shutdown.
	// To trick the deadlock detection algorithm, the number of debugger
	// blocked tasks is incremented so that it "appears" there is still a user
	// task executing.

	debugger_blocked_tasks += 1;

	delete dispatcher;
	delete sockClient;

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.main, done\n", __LINE__, this ); );
} // uLocalDebugger::main


void uLocalDebugger::setConditionMask( int bp_no, void *ul_thread_id ) {
	uBConditionEval *eval = new uBConditionEval( (ULThreadId )ul_thread_id );
	bpc_list[bp_no].add( eval );
} // uLocalDebugger::setConditionMask


void uLocalDebugger::resetConditionMask( int bp_no, void *ul_thread_id ) {
	// The uBConditionEval gets freed in uBConditionList::del.

	bool code = bpc_list[bp_no].del( (ULThreadId)ul_thread_id );

	// A non-existent conditional breakpoint should not be deleted.
	
	assert( code );
} // uLocalDebugger::resetConditionMask


void uLocalDebugger::deregister() {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.deregister, called\n", __LINE__, this ); );

  if ( ! uLocalDebuggerActive ) return;

	uDebuggerProtocolUnit pdu;

	// de-register the systemProcessor
	pdu.createNdestroyKernelThread( U_THIS_TASK, &uKernelModule::systemProcessor );
	send( pdu );

	// just consume the confirmation PDU and trust the global debugger
	receive();

	// de-register the systemCluster
	pdu.createNdestroyCluster( &uKernelModule::systemCluster );
	send( pdu );

	pdu.re_init( uDebuggerProtocolUnit::NoType );
	pdu.createNfinishLocalDebugger( true );
	send( pdu );
	// the confirmation is handled by the dispatcher task
} // uLocalDebugger::deregister


void uLocalDebugger::uCreateLocalDebugger( int port, const char *machine, const char *name ) {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.uCreateLocalDebugger, on its way...\n", __LINE__, this ); );

	sockClient = new uSocketClient( port, uSocket::gethostbyname( machine ) ); // create the socket client
	uGlobalDebuggerActive = true;						// global debugger has connected to socket

	// Turn on local debugger requests only after the socket is created.

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.uCreateLocalDebugger, sending init data : max_no_of_breakpoints %d\n", __LINE__, this, max_no_of_breakpoints ); );

	int length = strlen( name ) + 1;
	pdu.createNinitLocalDebugger( max_no_of_breakpoints, length );
	send( pdu );
	sockClient->write( name, length );
	readPDU( *sockClient, pdu );						// reader task not running yet so safe to read
	pdu.readCinitLocalDebugger();

#ifdef __U_MULTI__
	// Since stream I/O is done on the system cluster, ignore the migration
	// or the cost is too high.

	uKernelModule::systemCluster->debugIgnore = true;
#else
	// TEMPORARY: THE GLOBAL DEBUGGER FAILS IF THIS IS NOT SET TO FALSE: SOURCE
	// CODE FOR TASKS CANNOT BE FOUND. IT NEEDS FIXING.

	uKernelModule::systemProcessor->debugIgnore = false;
#endif // __U_MULTI__

	// now start the dispatcher task
	dispatcher = new uLocalDebuggerReader( *this, *sockClient );

	// register clusters
	uSeqIter<uClusterDL> ci;
	uClusterDL *cr;
	for ( ci.over( *uKernelModule::globalClusters ); ci >> cr; ) {
		uCluster &cluster = cr->cluster();
		uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.uCreateLocalDebugger, cluster is %s (%p)\n", __LINE__, this, cluster.getName(), &cluster ); );
		createCluster( cluster );
		send( pdu );

		// register processors associated with this cluster
		uProcessorDL *pr;
		for ( uSeqIter<uProcessorDL> iter( cluster.processorsOnCluster ); iter >> pr; ) {
			uProcessor &processor = pr->processor();
			uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.uCreateLocalDebugger, processor is %p\n", __LINE__, this, &processor ); );
			pdu.createNcreateKernelThread( U_THIS_TASK, U_THIS_CLUSTER, &processor, &cluster, processor.pid, processor.debugIgnore );
			send( pdu );
			receive();
		} // for

		// register tasks on the cluster
		uBaseTaskDL *bt;
		for ( uSeqIter<uBaseTaskDL> iter( cluster.tasksOnCluster ); iter >> bt; ) {
			uBaseTask &task = bt->task();
			uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.uCreateLocalDebugger, task %s (%p)\n", __LINE__, this, task.getName(), &task ); );
			// ignore  boot, system, local debugger, dispatcher and processor tasks

			if ( &task != uKernelModule::bootTask && &task != uKernelModule::systemTask &&
				 &task != uLocalDebuggerInstance && &task != dispatcher && std::addressof(task.bound_) == nullptr ) {
				// retrieve some important registers
				MinimalRegisterSet regs;
				int *registers = (int *)task.taskDebugMask;
				regs.pc = 0;
				regs.fp = registers[0];
				regs.sp = registers[1];
#ifdef __U_MULTI__
				pdu.createNattachULThread( &task, U_THIS_TASK, &cluster, regs, task.getName() );
#else
				// in uni-processor, pretend that every task exist on the system cluster
				pdu.createNattachULThread( &task, U_THIS_TASK, &uKernelModule::systemCluster, regs, task.getName() );
#endif
				send( pdu );
				receive();
				memset( task.taskDebugMask, 0, 8 );
			} // if
		} // for
	} // for

	// initialize conditional breakpoint array
	bpc_list = new uBConditionList[max_no_of_breakpoints]();

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.uCreateLocalDebugger, done\n", __LINE__, this ); );
} // uLocalDebugger::uCreateLocalDebugger


uLocalDebugger::uLocalDebugger( const char *port, const char *machine, const char *name ) :
		uBaseTask ( *uKernelModule::systemCluster ), port( atoi(port) ), machine( machine ), name( name ), pdu( *new uDebuggerProtocolUnit ) {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.uLocalDebugger( port:%s, machine:%s, name:%s, done\n", __LINE__, this, port, machine, name ); );

	debugger_blocked_tasks = 0;
	uLocalDebuggerActive = true;
	attaching = false;
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.uLocalDebugger, exit( port:%s, machine:%s, name:%s, done\n", __LINE__, this, port, machine, name ); );
} // uLocalDebugger::uLocalDebugger


uLocalDebugger::uLocalDebugger( int port ) : uBaseTask ( *uKernelModule::systemCluster ), port( port ), pdu( *new uDebuggerProtocolUnit ) {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.uLocalDebugger( port:%d ), done\n", __LINE__, this, port ); );

	debugger_blocked_tasks = 0;
	uLocalDebuggerActive = true;
	attaching = true;
} // uLocalDebugger::uLocalDebugger


uLocalDebugger::~uLocalDebugger() {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.~uLocalDebugger, called\n", __LINE__, this ); );

	uLocalDebuggerActive = false;
	delete &pdu;

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.~uLocalDebugger, done\n", __LINE__, this ); );
} // uLocalDebugger::~uLocalDebugger


void uLocalDebugger::send( uDebuggerProtocolUnit &pdu ) {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.send, type %d, total_size %d\n", __LINE__, this, pdu.getType(), pdu.total_size() ); );

	assert( uLocalDebuggerActive );
	sockClient->write( pdu.total_buffer(), pdu.total_size() );

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.send, done\n", __LINE__, this ); );
} // uLocalDebugger::send


void uLocalDebugger::receive() {						// only called by the local debugger
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.receive, task %s (%p) goes to sleep\n", __LINE__, this, (U_THIS_TASK)->getName(), U_THIS_TASK ); );

	assert( uLocalDebuggerActive );
	debugger_blocked_tasks += 1;
	_Accept( unblockTask );
	debugger_blocked_tasks -= 1;

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.receive, done\n", __LINE__, this ); );
} // uLocalDebugger::receive


bool uLocalDebugger::receive( uDebuggerProtocolUnit &pdu ) {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.receive, task %s (%p) goes to sleep / pdu is %p\n", __LINE__, this, (U_THIS_TASK)->getName(), U_THIS_TASK, &pdu ); );

	assert( uLocalDebuggerActive );
	debugger_blocked_tasks += 1;
	blockedNode node( U_THIS_TASK, pdu );
	blockedTasks.add( &node );
	node.wait.wait();
	blockedTasks.remove( &node );
	debugger_blocked_tasks -= 1;

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.receive, done, %p woke up\n", __LINE__, this, U_THIS_TASK ); );

	return true;
} // uLocalDebugger::receive


uLocalDebugger::blockedNode::blockedNode( uBaseTask *key, uDebuggerProtocolUnit &pdu ) : key(key), pdu(pdu) {}


void uLocalDebugger::unblockTask( uBaseTask *ul_thread, uDebuggerProtocolUnit &pdu ) {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.unblockTask, waking %p\n", __LINE__, this, ul_thread ); );

	blockedNode *p;
	for ( uSeqIter<blockedNode> iter(blockedTasks); iter >> p; ) {	// search for thread
	  if ( p->key == ul_thread ) break;
	} // for

	if ( p != nullptr ) {									// found
		p->pdu = pdu;									// copy result
		pdu = pdu;										// copy result
		p->wait.signal();								// wake thread
	} // if

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.unblockTask, woke %p\n", __LINE__, this, ul_thread ); );
} // uLocalDebugger::unblockTask


int uLocalDebugger::breakpointHandler( int no ) {
	int adjustment = 0;
	MinimalRegisterSet regs;

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.breakpointHandler, %d called\n", __LINE__, this, no ); );

	// save some important registers
	CREATE_MINIMAL_REGISTER_SET( regs );

	uBConditionEval *cond = bpc_list[no].search( U_THIS_TASK );
	if ( cond && cond->getOperator() == BreakpointCondition::NOT_SET ) {
		pdu.createArequestAddress( U_THIS_TASK, regs, no );
		send( pdu );
		if ( receive( pdu ) ) {
			pdu.readAreplyAddress( cond->bp_cond );
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.breakpointHandler, address received for bp[%d]: var1 [%d %d %d %d] var2 [%d %d %d %d] operation %d.\n",
					   __LINE__, this, no,
					   cond->bp_cond.var[0].offset, cond->bp_cond.var[0].field_off, cond->bp_cond.var[0].atype,
					   cond->bp_cond.var[0].vtype,
					   cond->bp_cond.var[1].offset, cond->bp_cond.var[1].field_off, cond->bp_cond.var[1].atype,
					   cond->bp_cond.var[1].vtype,cond->getOperator()); )

			// The following assert statement is a precaution - the
			// global debugger should not send unsupported breakpoint
			// information.
			assert( cond->bp_cond.var[0].atype != BreakpointCondition::UNSUPORTED );
			assert( cond->bp_cond.var[1].atype != BreakpointCondition::UNSUPORTED );
		} // if
	} // if

	if ( cond ) {										// set current fp and sp
		cond->setFp( regs.fp );
		cond->setSp( regs.sp );							// sp not used by current archs
	} // if

	if ( ! cond || cond->getOperator() == BreakpointCondition::NONE || cond->evaluate() ) {
		memset( U_THIS_TASK->taskDebugMask, 0, 8 );
		pdu.createNhitBreakpoint( U_THIS_TASK, no, regs );
		send( pdu );
		if ( receive( pdu ) ) {
			uBaseTask *task_address;
			pdu.readOcontULThread( task_address, U_THIS_TASK->taskDebugMask, adjustment );
		} // if
	} // if
	uDEBUGPRT( uDebugPrt( "%d uLocalDebugger::breakpointHandler : leaving with adjustment : %d\n", __LINE__, adjustment ); );
	return adjustment;
} // uLocalDebugger::breakpointHandler


void uLocalDebugger::performAtomicOperation() {
	// Executed by the thread of the reading task => writer task is blocked
	// during execution and none of the traversed data structures can change
	// because of the checkpointing into this task.

	uCluster *cluster;
	uDebuggerProtocolUnit pdu;

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.performAtomicOperation, sending atomic operation confirmation\n", __LINE__, this ); );

	pdu.createCconfirmAtomicOperation( true );
	send( pdu );

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.performAtomicOperation, waiting for code change check request\n", __LINE__, this ); );

	debugger_blocked_tasks += 1;						// pretend to be user task during this operation
	readPDU( *sockClient, pdu );
	debugger_blocked_tasks -= 1;

	if ( pdu.getType() == uDebuggerProtocolUnit::OcheckCodeRange ) {
		long *low1, *high1, *low2, *high2;
		bool result = true;
		pdu.readOcheckCodeRange( low1, high1, low2, high2, cluster );

		uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.performAtomicOperation, checking range %p - %p and %p - %p for cluster %p\n",
							  __LINE__, this, low1, high1, low2, high2, cluster ); )

#ifndef __U_MULTI__
		uSeqIter<uClusterDL> ci;
		uClusterDL *cr;
		for ( ci.over( *uKernelModule::globalClusters ); ci >> cr; ) {
			cluster = &cr->cluster();
#endif // __U_MULTI__

			uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.performAtomicOperation, checking cluster %p\n", __LINE__, this, cluster ); );

			uSeqIter<uBaseTaskDL> ci;
			uBaseTaskDL *cr;
			for ( ci.over( cluster->tasksOnCluster ); ci >> cr; ) {

				uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.performAtomicOperation, task %s (%p) at %p\n",
									  __LINE__, this, cr->task().getName(), &cr->task(), cr->task().debugPCandSRR ); )

				if ( ( (long *)cr->task().debugPCandSRR >= low1 && (long *)cr->task().debugPCandSRR < high1 )
					|| ( (long *)cr->task().debugPCandSRR >= low2 && (long *)cr->task().debugPCandSRR < high2 ) ) {
					result = false;

					uDEBUGPRT( 	uDebugPrt( "%d (uLocalDebugger &)%p.performAtomicOperation, interference !!!!\n", __LINE__, this ); );

					goto end;
				} // if
			} // for

#ifndef __U_MULTI__
		} // for
#endif //  __U_MULTI__

	  end: ;

		uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.performAtomicOperation, sending confirmation\n", __LINE__, this ); );

		pdu.re_init( uDebuggerProtocolUnit::NoType );
		pdu.createCconfirmCodeRange( result );
		send( pdu );

		// now the poke is done by the global debugger

		uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.performAtomicOperation, wait for confirmation\n", __LINE__, this ); );

		debugger_blocked_tasks += 1;					// pretend to be user task during this operation
		readPDU( *sockClient, pdu );
		debugger_blocked_tasks -= 1;
	} // if

	pdu.readCfinishAtomicOperation();

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.performAtomicOperation, done, got CfinishAtomicOperation\n", __LINE__, this ); );
} // uLocalDebugger::performAtomicOperation


void uLocalDebugger::attachULThread( uBaseTask *this_task, uCluster *this_cluster __attribute__(( unused )) ) {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.attachULThread, task %s (%p) / cluster %p\n", __LINE__, this, this_task->getName(), this_task, this_cluster ); );

  if ( ! uLocalDebuggerActive ) return;

	// retrieve some important registers
	MinimalRegisterSet regs;
	int *registers = (int *)this_task->taskDebugMask;
	regs.pc = 0;
	regs.fp = registers[0];
	regs.sp = registers[1];
#ifdef __U_MULTI__
	pdu.createNattachULThread( this_task, U_THIS_TASK, this_cluster, regs, this_task->getName() );
#else
	// in uni-processor, pretend that every task exist on the system cluster
	// WHY IS THIS DONE LIKE THIS?
	pdu.createNattachULThread( this_task, U_THIS_TASK, &uKernelModule::systemCluster, regs, this_task->getName() );
#endif

	if ( receive( pdu ) ) {
		uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.attachULThread, attach confirmation received\n", __LINE__, this ); );
		memset( this_task->taskDebugMask, 0, 8 );
	} // if

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.attachULThread, done\n", __LINE__, this ); );
} // uLocalDebugger::attachULThread


void uLocalDebugger::checkPoint() {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.checkPoint, task %s (%p) / cluster %p\n", __LINE__, this, U_THIS_TASK->getName(), U_THIS_TASK, U_THIS_CLUSTER ); );

  if ( U_THIS_TASK == uLocalDebuggerInstance || U_THIS_TASK == dispatcher || std::addressof(U_THIS_TASK->bound_) != nullptr ) return;
	checkPointMX();
} // uLocalDebugger::checkPoint


void uLocalDebugger::checkPointMX() {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.checkPointMX, task %s (%p) / cluster %p\n", __LINE__, this, U_THIS_TASK->getName(), U_THIS_TASK, U_THIS_CLUSTER ); );
} // uLocalDebugger::checkPointMX


void uLocalDebugger::createULThread() {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.createULThread, task %p (%s) / cluster %p (%s)\n",
						  __LINE__, this, U_THIS_TASK, U_THIS_TASK->getName(), U_THIS_CLUSTER, U_THIS_CLUSTER->getName() ); )
	MinimalRegisterSet regs;
	CREATE_MINIMAL_REGISTER_SET( regs );				// save caller information

  if ( U_THIS_TASK == uLocalDebuggerInstance || U_THIS_TASK == dispatcher || std::addressof(U_THIS_TASK->bound_) != nullptr ) {
		// save the regs here for attaching later on
		int *registers = (int *)U_THIS_TASK->taskDebugMask;
		registers[0] = regs.fp;
		registers[1] = regs.sp;
		return;
	} // if

	createULThreadMX( regs );							// obtain mutual exclusion
} // uLocalDebugger::createULThread


void uLocalDebugger::createULThreadMX( MinimalRegisterSet &regs ) {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.createULThreadMX, task %p (%s) / cluster %p (%s)\n",

						  __LINE__, this, U_THIS_TASK, U_THIS_TASK->getName(), U_THIS_CLUSTER, U_THIS_CLUSTER->getName() ); )

  if ( ! uLocalDebuggerActive ) return;

#ifdef __U_MULTI__
	pdu.createNcreateULThread( U_THIS_TASK, U_THIS_CLUSTER, regs, U_THIS_TASK->getName() );
#else
	// in uni-processor, pretend that every task exist on the system cluster
	// WHY IS THIS DONE LIKE THIS?
	pdu.createNcreateULThread( U_THIS_TASK, &uKernelModule::systemCluster, regs, U_THIS_TASK->getName() );
#endif
	if ( receive( pdu ) ) {
		int dummy;
		uBaseTask *task_address;
		pdu.readOcontULThread( task_address, U_THIS_TASK->taskDebugMask, dummy );
	} // if
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.createULThreadMX, done\n", __LINE__, this ); );
} // uLocalDebugger::createULThreadMX


void uLocalDebugger::destroyULThread() {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.destroyULThread, task %p (%s) / cluster %p (%s)\n",
						  __LINE__, this, U_THIS_TASK, U_THIS_TASK->getName(), U_THIS_CLUSTER, U_THIS_CLUSTER->getName() ); )

  if ( U_THIS_TASK == uLocalDebuggerInstance || U_THIS_TASK == dispatcher || U_THIS_TASK == uThisProcessor().procTask ) return;

	destroyULThreadMX();								// obtain mutual exclusion
} // uLocalDebugger::destroyULThread


void uLocalDebugger::destroyULThreadMX() {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.destroyULThreadMX, task %p (%s) / cluster %p (%s)\n",
						  __LINE__, this, U_THIS_TASK, U_THIS_TASK->getName(), U_THIS_CLUSTER, U_THIS_CLUSTER->getName() ); )

  if ( ! uLocalDebuggerActive ) return;

	pdu.createNdestroyULThread( U_THIS_TASK );
	receive( pdu );										// receive confirmation

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.destroyULThreadMX, done\n", __LINE__, this ); );
} // uLocalDebugger::destroyULThreadMX


void uLocalDebugger::createCluster( uCluster &cluster ) {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.createCluster, cluster %p\n", __LINE__, this, &cluster ); );

  if ( ! uLocalDebuggerActive ) return;

	pdu.createNcreateCluster( &cluster, cluster.getName(), cluster.debugIgnore );

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.createCluster, type %d, done\n", __LINE__, this, pdu.getType() ); );
} // uLocalDebugger::createCluster


void uLocalDebugger::destroyCluster( uCluster &cluster ) {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.destroyCluster, cluster %p\n", __LINE__, this, &cluster ); );

  if ( ! uLocalDebuggerActive ) return;

	pdu.createNdestroyCluster( &cluster );

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.destroyCluster, done\n", __LINE__, this ); );
} // uLocalDebugger::destroyCluster


void uLocalDebugger::createKernelThread( uProcessor &process, uCluster &cluster ) {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.createKernelThread, creating task: %s (%p), creating cluster:%p, process %p / cluster %p / pid %d\n",
						  __LINE__, this, (U_THIS_TASK)->getName(), U_THIS_TASK, U_THIS_CLUSTER, &process, &cluster, process.pid ); )

  if ( ! uLocalDebuggerActive ) return;

	pdu.createNcreateKernelThread( U_THIS_TASK, U_THIS_CLUSTER, &process, &cluster, process.pid, process.debugIgnore );
	receive( pdu );										// receive confirmation

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.createKernelThread, done\n", __LINE__, this ); );
} // uLocalDebugger::createKernelThread


void uLocalDebugger::destroyKernelThread( uProcessor &process ) {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.destroyKernelThread, process %p\n", __LINE__, this, &process ); );

  if ( ! uLocalDebuggerActive ) return;

	pdu.createNdestroyKernelThread( U_THIS_TASK, &process );
	receive( pdu );										// receive confirmation

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.destroyKernelThread, done\n", __LINE__, this ); );
} // uLocalDebugger::destroyKernelThread


void uLocalDebugger::migrateKernelThread( uProcessor &process, uCluster &to_cluster ) {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.migrateKernelThread, process %p / to_cluster %p\n", __LINE__, this, &process, &to_cluster ); );

  if ( ! uLocalDebuggerActive ) return;
  if ( U_THIS_CLUSTER->debugIgnore || to_cluster.debugIgnore || process.debugIgnore ) return;

	pdu.createNmigrateKernelThread( U_THIS_TASK, &process, &to_cluster );
	receive( pdu );										// receive confirmation

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.migrateKernelThread, done\n", __LINE__, this ); );
} // uLocalDebugger::migrateKernelThread


void uLocalDebugger::migrateULThread( uCluster &to_cluster ) {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.migrateULThread, task %s (%p) / to_cluster %p\n",
						  __LINE__, this, (U_THIS_TASK)->getName(), U_THIS_TASK, &to_cluster ); )

  if ( U_THIS_CLUSTER->debugIgnore || to_cluster.debugIgnore ) return;

	migrateULThreadMX( to_cluster );					// obtain mutual exclusion
} // uLocalDebugger::migrateULThread


void uLocalDebugger::migrateULThreadMX( uCluster &to_cluster ) {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.migrateULThreadMX, task %s (%p) / to_cluster %p\n",
						  __LINE__, this, (U_THIS_TASK)->getName(), U_THIS_TASK, &to_cluster ); )

  if ( ! uLocalDebuggerActive ) return;

	pdu.createNmigrateULThread( U_THIS_TASK, &to_cluster );
	receive( pdu );										// receive confirmation

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.migrateULThreadMX, done\n", __LINE__, this ); );
} // uLocalDebugger::migrateULThreadMX


void uLocalDebugger::finish() {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.finish, called\n", __LINE__, this ); );

	pdu.createNfinishLocalDebugger( false );
} // uLocalDebugger::finish


void uLocalDebugger::cont_handler( __U_SIGTYPE__ ) {
	uDEBUGPRT( uDebugPrt( "%d uLocalDebugger::cont_handler, got signal\n", __LINE__ ); );
	abort_confirmed = true;
} // cont_handler


void uLocalDebugger::abortApplication() {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.abortApplication\n", __LINE__, this ); );

  if ( ! uLocalDebuggerActive ) return;
  if ( U_THIS_TASK == uLocalDebuggerInstance ) return;	// prevent recursion

	UPP::uSigHandlerModule::signal( SIGCONT, cont_handler ); // install SIGCONT handler
	pdu.createNabortApplication( uThisProcessor().getPid() );
	send( pdu );
	while ( ! abort_confirmed ) {						// busy wait for a response
		sigset_t mask;
		sigemptyset( &mask );
		sigsuspend( &mask );							// block processor and no signals are blocked
	} // while

	uDEBUGPRT( uDebugPrt( "%d (uLocalDebugger &)%p.abortApplication, done\n", __LINE__, this ); );
} // uLocalDebugger::abortApplication


//######################### uLocalDebuggerBoot #########################


uLocalDebuggerBoot::uLocalDebuggerBoot() {
	uCount += 1;
	if ( uCount == 1 ) {
		char *port = getenv( "_KDB_DEBUGGER_PORT_" );
		char *machine = getenv( "_KDB_DEBUGGER_MACHINE_" );
		char *name = getenv( "_KDB_DEBUGGER_TARGET_NAME_" );

		// If the magic environment variables exists, the global debugger is
		// running and this application can connect to it through a socket.

		if ( port != nullptr && machine != nullptr && name != nullptr ) {
			uLocalDebugger::uLocalDebuggerInstance = new uLocalDebugger( port, machine, name );
		} // if
	} // if
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebuggerBoot &)%p.uLocalDebuggerBoot\n", __LINE__, this ); );
} // uLocalDebuggerBoot::uLocalDebuggerBoot

uLocalDebuggerBoot::~uLocalDebuggerBoot() {
	uDEBUGPRT( uDebugPrt( "%d (uLocalDebuggerBoot &)%p.~uLocalDebuggerBoot\n", __LINE__, this ); );
	uCount -= 1;
	if ( uCount == 0 ) {
		if ( uLocalDebugger::uLocalDebuggerInstance != nullptr ) {
			delete uLocalDebugger::uLocalDebuggerInstance;
			uLocalDebugger::uLocalDebuggerInstance = nullptr; // no more calls to local debugger
		} // if
	} // if
} // uLocalDebuggerBoot::~uLocalDebuggerBoot


int uLocalDebuggerBoot::uCount = 0;


void uLocalDebugger::uLocalDebuggerEndCode() {}


// Local Variables: //
// tab-width: 4 //
// compile-command: "make install" //
// End: //
