//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1997
// 
// uDebuggerProtocolUnit.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Wed Oct 15 13:16:10 1997
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr  5 08:06:46 2022
// Update Count     : 15
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
#include <uDebuggerProtocolUnit.h>


void uDebuggerProtocolUnit::re_init( RequestType type ) {
	pdu_data.request_type = type;
	switch(type) {
	case NoType:
		size = 0;
		break;
	case NinitLocalDebugger:
		size = sizeof(initLocalDebuggerPDU);
		break;
	case NcreateULThread:
		size = sizeof(createULThreadPDU);
		break;
	case NattachULThread:
		size = sizeof(attachULThreadPDU);
		break;
	case NdestroyULThread:
		size = sizeof(ULThreadId);
		break;
	case NcreateKernelThread:
		size = sizeof(createKernelThreadPDU);
		break;
	case NdestroyKernelThread:
		size = sizeof(destroyKernelThreadPDU);
		break;
	case NcreateCluster:
		size = sizeof(createClusterPDU);
		break;
	case NdestroyCluster:
		size = sizeof(ClusterId);
		break;
	case NmigrateULThread:
		size = sizeof(migrateULThreadPDU);
		break;
	case NmigrateKernelThread:
		size = sizeof(migrateKernelThreadPDU);
		break;
	case NhitBreakpoint:
		size = sizeof(hitBreakpointPDU);
		break;
	case NfinishLocalDebugger:
		size = sizeof(bool);
		break;
	case NabortApplication:
		size = sizeof(int);
		break;
	case NapplicationAttached:
		size = 0;
		break;
	case OcontULThread:
		size = sizeof(contULThreadPDU);
		break;
	case OshutdownConnection:
		size = 0;
		break;
	case OcheckCodeRange:
		size = sizeof(checkCodeRangePDU);
		break;
	case OignoreClusterMigration:
		size = sizeof(ignoreClusterMigrationPDU);
		break;
	case OignoreKernelThreadMigration:
		size = sizeof(ignoreKernelThreadMigrationPDU);
		break;
	case OstartAtomicOperation:
		size = 0;
		break;
	case CconfirmCodeRange:
		size = sizeof(confirmCodeRangePDU);
		break;
	case CconfirmAtomicOperation:
		size = sizeof(confirmAtomicOperationPDU);
		break;
	case CgeneralConfirmation:
		size = sizeof(ULThreadId);
		break;
	case CinitLocalDebugger:
		size = 0;
		break;
	case CfinishLocalDebugger:
		size = 0;
		break;
	case CfinishAtomicOperation:
		size = 0;
		break;
	case ArequestAddress:
		size = sizeof(requestAddressPDU);
		break;
	case AreplyAddress:
		size = sizeof(replyAddressPDU);
		break;
	case BbpMarkCondition:
		size = sizeof(bpMarkConditionPDU);
		break;
	case BbpClearCondition:
		size = sizeof(bpMarkConditionPDU);
		break;
	default:
																assert(0);
	}
} // void uDebuggerProtocolUnit::re_init

uDebuggerProtocolUnit::uDebuggerProtocolUnit( RequestType type ) {
	re_init( type );
}

uDebuggerProtocolUnit::~uDebuggerProtocolUnit() {}

int uDebuggerProtocolUnit::total_size() {
//													assert( pdu_data.request_type != NoType );
	return size + sizeof(RequestType);
}

InternalAddress uDebuggerProtocolUnit::total_buffer() {
//													assert( pdu_data.request_type != NoType );
	return (InternalAddress) &pdu_data;
}

int uDebuggerProtocolUnit::data_size() {
//													assert( pdu_data.request_type != NoType );
	return size;
}

InternalAddress uDebuggerProtocolUnit::data_buffer() {
//													assert( pdu_data.request_type != NoType );
	return (InternalAddress) &pdu_data.data;
}

void uDebuggerProtocolUnit::createNinitLocalDebugger ( int max_no_of_breakpoints, int path_length ) {
//									assert( pdu_data.request_type == NoType );
	pdu_data.data.init_local_debugger_data.max_no_of_breakpoints = max_no_of_breakpoints;
	pdu_data.data.init_local_debugger_data.path_length = path_length;
	pdu_data.request_type = NinitLocalDebugger;
	size = sizeof(initLocalDebuggerPDU);
}

