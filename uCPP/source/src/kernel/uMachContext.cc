//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uMachContext.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Fri Feb 25 15:46:42 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Nov 28 11:05:21 2024
// Update Count     : 975
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
#include <uAlign.h>
#ifdef __U_PROFILER__
#include <uProfiler.h>
#endif // __U_PROFILER__
#include <uHeapLmmm.h>

#include <uDebug.h>										// access: uDebugWrite
#undef __U_DEBUG_H__									// turn off debug prints

#include <cerrno>
#include <unistd.h>										// write


extern "C" void uInvokeStub( UPP::uMachContext * );


using namespace UPP;


extern "C" void pthread_deletespecific_( void * );		// see pthread simulation


enum { MinStackSize = 1000 };							// minimum feasible stack size in bytes


namespace UPP {
	void uMachContext::invokeCoroutine( uBaseCoroutine & This ) { // magically invoke the "main" of the most derived class
		// Called from the kernel when starting a coroutine or task so must switch back to user mode.

		This.setState( uBaseCoroutine::Active );		// set state of next coroutine to active
		uKernelModule::uKernelModuleData::enableInterrupts();

		#ifdef __U_PROFILER__
		if ( uThisTask().profileActive && uProfiler::uProfiler_postallocateMetricMemory ) {
			(*uProfiler::uProfiler_postallocateMetricMemory)( uProfiler::profilerInstance, uThisTask() );
		} // if
		// also appears in uBaseCoroutine::corCxtSw
		if ( ! TLS_GET( disableInt ) && uThisTask().profileActive && uProfiler::uProfiler_registerCoroutineUnblock ) {
			(*uProfiler::uProfiler_registerCoroutineUnblock)( uProfiler::profilerInstance, uThisTask() );
		} // if
		#endif // __U_PROFILER__

		// At this point, execution is on the stack of the new coroutine or task that has just been switched to by the
		// kernel.  Therefore, interrupts can legitimately occur now.

		try {
			try {
				This.corStarter();						// remember starter coroutine
				uEHM::poll();							// cancellation checkpoint
				This.main();							// start coroutine's "main" routine
			} catch( uBaseCoroutine::UnwindStack & ex ) { // cancellation to force stack unwind
				ex.exec_dtor = false;					// defuse the unwinder
				This.notHalted_ = false;				// terminate coroutine
			} catch( uBaseCoroutine::UnhandledException & ex ) {
				This.forwardUnhandled( ex );			// continue forwarding
			} catch( uBaseException & ex ) {			// uC++ raise ?
				ex.defaultTerminate();					// default defaultTerminate return here for forwarding
				This.handleUnhandled( &ex );			// start forwarding
			} catch( ... ) {							// C++ exception ?
				This.handleUnhandled();
			} // try
		} catch (...) {
			uEHM::terminate();							// if defaultTerminate or std::terminate throws exception
		} // try

		// check outside handler so exception is freed before suspending
		if ( ! This.notHalted_ ) {						// exceptional ending ?
			This.suspend();								// restart last resumer, which should immediately propagate nonlocal exception
		} // if
		if ( &This != activeProcessorKernel ) {			// uProcessorKernel exit ?
			This.corFinish();
			abort( "internal error, uMachContext::invokeCoroutine, no return" );
		} // if
	} // uMachContext::invokeCoroutine


