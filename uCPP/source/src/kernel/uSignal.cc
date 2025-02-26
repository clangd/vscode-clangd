//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uSignal.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sun Dec 19 16:32:13 1993
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Jul  7 11:53:29 2023
// Update Count     : 1010
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
#include <uDebug.h>										// access: uDebugWrite
#undef __U_DEBUG_H__									// turn off debug prints

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <unistd.h>										// _exit
#include <sys/wait.h>
#include <sys/types.h>
#include <ucontext.h>


namespace UPP {
	sigset_t uSigHandlerModule::block_mask;

	void uSigHandlerModule::signal( int sig, void (*handler)(__U_SIGPARMS__), int flags ) { // name clash with uSignal statement
		struct sigaction act;

		act.sa_sigaction = (void (*)(int, siginfo_t *, void *))handler;
		sigemptyset( &act.sa_mask );
		sigaddset( &act.sa_mask, SIGALRM );				// disabled during signal handler
		sigaddset( &act.sa_mask, SIGUSR1 );
		sigaddset( &act.sa_mask, SIGSEGV );
		sigaddset( &act.sa_mask, SIGBUS );
		sigaddset( &act.sa_mask, SIGILL );
		sigaddset( &act.sa_mask, SIGFPE );
		sigaddset( &act.sa_mask, SIGHUP );				// revert to default on second delivery
		sigaddset( &act.sa_mask, SIGTERM );
		sigaddset( &act.sa_mask, SIGINT );

		#ifdef __U_PROFILER__
		sigaddset( &act.sa_mask, SIGVTALRM );
		#if defined( __U_HW_OVFL_SIG__ )
		sigaddset( &act.sa_mask, __U_HW_OVFL_SIG__ );
		#endif // __U_HW_OVFL_SIG__
		#endif // __U_PROFILER__

		act.sa_flags = flags;

		if ( sigaction( sig, &act, nullptr ) == -1 ) {
			// THE KERNEL IS NOT STARTED SO CALL NO uC++ ROUTINES!
			char helpText[256];
			int len = snprintf( helpText, 256, "uSigHandlerModule::signal( sig:%d, handler:%p, flags:%d ), problem installing signal handler, error(%d) %s.\n",
								sig, handler, flags, errno, strerror( errno ) );
			uDebugWrite( STDERR_FILENO, helpText, len );
			_exit( EXIT_FAILURE );
		} // if
	} // uSigHandlerModule::signal


	static inline void * signalContextPC( __U_SIGCXT__ cxt ) {
		#if defined( __i386__ )
		return (void *)(cxt->uc_mcontext.gregs[REG_EIP]);
		#elif defined( __x86_64__ )
		return (void *)(cxt->uc_mcontext.gregs[REG_RIP]);
		#elif defined( __arm_64__ )
		return (void *)(cxt->uc_mcontext.pc);
		#else
			#error uC++ : internal error, unsupported architecture
		#endif // architecture
	} // signalContextPC


	static inline void * signalContextSP( __U_SIGCXT__ cxt ) {
		#if defined( __i386__ )
		return (void *)(cxt->uc_mcontext.gregs[REG_ESP]);
		#elif defined( __x86_64__ )
		return (void *)(cxt->uc_mcontext.gregs[REG_RSP]);
		#elif defined( __arm_64__ )
		return (void *)(cxt->uc_mcontext.sp);
		#else
			#error uC++ : internal error, unsupported architecture
		#endif // architecture
	} // signalContextSP


	// SKULLDUGGERY: Magic variables bracketing text_nopreempt section for non-preemptable routines.
	extern "C" {
		extern void * __start_text_nopreempt;
		extern void * __stop_text_nopreempt;
	}

