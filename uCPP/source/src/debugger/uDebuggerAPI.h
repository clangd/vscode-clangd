//                              -*- Mode: C++ -*- 
// 
// kdb, debugger for uC++, Copyright (C) Jun Shih 1996
// 
// API.h -- 
// 
// Author           : Jun Shih
// Created On       : Thu May 30 15:26:00 1996
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr  5 08:05:21 2022
// Update Count     : 36
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


#ifndef __U_DEBUGGERAPI_H__
#define __U_DEBUGGERAPI_H__


#define MAX_CMD_LEN		256
#define MAX_VAR_LEN		64
#define MAX_COND_LEN	128
#define MAX_PATH_LEN	256
#define MAX_PRINT_LEN	1024


typedef void *ThreadId;
typedef void *ListId;
typedef int  NotifyMsg;

enum MessageType {
	BP_SET,
	BP_CLEAR,
	CONTINUE,
	STOP,
	PRINT,
	ATTACH,
	CLUSTER_LIST,
	PROCESSOR_LIST,
	THREAD_LIST,
	BP_HIT,
	PROGRAM_TERMINATED,
	TERMINATE,
};


struct BP_SET_MSG {
	MessageType msg;
	ThreadId thread_id;
	char break_cmd[MAX_CMD_LEN + MAX_COND_LEN];
};


struct BP_CLEAR_MSG {
	MessageType msg;
	ThreadId thread_id;
	char clear_cmd[MAX_CMD_LEN];
};


struct CONTINUE_MSG {
	MessageType msg;
	ThreadId thread_id;
};


typedef CONTINUE_MSG STOP_MSG;


struct PRINT_MSG {
	MessageType msg;
	ThreadId thread_id;
	char var_name[MAX_VAR_LEN];
};


struct ATTACH_MSG {
	MessageType msg;
	int  pid;
	char path[MAX_PATH_LEN];
};


struct TERMNIATE_MSG {
	MessageType msg;
};


typedef TERMNIATE_MSG LIST_MSG;


// Notify Messages

struct BP_HIT_NOTIFY {
	MessageType msg;
	ThreadId thread_id;
};


typedef TERMNIATE_MSG PROGRAM_TERMINATED_NOTIFY;


struct GENERAL_NOTIFY {
	MessageType msg;
	NotifyMsg  nmsg;
};


struct PRINT_NOTIFY {
	MessageType msg;
	NotifyMsg nmsg;
	char print[MAX_PRINT_LEN];
};


struct LIST_NOTIFY {
	MessageType msg;
	NotifyMsg nmsg;
	ListId id;
};


#endif // __U_DEBUGGERAPI_H__


// Local Variables: //
// tab-width: 4 //
// compile-command: "make install" //
// End: //