	void uMachContext::invokeTask( uBaseTask & This ) {	// magically invoke the "main" of the most derived class
		// Called from the kernel when starting a coroutine or task so must switch back to user mode.

		#if defined(__U_MULTI__)
		assert( TLS_GET( activeTask ) == &This );
		#endif // __U_MULTI__

		errno = 0;										// reset errno for each task
		This.currCoroutine_->setState( uBaseCoroutine::Active ); // set state of next coroutine to active
		This.setState( uBaseTask::Running );

		// At this point, execution is on the stack of the new coroutine or task that has just been switched to by the
		// kernel.  Therefore, interrupts can legitimately occur now.
		uKernelModule::uKernelModuleData::enableInterrupts();

//		uHeapControl::startTask();

		#ifdef __U_PROFILER__
		if ( uThisTask().profileActive && uProfiler::uProfiler_postallocateMetricMemory ) {
			(*uProfiler::uProfiler_postallocateMetricMemory)( uProfiler::profilerInstance, uThisTask() );
		} // if
		#endif // __U_PROFILER__

		try {
			try {
				uEHM::poll();							// cancellation checkpoint
				This.main();							// start task's "main" routine
			} catch( uBaseCoroutine::UnwindStack & ex ) {
				ex.exec_dtor = false;					// defuse the unwinder
			} catch( uBaseCoroutine::UnhandledException & ex ) {
				This.forwardUnhandled( ex );			// continue forwarding
			} catch( uBaseException & ex ) {			// uC++ raise ?
				ex.defaultTerminate();					// default defaultTerminate return here for forwarding
				This.handleUnhandled( &ex );			// start forwarding
			} catch( ... ) {							// C++ throw ?
				// uMain::main, generated by the translator, handles:
				//   catch( uBaseCoroutine::UnhandledException & ex ) { ex.defaultTerminate(); }
				//   catch( ... ) { if ( ! cancelInProgress() ) std::terminate(); }
				uPthreadable *pthreadable = dynamic_cast<uPthreadable *>( &This );
				if ( pthreadable ) {					// pthread thread ?
					pthreadable->stop_unwinding = true;	// prevent continuation of unwinding
				} else {
					This.handleUnhandled();				// start forwarding
				} // if
			} // try
		} catch (...) {
			uEHM::terminate();							// if defaultTerminate or std::terminate throws exception
		} // try

		// NOTE: this code needs further protection as soon as asynchronous cancellation is supported

		// Clean up storage associated with the task for pthread thread-specific data, e.g., exception handling
		// associates thread-specific data with any task.

		if ( This.pthreadData != nullptr ) {
			pthread_deletespecific_( This.pthreadData );
		} // if

//		uHeapControl::finishTask();

		This.notHalted_ = false;
		This.setState( uBaseTask::Terminate );

		This.getSerial().leave2();
		// CONTROL NEVER REACHES HERE!
		abort( "(uMachContext &)%p.invokeTask() : internal error, attempt to return.", &This );
	} // uMachContext::invokeTask


	void uMachContext::cleanup( uBaseTask &This ) {
		try {
			std::terminate();							// call task terminate routine
		} catch( ... ) {								// control should not return
		} // try

		if ( This.pthreadData != nullptr ) {			// see above for explanation
			pthread_deletespecific_( This.pthreadData );
		} // if

		uEHM::terminate();								// call abort terminate routine
	} // uMachContext::cleanup


	void uMachContext::extraSave() {
		uContext *context;
		for ( uSeqIter<uContext> iter(additionalContexts_); iter >> context; ) {
			context->save();
		} // for
	} // uMachContext::extraSave


	void uMachContext::extraRestore() {
		uContext *context;
		for ( uSeqIter<uContext> iter(additionalContexts_); iter >> context; ) {
			context->restore();
		} // for
	} // uMachContext::extraRestore