	// Safe to make direct accesses through TLS pointer because interrupts are masked during execution of the handler.
	void uSigHandlerModule::sigAlrmHandler( __U_SIGPARMS__ ) {
		// This routine handles a SIGALRM/SIGUSR1 signal.  This signal is delivered as the result of time slicing being
		// enabled, a processor is being woken up after being idle for some time, or some intervention being delivered
		// to a thread.  This handler attempts to yield the currently executing thread so that another thread may be
		// scheduled by this processor.

		uDEBUGPRT(
			char buffer[512];
			uDebugPrtBuf( buffer, "sigAlrmHandler, signal:%d, errno:%d, cluster:%p (%s), processor:%p, task:%p (%s), stack:%p, program counter:%p, RFpending:%d, RFinprogress:%d, disableInt:%d, disableIntCnt:%d, disableIntSpin:%d, disableIntSpinCnt:%d\n",
						  sig, errno, &uThisCluster(), uThisCluster().getName(), &uThisProcessor(), &uThisTask(), uThisTask().getName(), signalContextSP( cxt ), signalContextPC( cxt ), uKernelModule::uKernelModuleBoot.RFpending, uKernelModule::uKernelModuleBoot.RFinprogress,
						  uKernelModule::uKernelModuleBoot.disableInt, uKernelModule::uKernelModuleBoot.disableIntCnt, uKernelModule::uKernelModuleBoot.disableIntSpin, uKernelModule::uKernelModuleBoot.disableIntSpinCnt );
		);

		#ifdef __U_STATISTICS__
		if ( sig == SIGUSR1 ) {
			uFetchAdd( UPP::Statistics::signal_usr1, 1 );
		} else if ( sig == SIGALRM ) {
			uFetchAdd( UPP::Statistics::signal_alarm, 1 );
		} else {
			abort( "UNKNOWN ALARM SIGNAL\n" );
		} // if
		#endif // __U_STATISTICS__

		decltype(errno) terrno = errno;					// preserve errno at point of interrupt

	  if ( uKernelModule::uKernelModuleBoot.RFinprogress || // roll forward in progress ?
			 uKernelModule::uKernelModuleBoot.disableInt ||	// inside kernel ?
			 uKernelModule::uKernelModuleBoot.disableIntSpin ) { // spinlock acquired ?
			uDEBUGPRT( uDebugPrtBuf( buffer, "sigAlrmHandler1, signal:%d\n", sig ); );
			errno = terrno;								// reset errno and continue )
			uKernelModule::uKernelModuleBoot.RFpending = true; // indicate roll forward is required
			return;
		} // if

	  // PC executing in non-preemptable routines ?
	  if ( &__start_text_nopreempt <= signalContextPC( cxt ) && signalContextPC( cxt ) <= &__stop_text_nopreempt ) {
	  		errno = terrno;
	  		uKernelModule::uKernelModuleBoot.RFpending = true;
	  		return;
	  	} // if

		// Unsafe to perform these checks if in kernel or performing roll forward, because the thread specific variables
		// used by uThis* routines are changing.
		#if defined( __U_DEBUG__ ) && defined( __U_MULTI__ )
		if ( sig == SIGALRM ) {							// only handle SIGALRM on system cluster
			assert( &uThisProcessor() == &uKernelModule::systemProcessor );
			assert( &uThisCluster() == &uKernelModule::systemCluster );
		} // if
		#endif // __U_DEBUG__ && __U_MULTI__

		#if defined( __U_DEBUG__ )
		uThisCoroutine().verify();						// good place to check for stack overflow
		#endif // __U_DEBUG__

		#if defined( __U_MULTI__ )
		if ( &uThisProcessor() != &uKernelModule::systemProcessor ) {
			uKernelModule::uKernelModuleBoot.RFinprogress = true; // starting roll forward
		} // if
		#endif // __U_MULTI__

		// Clear blocked SIGALRM/SIGUSR1 so more can arrive.
		if ( sizeof( sigset_t ) != sizeof( cxt->uc_sigmask ) ) { // should disappear due to constant folding
			// uc_sigmask is incorrect size
			sigset_t new_mask;
			sigemptyset( &new_mask );
			if ( &uThisProcessor() == &uKernelModule::systemProcessor ) {
				sigaddset( &new_mask, SIGALRM );
			} // if
			sigaddset( &new_mask, SIGUSR1 );
			if ( sigprocmask( SIG_UNBLOCK, &new_mask, nullptr ) == -1 ) {
				abort( "internal error, sigprocmask" );
			} // if
		} else {
			if ( sigprocmask( SIG_SETMASK, (sigset_t *)&(cxt->uc_sigmask), nullptr ) == -1 ) {
				abort( "internal error, sigprocmask" );
			} // if
		} // if

		uDEBUGPRT( uDebugPrtBuf( buffer, "sigAlrmHandler2, signal:%d\n", sig ); );

		#if __U_LOCALDEBUGGER_H__
		// The current PC is stored, so that it can be looked up by the local debugger to check if the task was time
		// sliced at a breakpoint location.
		uThisTask().debugPCandSRR = signalContextPC( cxt );
		#endif // __U_LOCALDEBUGGER_H__

		uKernelModule::rollForward();

		#if __U_LOCALDEBUGGER_H__
		// Reset this field before task is started, to denote that this task is not blocked.
		uThisTask().debugPCandSRR = nullptr;
		#endif // __U_LOCALDEBUGGER_H__

		// Block all signals from arriving so values can be safely reset.
		if ( sigprocmask( SIG_BLOCK, &block_mask, nullptr ) == -1 ) {
			abort( "internal error, sigprocmask" );
		} // if

		uDEBUGPRT(
			uDebugPrtBuf( buffer, "sigAlrmHandler3, signal:%d, errno:%d, cluster:%p (%s), processor:%p, task:%p (%s), stack:%p, program counter:%p, RFpending:%d, RFinprogress:%d, disableInt:%d, disableIntCnt:%d, disableIntSpin:%d, disableIntSpinCnt:%d\n",
						  sig, errno, &uThisCluster(), uThisCluster().getName(), &uThisProcessor(), &uThisTask(), uThisTask().getName(), signalContextSP( cxt ), signalContextPC( cxt ), uKernelModule::uKernelModuleBoot.RFpending, uKernelModule::uKernelModuleBoot.RFinprogress, uKernelModule::uKernelModuleBoot.disableInt, uKernelModule::uKernelModuleBoot.disableIntCnt, uKernelModule::uKernelModuleBoot.disableIntSpin, uKernelModule::uKernelModuleBoot.disableIntSpinCnt );
		);

		errno = terrno;									// reset errno and continue
	} // uSigHandlerModule::sigAlrmHandler