void uDebuggerProtocolUnit::createNcreateULThread( ULThreadId ul_thread_id, ClusterId cluster_id, MinimalRegisterSet& reg_set, const char* name ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.data.create_ul_thread_data.ul_thread_id = ul_thread_id;
	pdu_data.data.create_ul_thread_data.cluster_id = cluster_id;
	pdu_data.data.create_ul_thread_data.reg_set = reg_set;
	memcpy( pdu_data.data.create_ul_thread_data.name, (char*)name, maxEntityName );
	// just to be sure
	pdu_data.data.create_ul_thread_data.name[maxEntityName - 1] = '\0';
	pdu_data.request_type = NcreateULThread;
	size = sizeof(createULThreadPDU);
}

void uDebuggerProtocolUnit::createNattachULThread( ULThreadId ul_thread_id, ULThreadId ret_thread_id, ClusterId cluster_id, MinimalRegisterSet& reg_set, const char* name ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.data.attach_ul_thread_data.ul_thread_id = ul_thread_id;
	pdu_data.data.attach_ul_thread_data.ret_thread_id = ret_thread_id;
	pdu_data.data.attach_ul_thread_data.cluster_id = cluster_id;
	pdu_data.data.attach_ul_thread_data.reg_set = reg_set;
	memcpy( pdu_data.data.attach_ul_thread_data.name, (char*)name, maxEntityName );
	// just to be sure
	pdu_data.data.attach_ul_thread_data.name[maxEntityName - 1] = '\0';
	pdu_data.request_type = NattachULThread;
	size = sizeof(attachULThreadPDU);
}

void uDebuggerProtocolUnit::createNdestroyULThread( ULThreadId ul_thread_id ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.data.ul_thread_id = ul_thread_id;
	pdu_data.request_type = NdestroyULThread;
	size = sizeof(ULThreadId);
}

void uDebuggerProtocolUnit::createNcreateKernelThread( ULThreadId ul_thread_id, ClusterId cluster_id_exec, KernelThreadId kernel_thread_id, ClusterId cluster_id, OSKernelThreadId os_kernel_thread_id, bool copy_from_init ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.data.create_kernel_thread_data.ul_thread_id = ul_thread_id;
	pdu_data.data.create_kernel_thread_data.kernel_thread_id = kernel_thread_id;
	pdu_data.data.create_kernel_thread_data.os_kernel_thread_id = os_kernel_thread_id;
	pdu_data.data.create_kernel_thread_data.cluster_id = cluster_id;
	pdu_data.data.create_kernel_thread_data.cluster_id_exec = cluster_id_exec;
	pdu_data.data.create_kernel_thread_data.copy_from_init = copy_from_init;
	pdu_data.request_type = NcreateKernelThread;
	size = sizeof(createKernelThreadPDU);
}

void uDebuggerProtocolUnit::createNdestroyKernelThread( ULThreadId ul_thread_id, KernelThreadId kernel_thread_id ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.data.destroy_kernel_thread_data.kernel_thread_id = kernel_thread_id;
	pdu_data.data.destroy_kernel_thread_data.ul_thread_id = ul_thread_id;
	pdu_data.request_type = NdestroyKernelThread;
	size = sizeof(destroyKernelThreadPDU);
}

void uDebuggerProtocolUnit::createNcreateCluster( ClusterId cluster_id, const char* name, bool copy_from_init ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.data.create_cluster_data.cluster_id = cluster_id;
	memcpy( pdu_data.data.create_cluster_data.name, (char*)name, maxEntityName );
	// just to be sure
	pdu_data.data.create_cluster_data.name[maxEntityName - 1] = '\0';
	pdu_data.data.create_cluster_data.copy_from_init = copy_from_init;
	pdu_data.request_type = NcreateCluster;
	size = sizeof(createClusterPDU);
}