	void uMachContext::startHere( void (*uInvoke)( uMachContext & ) ) {
		#if defined( __i386__ )
		// https://software.intel.com/sites/default/files/article/402129/mpx-linux64-abi.pdf
		// The control bits of the MXCSR register are callee-saved (preserved across calls), while the status bits are
		// caller-saved (not preserved). The x87 status word register is caller-saved, whereas the x87 control word is
		// callee-saved.

		// Bits 16 through 31 of the MXCSR register are reserved and are cleared on a power-up or reset of the processor;
		// attempting to write a non-zero value to these bits, using either the FXRSTOR or LDMXCSR instructions, will result in
		// a general-protection exception (#GP) being generated.
		struct FakeStack {
			void *fixedRegisters[3];					// fixed registers ebx, edi, esi (popped on 1st uSwitch, values unimportant)
			uint32_t mxcsr;								// MXCSR control and status register
			uint16_t fpucsr;							// x87 FPU control and status register
			void *rturn;								// where to go on return from uSwitch
			void *dummyReturn;							// fake return compiler would have pushed on call to uInvoke
			void *argument;								// for 16-byte ABI, 16-byte alignment starts here
			void *padding[3];							// padding to force 16-byte alignment, as "base" is 16-byte aligned
		}; // FakeStack

		((uContext_t *)context_)->SP = (char *)base_ - sizeof( FakeStack );
		((uContext_t *)context_)->FP = nullptr;			// terminate stack with null fp

		((FakeStack *)(((uContext_t *)context_)->SP))->dummyReturn = nullptr;
		((FakeStack *)(((uContext_t *)context_)->SP))->argument = this; // argument to uInvoke
		((FakeStack *)(((uContext_t *)context_)->SP))->rturn = rtnAdr( (void (*)())uInvoke );
		((FakeStack *)(((uContext_t *)context_)->SP))->fpucsr = fncw;
		((FakeStack *)(((uContext_t *)context_)->SP))->mxcsr = mxcsr; // see note above

		#elif defined( __x86_64__ )

		struct FakeStack {
			void *fixedRegisters[5];					// fixed registers rbx, r12, r13, r14, r15
			uint32_t mxcsr;								// MXCSR control and status register
			uint16_t fpucsr;							// x87 FPU control and status register
			void *rturn;								// where to go on return from uSwitch
			void *dummyReturn;							// null return address to provide proper alignment
		}; // FakeStack

		((uContext_t *)context_)->SP = (char *)base_ - sizeof( FakeStack );
		((uContext_t *)context_)->FP = nullptr;			// terminate stack with null fp
		
		((FakeStack *)(((uContext_t *)context_)->SP))->dummyReturn = nullptr;
		((FakeStack *)(((uContext_t *)context_)->SP))->rturn = rtnAdr( (void (*)())uInvokeStub );
		((FakeStack *)(((uContext_t *)context_)->SP))->fixedRegisters[0] = this;
		((FakeStack *)(((uContext_t *)context_)->SP))->fixedRegisters[1] = rtnAdr( (void (*)())uInvoke );
		((FakeStack *)(((uContext_t *)context_)->SP))->fpucsr = fncw;
		((FakeStack *)(((uContext_t *)context_)->SP))->mxcsr = mxcsr; // see note above

		#elif defined( __arm_64__ )

		struct FakeStack {
			void * intRegs[12];							// x19-x30 integer registers
			double fpRegs[8];							// v8-v15 floating point
		};

		((uContext_t *)context_)->SP = (char *)base_ - sizeof( struct FakeStack );
		((uContext_t *)context_)->FP = nullptr;

		FakeStack * fs = (FakeStack *)(((uContext_t *)context_)->SP);

		fs->intRegs[0] = this;							// argument to invoke x19 => x0
		fs->intRegs[2] = rtnAdr( (void (*)())uInvoke );
		fs->intRegs[11] = rtnAdr( (void (*)())uInvokeStub ); // link register x30 => ret moves to pc

		#else
			#error uC++ : internal error, unsupported architecture
		#endif
	} // uMachContext::startHere


	/**************************************************************
		s  |  ,-----------------. \
		t  |  |                 | |
		a  |  |    uContext_t   | } (multiple of 8)
		c  |  |                 | |
		k  |  `-----------------' / <--- context (16 byte align)
		   |  ,-----------------. \ <--- base (stack grows down)
		g  |  |                 | |
		r  |  |    task stack   | } size (multiple of 16)
		o  |  |                 | |
		w  |  `-----------------' / <--- limit (16 byte align)
		t  |  0/8                   <--- storage
		h  V  ,-----------------.
			  |   guard page    |   debug only
			  | write protected |
			  `-----------------'   <--- storage, 4/8/16K page alignment
	**************************************************************/