	void sigSegvBusHandler( __U_SIGPARMS__ ) {
		if ( sfp->si_addr == nullptr ) {
			abort( uSigHandlerModule::Yes, "Null pointer (nullptr) dereference." );
		} else if ( sfp->si_addr ==
					#if __U_WORDSIZE__ == 32
					(void *)0xffff'ffff
					#else
					(void *)0xffff'ffff'ffff'ffff
					#endif // __U_WORDSIZE__ == 32
			) {
			abort( uSigHandlerModule::Yes, "Using a scrubbed pointer address %p.\n"
				   "Possible cause is using uninitialized storage or using storage after it has been freed.",
				   sfp->si_addr );
		} else {
			abort( uSigHandlerModule::Yes, "%s at memory location %p.\n"
				   "Possible cause is reading outside the address space or writing to a protected area within the address space with an invalid pointer or subscript.",
				   (sig == SIGSEGV ? "Segment fault" : "Bus error"), sfp->si_addr );
		} // if
	} // sigSegvBusHandler


	void sigIllHandler( __U_SIGPARMS__ ) {
		abort( uSigHandlerModule::Yes, "Executing illegal instruction at location %p.\n"
			   "Possible cause is stack corruption.",
			   sfp->si_addr );
	} // sigIllHandler


	void sigFpeHandler( __U_SIGPARMS__ ) {
		const char * msg;
		switch ( sfp->si_code ) {
		  case FPE_INTDIV:
		  case FPE_FLTDIV: msg = "divide by zero"; break;
		  case FPE_FLTOVF: msg = "overflow"; break;
		  case FPE_FLTUND: msg = "underflow"; break;
		  case FPE_FLTRES: msg = "inexact result"; break;
		  case FPE_FLTINV: msg = "invalid operation"; break;
		  default: msg = "unknown";
		} // switch
		abort( uSigHandlerModule::Yes, "Computation error %s at location %p.\n", msg, sfp->si_addr );
	} // sigFpeHandler


	void sigTermHandler( __U_SIGPARMS__ ) {
		// This routine handles a SIGHUP, SIGINT, or a SIGTERM signal.  The signal is delivered to the root process as
		// the result of some action on the part of the user attempting to terminate the application.  It must be caught
		// here so that all processes in the application may be terminated.

		uDEBUGPRT(
			char buffer[256];
			uDebugPrtBuf( buffer, "sigTermHandler, cluster:%.128s (%p), processor:%p\n",
						  uThisCluster().getName(), &uThisCluster(), &uThisProcessor() );
		);

		abort( uSigHandlerModule::Yes, "Application interrupted by signal: %s.", strsignal( sig ) );
	} // sigTermHandler