void uDebuggerProtocolUnit::createNdestroyCluster( ClusterId cluster_id ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.data.cluster_id = cluster_id;
	pdu_data.request_type = NdestroyCluster;
	size = sizeof(ClusterId);
}

void uDebuggerProtocolUnit::createNmigrateULThread( ULThreadId ul_thread_id, ClusterId cluster_id_to ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.data.migrate_ul_thread_data.ul_thread_id = ul_thread_id;
	pdu_data.data.migrate_ul_thread_data.cluster_id_to = cluster_id_to;
	pdu_data.request_type = NmigrateULThread;
	size = sizeof(migrateULThreadPDU);
}

void uDebuggerProtocolUnit::createNmigrateKernelThread( ULThreadId ul_thread_id, KernelThreadId kernel_thread_id, ClusterId cluster_id_to ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.data.migrate_kernel_thread_data.ul_thread_id = ul_thread_id;
	pdu_data.data.migrate_kernel_thread_data.kernel_thread_id = kernel_thread_id;
	pdu_data.data.migrate_kernel_thread_data.cluster_id_to = cluster_id_to;
	pdu_data.request_type = NmigrateKernelThread;
	size = sizeof(migrateKernelThreadPDU);
}

void uDebuggerProtocolUnit::createNhitBreakpoint( ULThreadId ul_thread_id, int breakpoint_no, MinimalRegisterSet& reg_set ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.data.hit_breakpoint_data.ul_thread_id = ul_thread_id;
	pdu_data.data.hit_breakpoint_data.breakpoint_no = breakpoint_no;
	pdu_data.data.hit_breakpoint_data.reg_set = reg_set;
	pdu_data.request_type = NhitBreakpoint;
	size = sizeof(hitBreakpointPDU);
}

void uDebuggerProtocolUnit::createNfinishLocalDebugger( bool deliver ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.request_type = NfinishLocalDebugger;
	pdu_data.data.deliver = deliver;
	size = sizeof(bool);
}

void uDebuggerProtocolUnit::createNabortApplication( uPid_t pid ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.request_type = NabortApplication;
	pdu_data.data.pid = pid;
	size = sizeof(int);
}

void uDebuggerProtocolUnit::createNapplicationAttached() {
//	                                                assert( pdu_data.request_type == NoType );
	pdu_data.request_type = NapplicationAttached;
	size = 0;
}

void uDebuggerProtocolUnit::createOcontULThread( ULThreadId ul_thread_id, char* bp_mask, int adjustment ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.data.cont_ul_thread_data.ul_thread_id = ul_thread_id;
	pdu_data.data.cont_ul_thread_data.adjustment = adjustment;
	if ( bp_mask ) {
		memcpy( (char*)pdu_data.data.cont_ul_thread_data.bp_mask, (char*)bp_mask, SIZE_OF_BREAKPOINT_FIELD / NBBY );
	} else {
		memset( (char*)pdu_data.data.cont_ul_thread_data.bp_mask, 0, SIZE_OF_BREAKPOINT_FIELD / NBBY );
	}
	pdu_data.request_type = OcontULThread;
	size = sizeof(contULThreadPDU);
}

void uDebuggerProtocolUnit::createOshutdownConnection() {
//													assert( pdu_data.request_type == NoType );
	pdu_data.request_type = OshutdownConnection;
	size = 0;
}

void uDebuggerProtocolUnit::createOcheckCodeRange( CodeAddress low_pc_app, CodeAddress high_pc_app, CodeAddress low_pc_handler, CodeAddress high_pc_handler, ClusterId cluster_id ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.request_type = OcheckCodeRange;
	pdu_data.data.check_breakpoint_range_data.low_pc_app = low_pc_app;
	pdu_data.data.check_breakpoint_range_data.high_pc_app = high_pc_app;
	pdu_data.data.check_breakpoint_range_data.low_pc_handler = low_pc_handler;
	pdu_data.data.check_breakpoint_range_data.high_pc_handler = high_pc_handler;
	pdu_data.data.check_breakpoint_range_data.cluster_id = cluster_id;
	size = sizeof(checkCodeRangePDU);
}

