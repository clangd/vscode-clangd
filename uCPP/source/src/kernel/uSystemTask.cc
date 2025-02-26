//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1998
// 
// uSystemTask.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Jun 22 15:25:53 1998
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Apr  3 10:50:56 2022
// Update Count     : 118
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
#include <uSystemTask.h>

//#include <uDebug.h>


void uSystemTask::pthreadDetachEnd( uBaseTask &victim ) {
	uSystemTask::victim = &victim;
} // uSystemTask::pthreadDetachEnd


void uSystemTask::main() {
	for ( ;; ) {
		_Accept( ~uSystemTask ) {
			break;
		} or _Accept( reaper ) {
			delete victim;
		} or _Accept( pthreadDetachEnd ) {
			delete victim;
// #if __U_LOCALDEBUGGER_H__
// 	} or _Timeout( uDuration( 1 ) ) {		// 1 second
// #endif // __U_LOCALDEBUGGER_H__
		} // _Accept

#if __U_LOCALDEBUGGER_H__
		// check for debugger attach request

		if ( uKernelModule::attaching != 0 ) {
			uDEBUFPRT( uDebugPrt( "(uSystemTask &)%p.main, attaching\n", this ); );
			int port = uKernelModule::attaching;
			uKernelModule::attaching = 0;		// reset flag so don't do this again
			uLocalDebugger::uLocalDebuggerInstance = new uLocalDebugger( port );
			uDEBUGPRT( uDebugPrt( "(uSystemTask &)%p.main, local debugger started\n", this ); );
		} // if
#endif // __U_LOCALDEBUGGER_H__
	} // for
} // uSystemTask::main


uSystemTask::uSystemTask() : uBaseTask( *uKernelModule::systemCluster ) {
} // uSystemTask::uSystemTask


uSystemTask::~uSystemTask() {
} // uSystemTask::~uSystemTask


void uSystemTask::reaper( uBaseTask &victim ) {
	uSystemTask::victim = &victim;
} // uSystemTask::reaper


// Local Variables: //
// compile-command: "make install" //
// End: //
