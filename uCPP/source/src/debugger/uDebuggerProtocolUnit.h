//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Martin Karsten 1995
// 
// uDebuggerProtocolUnit.h -- 
// 
// Author           : Martin Karsten
// Created On       : Thu Apr 20 21:36:45 1995
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr  5 08:12:46 2022
// Update Count     : 59
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


#ifndef _uDebuggerProtocolUnit_h_
#define _uDebuggerProtocolUnit_h_ 1


#include "LangBasics.h"
#include "ArchBasics.h"

#ifndef NBBY
#define NBBY 8
#endif

#define uDefAttachName "-*-uAttachName-*-"

struct BreakpointCondition {
	enum OperationType {
		NOT_SET,										// Not yet set
		NONE,											// set with no condition (error in condition)
		EQUAL,
		NOT_EQUAL,
		GREATER,
		GREATER_EQUAL,
		LESS,
		LESS_EQUAL
	};

	enum AddressType {
		UNSUPORTED,
		LOCAL,
		STATIC,
		REGISTER,
		CONST
	};


	enum VariableType {
		INVALID,
		INT,
		PTR,											// pointer type
		FLOAT,
		DOUBLE,
		CHAR
	};
	
	struct ConditionVar {
		long field_off;									// field offset of a field in a struct or class
		long offset;
		AddressType atype;
		VariableType vtype;
	};

	ConditionVar var[2];
	OperationType Operator;
	long fp;
	long sp;
};


class uDebuggerProtocolUnit {
  public:
	enum GRequestType {
		NoType = 0,
		// notifications local debugger -> global debugger
		NinitLocalDebugger,								//  1
		NcreateULThread,								//  2
		NattachULThread,                                //  3
		NdestroyULThread,								//  4
		NcreateKernelThread,							//  5
		NdestroyKernelThread,							//  6
		NcreateCluster,									//  7
		NdestroyCluster,								//  8
		NmigrateULThread,								//  9
		NmigrateKernelThread,							// 10
		NhitBreakpoint,									// 11
		NfinishLocalDebugger,							// 12
		NabortApplication,								// 13
		NapplicationAttached,                           // 14
		// operations global debugger -> local debugger
		OcontULThread,									// 15
		OshutdownConnection,							// 16
		OcheckCodeRange,								// 17
		OstartAtomicOperation,							// 18
		OignoreClusterMigration,						// 19
		OignoreKernelThreadMigration,					// 20
		// confirmation local debugger -> global debugger
		CconfirmCodeRange,								// 21
		CconfirmAtomicOperation,						// 22
		// confirmation global debugger -> local debugger
		CgeneralConfirmation,							// 23
		CinitLocalDebugger,								// 24
		CfinishLocalDebugger,							// 25
		CfinishAtomicOperation,							// 26
		ArequestAddress,                                // 27
		AreplyAddress,                                  // 28
		BbpMarkCondition,                               // 29
	    BbpClearCondition,                              // 30
	};

	static const int maxEntityName = 64;
  private:
	struct createULThreadPDU {
		ULThreadId			ul_thread_id;
		ClusterId			cluster_id;
		MinimalRegisterSet	reg_set;
		char				name[maxEntityName];
	};

	struct attachULThreadPDU {
		ULThreadId			ul_thread_id;
		ULThreadId          ret_thread_id;
		ClusterId			cluster_id;
		MinimalRegisterSet	reg_set;
		char				name[maxEntityName];
	};

	struct createClusterPDU {
		ClusterId			cluster_id;
		char				name[maxEntityName];
		bool				copy_from_init;
		};

	struct createKernelThreadPDU {
		ULThreadId			ul_thread_id;
		ClusterId			cluster_id_exec;
		KernelThreadId		kernel_thread_id;
		ClusterId			cluster_id;
		OSKernelThreadId	os_kernel_thread_id;
		bool				copy_from_init;
	};

	struct destroyKernelThreadPDU {
		ULThreadId			ul_thread_id;
		KernelThreadId		kernel_thread_id;
	};
	
	struct migrateULThreadPDU {
		ULThreadId			ul_thread_id;
		ClusterId			cluster_id_to;
	};
	                
	struct migrateKernelThreadPDU {
		ULThreadId			ul_thread_id;
		KernelThreadId		kernel_thread_id;
		ClusterId			cluster_id_to;
	};

	struct hitBreakpointPDU {
		ULThreadId			ul_thread_id;
		int					breakpoint_no;
		MinimalRegisterSet	reg_set;
	};

	struct contULThreadPDU {
		ULThreadId			ul_thread_id;
		char				bp_mask[SIZE_OF_BREAKPOINT_FIELD / NBBY];
		int					adjustment;
	};