void uDebuggerProtocolUnit::createOignoreClusterMigration( ClusterId cluster_id, bool ignore ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.request_type = OignoreClusterMigration;
	pdu_data.data.ignore_cluster_migration_data.cluster_id = cluster_id;
	pdu_data.data.ignore_cluster_migration_data.ignore = ignore;
	size = sizeof(ignoreClusterMigrationPDU);
}

void uDebuggerProtocolUnit::createOignoreKernelThreadMigration( KernelThreadId kernel_thread_id, bool ignore ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.request_type = OignoreKernelThreadMigration;
	pdu_data.data.ignore_kernel_thread_migration_data.kernel_thread_id = kernel_thread_id;
	pdu_data.data.ignore_kernel_thread_migration_data.ignore = ignore;
	size = sizeof(ignoreKernelThreadMigrationPDU);
}

void uDebuggerProtocolUnit::createOstartAtomicOperation() {
//													assert( pdu_data.request_type == NoType );
	pdu_data.request_type = OstartAtomicOperation;
	size = 0;
}

void uDebuggerProtocolUnit::createCconfirmCodeRange( bool ok ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.request_type = CconfirmCodeRange;
	pdu_data.data.confirm_breakpoint_range_data.ok = ok;
	size = sizeof(confirmCodeRangePDU);
}

void uDebuggerProtocolUnit::createCconfirmAtomicOperation ( bool ok ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.request_type = CconfirmAtomicOperation;
	pdu_data.data.confirm_atomic_operation_data.ok = ok;
	size = sizeof(confirmAtomicOperationPDU);
}

void uDebuggerProtocolUnit::createCgeneralConfirmation ( ULThreadId ul_thread_id ) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.data.ul_thread_id = ul_thread_id;
	pdu_data.request_type = CgeneralConfirmation;
	size = sizeof(ULThreadId);
}

void uDebuggerProtocolUnit::createCfinishLocalDebugger () {
//													assert( pdu_data.request_type == NoType );
	pdu_data.request_type = CfinishLocalDebugger;
	size = 0;
}

void uDebuggerProtocolUnit::createCinitLocalDebugger () {
//													assert( pdu_data.request_type == NoType );
	pdu_data.request_type = CinitLocalDebugger;
	size = 0;
}

void uDebuggerProtocolUnit::createCfinishAtomicOperation () {
//													assert( pdu_data.request_type == NoType );
	pdu_data.request_type = CfinishAtomicOperation;
	size = 0;
}

void uDebuggerProtocolUnit::createArequestAddress (ULThreadId ul_thread_id, MinimalRegisterSet& reg_set, int bp_no) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.data.addr_request.ul_thread_id = ul_thread_id;
	pdu_data.data.addr_request.bp_no = bp_no;
	pdu_data.request_type = ArequestAddress ;
	pdu_data.data.addr_request.reg_set = reg_set;
	size = sizeof(requestAddressPDU);
}

void uDebuggerProtocolUnit::createAreplyAddress (ULThreadId ul_thread_id, long field_off1, long offset1, BreakpointCondition::AddressType atp1, 
														BreakpointCondition::VariableType vtp1, long field_off2, long offset2, 
														BreakpointCondition::AddressType atp2, BreakpointCondition::VariableType vtp2, 
														BreakpointCondition::OperationType opt) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.request_type = AreplyAddress;
	pdu_data.data.addr_reply.ul_thread_id = ul_thread_id;
	pdu_data.data.addr_reply.bp_condition.var[0].field_off   = field_off1;
	pdu_data.data.addr_reply.bp_condition.var[0].offset = offset1;
	pdu_data.data.addr_reply.bp_condition.var[0].atype  = atp1;
	pdu_data.data.addr_reply.bp_condition.var[0].vtype  = vtp1;
	pdu_data.data.addr_reply.bp_condition.var[1].field_off   = field_off2;
	pdu_data.data.addr_reply.bp_condition.var[1].offset = offset2;
	pdu_data.data.addr_reply.bp_condition.var[1].atype  = atp2;
	pdu_data.data.addr_reply.bp_condition.var[1].vtype  = vtp2;
	pdu_data.data.addr_reply.bp_condition.Operator      = opt;
	size = sizeof(replyAddressPDU);
}