	uSigHandlerModule::uSigHandlerModule() {
		// SKULLDUGGERY: In Ubuntu 22.04, someone augmented signal.h to allow SIGSTKSZ to be "sysconf(_SC_SIGSTKSZ)" in
		// sigstksz.h, as well as 8192 in sigstack.h. HOWEVER, they forgot to provide a mechanism to tell signal.h to
		// use sigstack.h rather than sigstksz.h. (I'm not happy.) By undefining _GNU_SOURCE before signal.h and
		// redefining it afterwards, you can get 8192, but then nothing works correctly inside of signal.h without
		// _GNU_SOURCE defined.  So what is needed is a way to get signal.h to use sigstack.h WITH _GNU_SOURCE defined.
		// Basically something is wrong with features.h and its use in signal.h.

		// Now don't even think about dynamically allocating the storage for this stack, because C++ has no orderings of
		// ctor/dtor among translation units, so the dynamic allocation might occur AFTER an error occurs at boot time
		// and attempt to deliver the signal on uninitialized signal stack. So currently, I'm hard-coding 8192 until
		// whoever made this change fixes the problem.
		#undef SIGSTKSZ
		#define SIGSTKSZ 8192

		// As a precaution (and necessity), errors that result in termination are delivered on a separate stack because
		// task stacks might be very small (4K) and the signal delivery corrupts memory to the point that a clean
		// shutdown is impossible. Also, when a stack overflow encounters the non-accessible sentinel page (debug only)
		// and generates a segment fault, the signal cannot be delivered on the sentinel page. Finally, calls to abort
		// print a stack trace that uses substantial stack space.

		#define MINSTKSZ SIGSTKSZ * 8
		static char stack[MINSTKSZ] __attribute__(( aligned (16) ));
		static stack_t ss;
		uDEBUGPRT( uDebugPrt( "uSigHandlerModule, stack:%p, size:%d, %p\n", stack, SIGSTKSZ, stack + MINSTKSZ ); );

		ss.ss_sp = stack;
		ss.ss_size = MINSTKSZ;
		ss.ss_flags = 0;
		if ( sigaltstack( &ss, nullptr ) == -1 ) {
			abort( "uSigHandlerModule::uSigHandlerModule : internal error, sigaltstack error(%d) %s.", errno, strerror( errno ) );
		} // if

		// Associate handlers with the set of signals that this application is interested in.  These handlers are
		// inherited by all unix processes that are subsequently created so they are not installed again.

		// internal errors
		signal( SIGSEGV, sigSegvBusHandler, SA_SIGINFO | SA_ONSTACK ); // Invalid memory reference (default: Core)
		signal( SIGBUS,  sigSegvBusHandler, SA_SIGINFO | SA_ONSTACK ); // Bus error, bad memory access (default: Core)
		signal( SIGILL,  sigIllHandler, SA_SIGINFO | SA_ONSTACK ); // Illegal Instruction (default: Core)
		signal( SIGFPE,  sigFpeHandler, SA_SIGINFO | SA_ONSTACK ); // Floating-point exception (default: Core)

		// handlers from outside errors
		// one shot handler, reset to default in-case there are multiple attempts, e.g., C-c, C-c, C-c ...
		signal( SIGTERM, sigTermHandler, SA_SIGINFO | SA_ONSTACK | SA_RESETHAND ); // Termination signal (default: Term)
		signal( SIGINT,  sigTermHandler, SA_SIGINFO | SA_ONSTACK | SA_RESETHAND ); // Interrupt from keyboard (default: Term)
		signal( SIGHUP,  sigTermHandler, SA_SIGINFO | SA_ONSTACK | SA_RESETHAND ); // Hangup detected on controlling terminal or death of controlling process (default: Term)
		signal( SIGQUIT, sigTermHandler, SA_SIGINFO | SA_ONSTACK | SA_RESETHAND ); // Quit from keyboard (default: Core)
		signal( SIGABRT, sigTermHandler, SA_SIGINFO | SA_ONSTACK | SA_RESETHAND ); // Abort signal from abort(3) (default: Core)

		// Do NOT specify SA_RESTART for SIGALRM because "select" does not wake up when sent a SIGALRM from another UNIX
		// process, which means non-blocking I/O does not work correctly in multiprocessor mode.

		signal( SIGALRM, sigAlrmHandler, SA_SIGINFO );
		signal( SIGUSR1, sigAlrmHandler, SA_SIGINFO );

		sigfillset( &block_mask );						// turn all bits on
	} // uSigHandlerModule::uSigHandlerModule
} // UPP


// Local Variables: //
// compile-command: "make install" //
// End: //