	void uMachContext::createContext( unsigned int storageSize ) { // used by all constructors
		size_t cxtSize = uCeiling( sizeof(uContext_t), 8 ); // minimum alignment
		size_t size;

		if ( storage_ == nullptr ) {
			size = uCeiling( storageSize, 16 );
			// use malloc/memalign because "new" raises an exception for out-of-memory
			#ifdef __U_DEBUG__
			storage_ = memalign( pageSize, cxtSize + size + pageSize );
			if ( ::mprotect( storage_, pageSize, PROT_NONE ) == -1 ) {
				abort( "(uMachContext &)%p.createContext() : internal error, mprotect failure, error(%d) %s.", this, errno, strerror( errno ) );
			} // if
			#else
			storage_ = malloc( cxtSize + size );		// assume malloc has 16 byte alignment
			#endif // __U_DEBUG__
			if ( storage_ == nullptr ) {
				abort( "Attempt to allocate %zd bytes of storage for coroutine or task execution-state but insufficient memory available.", size );
			} // if
			#ifdef __U_DEBUG__
			limit_ = (char *)storage_ + pageSize;
			#else
			limit_ = (char *)uCeiling( (unsigned long)storage_, 16 ); // minimum alignment
			#endif // __U_DEBUG__
		} else {
			#ifdef __U_DEBUG__
			if ( ((size_t)storage_ & (uAlign() - 1)) != 0 ) { // multiple of uAlign ?
				abort( "Stack storage %p for task/coroutine must be aligned on %d byte boundary.", storage_, (int)uAlign() );
			} // if
			#endif // __U_DEBUG__
			size = storageSize - cxtSize;
			if ( size % 16 != 0 ) size -= 8;
			limit_ = (void *)uCeiling( (unsigned long)storage_, 16 ); // minimum alignment
			storage_ = (void *)((uintptr_t)storage_ | 1); // add user stack storage mark
		} // if
		#ifdef __U_DEBUG__
		if ( size < MinStackSize ) {					// below minimum stack size ?
			abort( "Stack size %zd provides less than minimum of %d bytes for a stack.", size, MinStackSize );
		} // if
		#endif // __U_DEBUG__

		base_ = (char *)limit_ + size;
		context_ = base_;

		extras_.allExtras = 0;

//		uDebugPrt( "(uMachContext &)%p.createContext( %u ), storage:%p, limit:%p, base:%p, context:%p, size:0x%zd\n",			
//				   this, storageSize, storage, limit, base, context, size );
	} // uMachContext::createContext


	void * uMachContext::stackPointer() const {
		if ( &uThisCoroutine() == this ) {				// accessing myself ?
			void *sp;									// use my current stack value
			#if defined( __i386__ )
			asm( "movl %%esp,%0" : "=m" (sp) : );
			#elif defined( __x86_64__ )
			asm( "movq %%rsp,%0" : "=m" (sp) : );
			#elif defined( __arm_64__ )
			asm( "mov x9, sp; str x9,%0" : "=m" (sp) : : "x9" );
			#else
				#error uC++ : internal error, unsupported architecture
			#endif
			return sp;
		} else {										// accessing another coroutine
			return ((uContext_t *)context_)->SP;
		} // if
	} // uMachContext::stackPointer


	ptrdiff_t uMachContext::stackFree() const {
		return (char *)stackPointer() - (char *)limit_;
	} // uMachContext::stackFree


	ptrdiff_t uMachContext::stackUsed() const {
		return (char *)base_ - (char *)stackPointer();
	} // uMachContext::stackUsed


	void uMachContext::verify() {
		// Ignore boot task as it uses the UNIX stack.
		if ( storage_ == ((uBaseTask *)uKernelModule::bootTask)->storage_ ) return;

		void * sp = stackPointer();						// optimization

		if ( sp < limit_ ) {
			abort( "Stack overflow detected: stack pointer %p below limit %p.\n"
					"Possible cause is allocation of large stack frame(s) and/or deep call stack.",
					sp, limit_ );
			#define MINSTACKSIZE 1
		} else if ( stackFree() < MINSTACKSIZE * 1024 ) {
			// Do not use fprintf because it uses a lot of stack space.
			#define xstr(s) str(s)
			#define str(s) #s
			#define MINSTACKSIZEWARNING "uC++ Runtime warning : within " xstr(MINSTACKSIZE) "K of stack limit.\n"
			uDebugWrite( STDERR_FILENO, MINSTACKSIZEWARNING, sizeof(MINSTACKSIZEWARNING) - 1 );
		} else if ( sp > base_ ) {
			abort( "Stack underflow detected: stack pointer %p above base %p.\n"
					"Possible cause is corrupted stack frame via overwriting memory.",
					sp, base_ );
		} // if
	} // uMachContext::verify


	void *uMachContext::rtnAdr( void (*rtn)() ) {
		return (void *)rtn;
	} // uMachContext::rtnAdr
} // UPP


// Local Variables: //
// compile-command: "make install" //
// End: //