void uDebuggerProtocolUnit::createBbpMarkCondition(int bp_no, ULThreadId ul_thread_id) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.data.bp_add.bp_no = bp_no;
	pdu_data.data.bp_add.ul_thread_id = ul_thread_id;
	pdu_data.request_type = BbpMarkCondition;
	size = sizeof(bpMarkConditionPDU);
}

void uDebuggerProtocolUnit::createBbpClearCondition (int bp_no, ULThreadId ul_thread_id) {
//													assert( pdu_data.request_type == NoType );
	pdu_data.data.bp_clear.bp_no = bp_no;
	pdu_data.data.bp_clear.ul_thread_id = ul_thread_id;
	pdu_data.request_type = BbpClearCondition;
	size = sizeof(bpMarkConditionPDU);
}

uDebuggerProtocolUnit::RequestType uDebuggerProtocolUnit::getType() {
	return pdu_data.request_type;
}

//
// Routines reading from the socket.
//

void	uDebuggerProtocolUnit::readNinitLocalDebugger ( int& max_no_of_breakpoints, int& path_length ) {
													assert( pdu_data.request_type == NinitLocalDebugger );
	max_no_of_breakpoints = pdu_data.data.init_local_debugger_data.max_no_of_breakpoints;
	path_length = pdu_data.data.init_local_debugger_data.path_length;
}

void uDebuggerProtocolUnit::readNcreateULThread( ULThreadId& ul_thread_id, ClusterId& cluster_id, MinimalRegisterSet &reg_set, char*& name ) {
													assert( pdu_data.request_type == NcreateULThread );
	ul_thread_id = pdu_data.data.create_ul_thread_data.ul_thread_id;
	cluster_id = pdu_data.data.create_ul_thread_data.cluster_id;
	reg_set = pdu_data.data.create_ul_thread_data.reg_set;
	name = pdu_data.data.create_ul_thread_data.name;
}

void uDebuggerProtocolUnit::readNattachULThread( ULThreadId& ul_thread_id, ULThreadId& ret_thread_id, ClusterId& cluster_id, MinimalRegisterSet &reg_set, char*& name ) {
													assert( pdu_data.request_type == NattachULThread );
	ul_thread_id = pdu_data.data.attach_ul_thread_data.ul_thread_id;
	ret_thread_id = pdu_data.data.attach_ul_thread_data.ret_thread_id;
	cluster_id = pdu_data.data.attach_ul_thread_data.cluster_id;
	reg_set = pdu_data.data.attach_ul_thread_data.reg_set;
	name = pdu_data.data.attach_ul_thread_data.name;
}

void uDebuggerProtocolUnit::readNdestroyULThread( ULThreadId& ul_thread_id ) {
													assert( pdu_data.request_type == NdestroyULThread );
	ul_thread_id = pdu_data.data.ul_thread_id;
}

void uDebuggerProtocolUnit::readNcreateKernelThread( ULThreadId& ul_thread_id, ClusterId &cluster_id_exec, KernelThreadId& kernel_thread_id, ClusterId& cluster_id, OSKernelThreadId& os_kernel_thread_id, bool& copy_from_init ) {
													assert( pdu_data.request_type == NcreateKernelThread );
	ul_thread_id = pdu_data.data.create_kernel_thread_data.ul_thread_id;
	kernel_thread_id = pdu_data.data.create_kernel_thread_data.kernel_thread_id;
	os_kernel_thread_id = pdu_data.data.create_kernel_thread_data.os_kernel_thread_id;
	cluster_id = pdu_data.data.create_kernel_thread_data.cluster_id;
	cluster_id_exec = pdu_data.data.create_kernel_thread_data.cluster_id_exec;
	copy_from_init = pdu_data.data.create_kernel_thread_data.copy_from_init;
}

void uDebuggerProtocolUnit::readNdestroyKernelThread( ULThreadId& ul_thread_id, KernelThreadId& kernel_thread_id ) {
													assert( pdu_data.request_type == NdestroyKernelThread );
	kernel_thread_id = pdu_data.data.destroy_kernel_thread_data.kernel_thread_id;
	ul_thread_id = pdu_data.data.destroy_kernel_thread_data.ul_thread_id;
}