	struct initLocalDebuggerPDU {
		int					max_no_of_breakpoints;
		int					path_length;
	};

	struct checkCodeRangePDU {
		CodeAddress			low_pc_app;
		CodeAddress			high_pc_app;
		CodeAddress			low_pc_handler;
		CodeAddress			high_pc_handler;
		ClusterId			cluster_id;
	};

	struct ignoreClusterMigrationPDU {
		ClusterId			cluster_id;
		bool				ignore;
	};

	struct ignoreKernelThreadMigrationPDU {
		KernelThreadId		kernel_thread_id;
		bool				ignore;
	};

	struct confirmCodeRangePDU {
		bool				ok;
	};

	struct confirmAtomicOperationPDU {
		bool				ok; 
	};

	struct requestAddressPDU {
		ULThreadId			ul_thread_id;
		MinimalRegisterSet	reg_set;
		int                 bp_no;
	};

	struct replyAddressPDU {
		BreakpointCondition bp_condition;
		ULThreadId	        ul_thread_id;
	};

	struct bpMarkConditionPDU {
		int  				bp_no;
		ULThreadId			ul_thread_id;
	};

	struct PDU_DATA {
		GRequestType request_type;
		union Data {
			ULThreadId						ul_thread_id;
			ClusterId						cluster_id;
			createULThreadPDU				create_ul_thread_data;
			attachULThreadPDU				attach_ul_thread_data;
			createClusterPDU				create_cluster_data;
			createKernelThreadPDU			create_kernel_thread_data;
			destroyKernelThreadPDU			destroy_kernel_thread_data;
			migrateULThreadPDU				migrate_ul_thread_data;
			migrateKernelThreadPDU			migrate_kernel_thread_data;
			hitBreakpointPDU				hit_breakpoint_data;
			contULThreadPDU					cont_ul_thread_data;
			initLocalDebuggerPDU			init_local_debugger_data;
			checkCodeRangePDU				check_breakpoint_range_data;
			confirmCodeRangePDU				confirm_breakpoint_range_data;
			confirmAtomicOperationPDU		confirm_atomic_operation_data;
			ignoreClusterMigrationPDU		ignore_cluster_migration_data;
			ignoreKernelThreadMigrationPDU	ignore_kernel_thread_migration_data;
			uPid_t							pid;
			bool							deliver;
			requestAddressPDU               addr_request;
	        replyAddressPDU                 addr_reply;
			bpMarkConditionPDU              bp_add;
			bpMarkConditionPDU              bp_clear;
		} data;
//		PDU_DATA( GRequestType request_type ) : request_type(request_type) {}
	};

public:
	typedef GRequestType	RequestType;

private:
	PDU_DATA	pdu_data;
	int			size;

public:
	uDebuggerProtocolUnit( RequestType type = NoType );
	~uDebuggerProtocolUnit();

	// methods for a sender to create a protocol unit
	void createNinitLocalDebugger ( int max_no_of_breakpoints, int path_length );
	void createNcreateULThread( ULThreadId ul_thread_id, ClusterId cluster_id, MinimalRegisterSet& reg_set, const char* name );
	void createNattachULThread( ULThreadId ul_thread_id, ULThreadId ret_thread_id, ClusterId cluster_id, MinimalRegisterSet& reg_set, const char* name );
	void createNdestroyULThread( ULThreadId ul_thread_id );
	void createNcreateKernelThread( ULThreadId ul_thread_id, ClusterId cluster_id_exec, KernelThreadId kernel_thread_id, ClusterId cluster_id, OSKernelThreadId os_kernel_thread_id, bool copy_from_init = false );
	void createNdestroyKernelThread( ULThreadId ul_thread_id, KernelThreadId kernel_thread_id );
	void createNcreateCluster( ClusterId cluster_id, const char* name, bool copy_from_init = false );
	void createNdestroyCluster( ClusterId cluster_id );
	void createNmigrateULThread( ULThreadId ul_thread_id, ClusterId cluster_id_to );
	void createNmigrateKernelThread( ULThreadId ul_thread_id, KernelThreadId kernel_thread_id, ClusterId cluster_id_to );
	void createNhitBreakpoint( ULThreadId ul_thread_id, int breakpoint_no, MinimalRegisterSet& reg_set );
	void createNfinishLocalDebugger( bool deliver );
	void createNabortApplication( uPid_t pid );
	void createNapplicationAttached ();
	void createOcontULThread( ULThreadId ul_thread_id, char *bp_mask, int adjustment = 0 );
	void createOshutdownConnection();
	void createOcheckCodeRange( CodeAddress low_pc_app, CodeAddress high_pc_app, CodeAddress low_pc_handler, CodeAddress high_pc_handler, ClusterId cluster_id );
	void createOstartAtomicOperation();
	void createOignoreClusterMigration( ClusterId cluster_id, bool ignore );
	void createOignoreKernelThreadMigration( KernelThreadId kernel_thread_id, bool ignore );
	void createCconfirmCodeRange( bool ok );
	void createCconfirmAtomicOperation( bool ok );
	void createCgeneralConfirmation( ULThreadId ul_thread_id );
	void createCinitLocalDebugger();
	void createCfinishLocalDebugger();
	void createCfinishAtomicOperation();
	void createArequestAddress (ULThreadId ul_thread_id, MinimalRegisterSet& reg_set, int bp_no);
	void createAreplyAddress (ULThreadId ul_thread_id, long field_off1, long offset1, BreakpointCondition::AddressType atp1, BreakpointCondition::VariableType vtp1, 
							  long field_off2, long offset2, BreakpointCondition::AddressType atp2, BreakpointCondition::VariableType vtp2, 
							  BreakpointCondition::OperationType opt);
	void createBbpMarkCondition(int bp_no, ULThreadId ul_thread_id );
	void createBbpClearCondition (int bp_no, ULThreadId ul_thread_id );

