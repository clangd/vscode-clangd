//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// abortExit.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Fri Oct 26 11:54:31 1990
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Sep 30 16:02:30 2022
// Update Count     : 671
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
#include <uHeapLmmm.h>
#ifdef __U_PROFILER__
#include <uProfiler.h>
#endif // __U_PROFILER__

#include <uDebug.h>										// access: uDebugWrite
#undef __U_DEBUG_H__									// turn off debug prints

#include <cstdio>
#include <cstdarg>
#include <cerrno>
#include <unistd.h>										// _exit

using namespace UPP;

#include <execinfo.h>									// backtrace, backtrace_symbols
#include <cxxabi.h>										// __cxa_demangle

static void uBacktrace( int start ) {
	enum {
		Frames = 50,									// maximum number of stack frames
		Last = 2,										// skip last N stack frames
	};

	void * array[Frames];
	size_t size = ::backtrace( array, Frames );
	char ** messages = ::backtrace_symbols( array, size ); // does not demangle names
	char helpText[256];
	int len;

	*index( messages[0], '(' ) = '\0';					// find executable name
	len = snprintf( helpText, 256, "Stack back trace for: %s\n", messages[0] );
	uDebugWrite( STDERR_FILENO, helpText, len );

	// skip last stack frame after uMain
	for ( unsigned int i = start; i < size - Last && messages != nullptr; i += 1 ) {
		char * mangled_name = nullptr, * offset_begin = nullptr, * offset_end = nullptr;

		for ( char * p = messages[i]; *p; ++p ) {		// find parantheses and +offset
			if ( *p == '(' ) {
				mangled_name = p;
			} else if ( *p == '+' ) {
				offset_begin = p;
			} else if ( *p == ')' ) {
				offset_end = p;
				break;
			} // if
		} // for

		// if line contains symbol, attempt to demangle
		int frameNo = i - start;
		if ( mangled_name && offset_begin && offset_end && mangled_name < offset_begin ) {
			*mangled_name++ = '\0';						// delimit strings
			*offset_begin++ = '\0';
			*offset_end++ = '\0';

			int status;
			char * real_name = __cxxabiv1::__cxa_demangle( mangled_name, 0, 0, &status );
			// bug in __cxa_demangle for single-character lower-case non-mangled names
			if ( status == 0 ) {						// demangling successful ?
				len = snprintf( helpText, 256, "(%d) %s %s+%s%s\n",
								frameNo, messages[i], strncmp( real_name, "uCpp_main", 9 ) == 0 ? &real_name[5] : real_name, offset_begin, offset_end );
			} else {									// otherwise, output mangled name
				len = snprintf( helpText, 256, "(%d) %s %s(/*unknown*/)+%s%s\n",
								frameNo, messages[i], mangled_name, offset_begin, offset_end );
			} // if

			free( real_name );
		} else {										// otherwise, print the whole line
			len = snprintf( helpText, 256, "(%d) %s\n", frameNo, messages[i] );
		} // if
		uDebugWrite( STDERR_FILENO, helpText, len );
	} // for
	free( messages );
} // uBacktrace


void exit( int retcode ) __THROW {						// interpose
	RealRtn::exit( retcode );							// call the real exit
	// CONTROL NEVER REACHES HERE!
} // exit

void exit( int retcode, const char fmt[], ... ) __THROW {
	va_list args;
	va_start( args, fmt );
	vfprintf( stderr, fmt, args );
	va_end( args );
	RealRtn::exit( retcode );							// call the real exit
	// CONTROL NEVER REACHES HERE!
} // exit