void uDebuggerProtocolUnit::readNcreateCluster( ClusterId& cluster_id, char*& name, bool& copy_from_init ) {
													assert( pdu_data.request_type == NcreateCluster );
	cluster_id = pdu_data.data.create_cluster_data.cluster_id;
	name = pdu_data.data.create_cluster_data.name;
	copy_from_init = pdu_data.data.create_cluster_data.copy_from_init;
}

void uDebuggerProtocolUnit::readNdestroyCluster( ClusterId& cluster_id ) {
													assert( pdu_data.request_type == NdestroyCluster );
	cluster_id = pdu_data.data.cluster_id;
}

void uDebuggerProtocolUnit::readNmigrateULThread( ULThreadId& ul_thread_id, ClusterId& cluster_id_to ) {
													assert( pdu_data.request_type == NmigrateULThread );
	ul_thread_id = pdu_data.data.migrate_ul_thread_data.ul_thread_id;
	cluster_id_to = pdu_data.data.migrate_ul_thread_data.cluster_id_to;
}

void uDebuggerProtocolUnit::readNmigrateKernelThread( ULThreadId& ul_thread_id, KernelThreadId& kernel_thread_id, ClusterId& cluster_id_to ) {
													assert( pdu_data.request_type == NmigrateKernelThread );
	ul_thread_id = pdu_data.data.migrate_kernel_thread_data.ul_thread_id;
	kernel_thread_id = pdu_data.data.migrate_kernel_thread_data.kernel_thread_id;
	cluster_id_to = pdu_data.data.migrate_kernel_thread_data.cluster_id_to;
}

void uDebuggerProtocolUnit::readNhitBreakpoint( ULThreadId& ul_thread_id, int& breakpoint_no, MinimalRegisterSet& reg_set ) {
													assert( pdu_data.request_type == NhitBreakpoint );
	ul_thread_id = pdu_data.data.hit_breakpoint_data.ul_thread_id;
	breakpoint_no = pdu_data.data.hit_breakpoint_data.breakpoint_no;
	reg_set = pdu_data.data.hit_breakpoint_data.reg_set;
}

void	uDebuggerProtocolUnit::readNfinishLocalDebugger( bool& deliver ) {
													assert( pdu_data.request_type == NfinishLocalDebugger );
	deliver = pdu_data.data.deliver;
}

void	uDebuggerProtocolUnit::readNabortApplication( uPid_t &pid ) {
													assert( pdu_data.request_type == NabortApplication );
	pid = pdu_data.data.pid;
}

void uDebuggerProtocolUnit::readNapplicationAttached() {
													assert( pdu_data.request_type == NapplicationAttached );
}

void uDebuggerProtocolUnit::readOcontULThread( ULThreadId& ul_thread_id, char* bp_mask, int &adjustment ) {
													assert( pdu_data.request_type == OcontULThread );
	ul_thread_id = pdu_data.data.cont_ul_thread_data.ul_thread_id;
	adjustment = pdu_data.data.cont_ul_thread_data.adjustment;
	memcpy( (char*)bp_mask, (char*)pdu_data.data.cont_ul_thread_data.bp_mask, SIZE_OF_BREAKPOINT_FIELD / NBBY );
}

void uDebuggerProtocolUnit::readOshutdownConnection() {
													assert( pdu_data.request_type == OshutdownConnection );
}

void uDebuggerProtocolUnit::readOcheckCodeRange( CodeAddress& low_pc_app, CodeAddress& high_pc_app, CodeAddress& low_pc_handler, CodeAddress& high_pc_handler, ClusterId& cluster_id ) {
													assert( pdu_data.request_type == OcheckCodeRange );
	low_pc_app = pdu_data.data.check_breakpoint_range_data.low_pc_app;
	high_pc_app = pdu_data.data.check_breakpoint_range_data.high_pc_app;
	low_pc_handler = pdu_data.data.check_breakpoint_range_data.low_pc_handler;
	high_pc_handler = pdu_data.data.check_breakpoint_range_data.high_pc_handler;
	cluster_id = pdu_data.data.check_breakpoint_range_data.cluster_id;
}