	// to re-initialize an allocated unit
	void re_init( RequestType type );

	// methods for a sender to access the pure data storage
	int				total_size();
	InternalAddress	total_buffer();

	// methods for a receiver to access the pure data storage
	int				data_size();
	InternalAddress	data_buffer();

	// methods for a receiver to read the protocol unit
	RequestType		getType();

	// all arguments are return values
	void readNinitLocalDebugger ( int& max_no_of_breakpoints, int& pathlength );
	void readNcreateULThread( ULThreadId& ul_thread_id, ClusterId& cluster_id, MinimalRegisterSet& reg_set, char*& name );
	void readNattachULThread( ULThreadId& ul_thread_id, ULThreadId& ret_thread_id, ClusterId& cluster_id, MinimalRegisterSet& reg_set, char*& name );
	void readNdestroyULThread( ULThreadId& ul_thread_id );
	void readNcreateKernelThread( ULThreadId& ul_thread_id, ClusterId& cluster_id_exec, KernelThreadId& kernel_thread_id, ClusterId& cluster_id, OSKernelThreadId& os_kernel_thread_id, bool& copy_from_init );
	void readNdestroyKernelThread( ULThreadId& ul_thread_id, KernelThreadId& kernel_thread_id );
	void readNcreateCluster( ClusterId& cluster_id, char*& name, bool& copy_from_init );
	void readNdestroyCluster( ClusterId& cluster_id );
	void readNmigrateULThread( ULThreadId& ul_thread_id, ClusterId& cluster_id_to );
	void readNmigrateKernelThread( ULThreadId& ul_thread_id, KernelThreadId& kernel_thread_id, ClusterId& cluster_id_to );
	void readNhitBreakpoint( ULThreadId& ul_thread_id, int& breakpoint_no, MinimalRegisterSet& reg_set );
	void readNfinishLocalDebugger( bool& deliver );
	void readNabortApplication( uPid_t &pid );
	void readNapplicationAttached ();
	void readOcontULThread( ULThreadId& ul_thread_id, char* bp_mask, int& adjustment );
	void readOshutdownConnection();
	void readOcheckCodeRange( CodeAddress& low_pc_app, CodeAddress& high_pc_app, CodeAddress& low_pc_handler, CodeAddress& high_pc_handler, ClusterId& cluster_id );
	void readOstartAtomicOperation();
	void readOignoreClusterMigration( ClusterId& cluster_id, bool& ignore );
	void readOignoreKernelThreadMigration( KernelThreadId& kernel_thread_id, bool& ignore );
	void readCconfirmCodeRange( bool& ok );
	void readCconfirmAtomicOperation( bool &ok );
	ULThreadId readCgeneralConfirmation();
	void readCinitLocalDebugger();
	void readCfinishLocalDebugger();
	void readCfinishAtomicOperation();
	void readArequestAddress (ULThreadId& ul_thread_id, MinimalRegisterSet& reg_set, int& bp_no );
	void readAreplyAddress (BreakpointCondition& bp_condition);
	void readBbpMarkCondition(int& bp_no, ULThreadId& ul_thread_id);
	void readBbpClearCondition (int &bp_no, ULThreadId& ul_thread_id);
}; // class uDebuggerProtocolUnit


#endif // _uDebuggerProtocolUnit_h_


// Local Variables: //
// tab-width: 4 //
// compile-command: "make install" //
// End: //