// Used by multiple abort function, must use "va_list args" not "...".
void uAbort( uSigHandlerModule::SignalAbort signalAbort, const char fmt[], va_list args ) __attribute__(( __nothrow__, __leaf__, __noreturn__ ));
void uAbort( uSigHandlerModule::SignalAbort signalAbort, const char fmt[], va_list args ) {
#if defined( __U_MULTI__ )
	// abort cannot be recursively entered by the same or different processors because all signal handlers return when
	// the globalAbort flag is true.
	uKernelModule::globalAbortLock->acquire();
	if ( uKernelModule::globalAbort ) {					// not first task to abort ?
		uKernelModule::globalAbortLock->release();
		sigset_t mask;
		sigemptyset( &mask );
		sigaddset( &mask, SIGALRM );					// block SIGALRM signals
		sigaddset( &mask, SIGUSR1 );					// block SIGUSR1 signals
		sigsuspend( &mask );							// block the processor to prevent further damage during abort
		_exit( EXIT_FAILURE );							// if processor unblocks before it is killed, terminate it
	} else {
		uKernelModule::globalAbort = true;				// first task to abort ?
		uKernelModule::globalAbortLock->release();
	} // if
#endif // __U_MULTI__

	signal( SIGABRT, SIG_DFL );							// prevent final "real" abort from recursing to handler

#ifdef __U_STATISTICS__
	if ( Statistics::prtStatTerm() ) Statistics::print();
	if ( uHeapControl::prtHeapTerm() ) malloc_stats();
#endif // __U_STATISTICS__

	uBaseTask &task = uThisTask();						// optimization

#ifdef __U_PROFILER__
	// profiling

	task.profileInactivate();							// make sure the profiler is not called from this point on
#endif // __U_PROFILER__

#ifdef __U_DEBUG__
	// Turn off uOwnerLock checking.
	uKernelModule::initialized = false;
#endif // __U_DEBUG__

	enum { BufferSize = 1024 };
	static char helpText[BufferSize];
	int len = snprintf( helpText, BufferSize, "uC++ Runtime error (UNIX pid:%ld) ", (long int)getpid() ); // use UNIX pid (versus getPid)
	uDebugWrite( STDERR_FILENO, helpText, len );

	if ( fmt ) {										// null format may cause problems
		// Display the relevant shut down information.
		len = vsnprintf( helpText, BufferSize, fmt, args );
		uDebugWrite( STDERR_FILENO, helpText, len );
		if ( fmt[strlen( fmt ) - 1] != '\n' ) {			// add optional newline if missing at the end of the format text
			uDebugWrite( STDERR_FILENO, "\n", 1 );
		} // if
	} // if

	len = snprintf( helpText, BufferSize, "Error occurred while executing task %.256s (%p)", task.getName(), &task );
	uDebugWrite( STDERR_FILENO, helpText, len );
	if ( &task != &uThisCoroutine() ) {
		len = snprintf( helpText, BufferSize, " in coroutine %.256s (%p).\n", uThisCoroutine().getName(), &uThisCoroutine() );
		uDebugWrite( STDERR_FILENO, helpText, len );
	} else {
		uDebugWrite( STDERR_FILENO, ".\n", 2 );
	} // if

	uBacktrace( signalAbort == uSigHandlerModule::Yes ?
				// SKULLDUGGERY: the active frame varies and some of these are ignored.
#if defined( __U_MULTI__ )
				3 : 1
#else
				4 : 2
#endif // __U_MULTI__
		);

	// In debugger mode, tell the global debugger to stop the application.

#if __U_LOCALDEBUGGER_H__
	if ( ! uKernelModuleBoot::uKernelModuleData.disableInt && ! uKernelModuleBoot::uKernelModuleData.disableIntSpin ) {
		if ( uLocalDebugger::uLocalDebuggerActive ) uLocalDebugger::uLocalDebuggerInstance->abortApplication();
	} // if
#endif // __U_LOCALDEBUGGER_H__

	// After having killed off the other processors, dump core

	RealRtn::abort();									// call the real abort
	// CONTROL NEVER REACHES HERE!
} // abort

// Only one processor should call abort and succeed.  Once a processor calls abort, all other processors quietly exit
// while the aborting processor cleans up the system and possibly dumps core.

void abort( uSigHandlerModule::SignalAbort signalAbort, const char fmt[], ... ) {
	va_list args;
	va_start( args, fmt );
	uAbort( signalAbort, fmt, args );
	// CONTROL NEVER REACHES HERE!
} // abort

void abort( const char fmt[], ... ) {
	va_list args;
	va_start( args, fmt );
	uAbort( uSigHandlerModule::No, fmt, args );
	// CONTROL NEVER REACHES HERE!
} // abort

void abort( void ) {									// interpose
	abort( uSigHandlerModule::No, "%s", "" );
} // abort


// Local Variables: //
// compile-command: "make install" //
// End: //