void uDebuggerProtocolUnit::readOignoreClusterMigration( ClusterId& cluster_id, bool& ignore ) {
													assert( pdu_data.request_type == OignoreClusterMigration );
	cluster_id = pdu_data.data.ignore_cluster_migration_data.cluster_id;
	ignore = pdu_data.data.ignore_cluster_migration_data.ignore;
}

void uDebuggerProtocolUnit::readOignoreKernelThreadMigration( KernelThreadId& kernel_thread_id, bool& ignore ) {
													assert( pdu_data.request_type == OignoreKernelThreadMigration );
	kernel_thread_id = pdu_data.data.ignore_kernel_thread_migration_data.kernel_thread_id;
	ignore = pdu_data.data.ignore_kernel_thread_migration_data.ignore;
}

void uDebuggerProtocolUnit::readOstartAtomicOperation() {
													assert( pdu_data.request_type == OstartAtomicOperation );
}

void uDebuggerProtocolUnit::readCconfirmCodeRange( bool &ok ) {
													assert( pdu_data.request_type == CconfirmCodeRange );
	ok = pdu_data.data.confirm_breakpoint_range_data.ok;
}

void uDebuggerProtocolUnit::readCconfirmAtomicOperation ( bool &ok ) {
													assert( pdu_data.request_type == CconfirmAtomicOperation );
	ok = pdu_data.data.confirm_atomic_operation_data.ok;
}

ULThreadId uDebuggerProtocolUnit::readCgeneralConfirmation () {
													assert( pdu_data.request_type == CgeneralConfirmation || 
															 pdu_data.request_type == OcontULThread ||
															 pdu_data.request_type == AreplyAddress);
	if ( pdu_data.request_type == CgeneralConfirmation ) {
		return pdu_data.data.ul_thread_id;
	} 
	else if ( pdu_data.request_type == OcontULThread) {
		return pdu_data.data.cont_ul_thread_data.ul_thread_id;
	}
	return pdu_data.data.addr_reply.ul_thread_id;
}

void uDebuggerProtocolUnit::readCfinishLocalDebugger () {
													assert( pdu_data.request_type == CfinishLocalDebugger );
}

void uDebuggerProtocolUnit::readCinitLocalDebugger () {
													assert( pdu_data.request_type == CinitLocalDebugger );
}

void uDebuggerProtocolUnit::readCfinishAtomicOperation () {
													assert( pdu_data.request_type == CfinishAtomicOperation );
}

void uDebuggerProtocolUnit::readArequestAddress (ULThreadId& ul_thread_id, MinimalRegisterSet& reg_set, int& bp_no ) {
	                                                assert( pdu_data.request_type == ArequestAddress );
	ul_thread_id = pdu_data.data.addr_request.ul_thread_id;
	bp_no = pdu_data.data.addr_request.bp_no;
	reg_set = pdu_data.data.addr_request.reg_set;
}

void uDebuggerProtocolUnit::readAreplyAddress (BreakpointCondition& bp_condition) {
	                                                assert( pdu_data.request_type == AreplyAddress );
	bp_condition = pdu_data.data.addr_reply.bp_condition;													
}

void uDebuggerProtocolUnit::readBbpMarkCondition(int& bp_no, ULThreadId& ul_thread_id) {
	                                                assert( pdu_data.request_type == BbpMarkCondition );
	bp_no = pdu_data.data.bp_add.bp_no;
	ul_thread_id = pdu_data.data.bp_add.ul_thread_id;
}

void uDebuggerProtocolUnit::readBbpClearCondition (int &bp_no, ULThreadId& ul_thread_id) {
	                                                assert( pdu_data.request_type == BbpClearCondition );
	bp_no = pdu_data.data.bp_clear.bp_no;
	ul_thread_id = pdu_data.data.bp_clear.ul_thread_id;
}


// Local Variables: //
// tab-width: 4 //
// compile-command: "make install" //
// End: //
