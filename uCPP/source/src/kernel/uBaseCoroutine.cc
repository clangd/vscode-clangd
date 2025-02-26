//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1997
// 
// uBaseCoroutine.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sat Sep 27 16:46:37 1997
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Sep 10 22:31:34 2024
// Update Count     : 710
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
#ifdef __U_PROFILER__
#include <uProfiler.h>
#endif // __U_PROFILER__
//#include <uDebug.h>


//######################### uBaseCoroutine #########################


namespace __cxxabiv1 {
	// This routine must be called manually since our exception throwing interferes with normal deallocation
	extern "C" void __cxa_free_exception(void *vptr) throw();

	// These two routines allow proper per-coroutine exception handling
	extern "C" __cxa_eh_globals *__cxa_get_globals_fast() throw() {
		return &uThisCoroutine().ehGlobals;
	}

	extern "C" __cxa_eh_globals *__cxa_get_globals() throw() {
		return &uThisCoroutine().ehGlobals;
	}
} // namespace


uBaseCoroutine::UnwindStack::UnwindStack( bool e ) : exec_dtor( e ) {
} // uBaseCoroutine::UnwindStack::UnwindStack

uBaseCoroutine::UnwindStack::~UnwindStack() {
	if ( exec_dtor && ! std::__U_UNCAUGHT_EXCEPTION__() ) { // if executed as part of an exceptional clean-up do nothing, otherwise terminate is called
		__cxxabiv1::__cxa_free_exception( this );		// if handler terminates and 'safety' is off, clean up the memory for old exception
		_Throw UnwindStack( true );						// and throw a new exception to continue unwinding
	} // if
} // uBaseCoroutine::UnwindStack::~UnwindStack


void uBaseCoroutine::unwindStack() {
	if ( ! cancelInProgress_ ) {						// do not cancel if we're already cancelling
		cancelInProgress_ = true;
		// NOTE: This throw fails and terminates the application if it occurs in the middle of a destructor triggered by
		// an exception.  While not a serious restriction now, it could be one when time-slice polling is introduced
		_Throw UnwindStack( true );						// start the cancellation unwinding
	} // if
} // uBaseCoroutine::unwindStack


void uBaseCoroutine::createCoroutine() {
	state_ = Start;
	notHalted_ = true;									// must be a non-zero value so detectable after memory is scrubbed

	// start_ is initialized to last resumer at the start of invokeCoroutine
	last_ = nullptr;									// see ~uCoroutineDestructor
	#ifdef __U_DEBUG__
	currSerialOwner_ = nullptr;							// for error checking
	currSerialCount_ = 0;
	#endif // __U_DEBUG__

	// exception handling / cancellation

	handlerStackTop_ = handlerStackVisualTop_ = nullptr;
	resumedObj_ = nullptr;
	topResumedType_ = nullptr;
	DEStack_ = nullptr;
	unexpectedRtn_ = uEHM::unexpected;					// initialize default unexpected routine
	unexpected_ = false;

	memset( &ehGlobals, 0, sizeof( ehGlobals ) );

	cancelled_ = false;
	cancelInProgress_ = false;
	cancelState_ = CancelEnabled;
	cancelType_ = CancelPoll;							// not used yet, but makes uPthread cancellation easier

	#ifdef __U_PROFILER__
	// profiling

	profileTaskSamplerInstance = nullptr;
	#endif // __U_PROFILER__
} // uBaseCoroutine::createCoroutine


// SKULLDUGGERY: __errno_location may be defined with attribute "const" so its result can be stored and reused. So in
// taskCxtSw, the thread-local address may be stored in a register on the front side of the context switch, but on the
// back side of the context switch the processor may have changed so the stored value is wrong. This routine forces
// another call to __errno_location on the back side of the context switch.

static int *errno_location() __attribute__(( noinline )); // prevent unwrapping
static int *errno_location() {
	asm( "" : : : "memory" );							// prevent call eliding (see GCC function attribute)
	return &errno;
} // errno_location


void uBaseCoroutine::taskCxtSw() {						// switch between a task and the kernel
	uBaseCoroutine &coroutine = uThisCoroutine();		// optimization
	uBaseTask &currTask = uThisTask();

	#ifdef __U_PROFILER__
	if ( currTask.profileActive && uProfiler::uProfiler_builtinRegisterTaskBlock ) { // uninterruptable hooks
		(*uProfiler::uProfiler_builtinRegisterTaskBlock)( uProfiler::profilerInstance, currTask );
	} // if
	#endif // __U_PROFILER__

	coroutine.setState( Inactive );						// set state of current coroutine to inactive

	uDEBUGPRT( uDebugPrt( "(uBaseCoroutine &)%p.taskCxtSw, coroutine:%p, coroutine.SP:%p, coroutine.storage:%p, storage:%p\n",
						  this, &coroutine, coroutine.stackPointer(), coroutine.storage, storage ); )

	decltype(errno) errno_ = errno;						// save
	coroutine.save();									// save user specified contexts

	uSwitch( coroutine.context_, context_ );			// context switch to kernel

	coroutine.restore();								// restore user specified contexts
	*errno_location() = errno_;							// restore

	coroutine.setState( Active );						// set state of new coroutine to active
	currTask.setState( uBaseTask::Running );

	#ifdef __U_PROFILER__
	if ( currTask.profileActive && uProfiler::uProfiler_builtinRegisterTaskUnblock ) { // uninterruptable hooks
		(*uProfiler::uProfiler_builtinRegisterTaskUnblock)( uProfiler::profilerInstance, currTask );
	} // if
	#endif // __U_PROFILER__
} // uBaseCoroutine::taskCxtSw


void uBaseCoroutine::corCxtSw() {						// switch between two coroutine contexts
	uBaseCoroutine &coroutine = uThisCoroutine();		// optimization
	uBaseTask &currTask = uThisTask();

	#ifdef __U_DEBUG__
	// reset task in current coroutine?
	if ( coroutine.currSerialCount_ == currTask.currSerialLevel ) {
		coroutine.currSerialOwner_ = nullptr;
	} // if

	// check and set for new owner
	if ( currSerialOwner_ != &currTask ) {
		if ( currSerialOwner_ != nullptr  ) {
			if ( &currSerialOwner_->getCoroutine() != this ) {
				abort( "Attempt by task %.256s (%p) to activate coroutine %.256s (%p) currently executing in a mutex object owned by task %.256s (%p).\n"
					   "Possible cause is task attempting to logically change ownership of a mutex object via a coroutine.",
					   currTask.getName(), &currTask, this->getName(), this, currSerialOwner_->getName(), currSerialOwner_ );
			} else {
				abort( "Attempt by task %.256s (%p) to resume coroutine %.256s (%p) currently being executed by task %.256s (%p).\n"
					   "Possible cause is two tasks attempting simultaneous execution of the same coroutine.",
					   currTask.getName(), &currTask, this->getName(), this, currSerialOwner_->getName(), currSerialOwner_ );
			} // if
		}  else {
			currSerialOwner_ = &currTask;
			currSerialCount_ = currTask.currSerialLevel;
		} // if
	} // if
	#endif // __U_DEBUG__

	#ifdef __U_PROFILER__
	if ( currTask.profileActive && uProfiler::uProfiler_registerCoroutineBlock ) {
		(*uProfiler::uProfiler_registerCoroutineBlock)( uProfiler::profilerInstance, currTask, *this );
	} // if
	#endif // __U_PROFILER__

	uKernelModule::uKernelModuleData::disableInterrupts();

	coroutine.setState( Inactive );						// set state of current coroutine to inactive

	uDEBUGPRT( uDebugPrt( "(uBaseCoroutine &)%p.corCxtSw, coroutine:%p, coroutine.SP:%p\n",
						  this, &coroutine, coroutine.stackPointer() ); )

	coroutine.save();									// save user specified contexts
	currTask.currCoroutine_ = this;						// set new coroutine that task is executing

	#ifdef __U_STATISTICS__
	uFetchAdd( UPP::Statistics::coroutine_context_switches, 1 );
	#endif // __U_STATISTICS__

	uSwitch( coroutine.context_, context_ );			// context switch to specified coroutine

	coroutine.restore();								// restore user specified contexts
	coroutine.setState( Active );						// set state of new coroutine to active

	uKernelModule::uKernelModuleData::enableInterrupts();

	#ifdef __U_PROFILER__
	if ( uThisTask().profileActive && uProfiler::uProfiler_registerCoroutineUnblock ) {
		(*uProfiler::uProfiler_registerCoroutineUnblock)( uProfiler::profilerInstance, uThisTask() );
	} // if
	#endif // __U_PROFILER__
} // uBaseCoroutine::corCxtSw


void uBaseCoroutine::corFinish() {						// resumes the coroutine that first resumed this coroutine
	#ifdef __U_DEBUG__
	if ( ! starter_->notHalted_ ) {						// check if terminated
			abort( "Attempt by coroutine %.256s (%p) to resume back to terminated starter coroutine %.256s (%p).\n"
					"Possible cause is terminated coroutine's main routine has already returned.",
					uThisCoroutine().getName(), &uThisCoroutine(), starter_->getName(), starter_ );
	} // if
	#endif // __U_DEBUG__

	notHalted_ = false;
	starter_->corCxtSw();
	// CONTROL NEVER REACHES HERE!
	abort( "(uBaseCoroutine &)%p.corFinish() : internal error, attempt to return.", this );
} // uBaseCoroutine::corFinish


const char * uBaseCoroutine::setName( const char name[] ) {
	const char * prev = name;
	name_ = name;

	#ifdef __U_PROFILER__
	if ( uThisTask().profileActive && uProfiler::uProfiler_registerSetName ) { 
		(*uProfiler::uProfiler_registerSetName)( uProfiler::profilerInstance, *this, name ); 
	} // if
	#endif // __U_PROFILER__
	return prev;
} // uBaseCoroutine::setName

const char * uBaseCoroutine::getName() const {
	// storage might be uninitialized or scrubbed
	return name_ == nullptr
		#ifdef __U_DEBUG__
			 || name_ == (const char *)-1				// only scrub in debug
		#endif // __U_DEBUG__
		? "*unknown*" : name_;
} // uBaseCoroutine::getName


void uBaseCoroutine::setCancelState( CancellationState state ) {
	#ifdef __U_DEBUG__
	if ( this != &uThisCoroutine() && state_ != Start ) {
		abort( "Attempt to set the cancellation state of coroutine %.256s (%p) by coroutine %.256s (%p).\n"
				"A coroutine/task may only change its own cancellation state.",
				getName(), this, uThisCoroutine().getName(), &uThisCoroutine() );
	} // if
	#endif // __U_DEBUG__
	cancelState_ = state;
} // uBaseCoroutine::setCancelState

void uBaseCoroutine::setCancelType( CancellationType type ) {
	#ifdef __U_DEBUG__
	if ( this != &uThisCoroutine() && state_ != Start ) {
		abort( "Attempt to set the cancellation state of coroutine %.256s (%p) by coroutine %.256s (%p).\n"
				"A coroutine/task may only change its own cancellation state.",
				getName(), this, uThisCoroutine().getName(), &uThisCoroutine() );
	} // if
	#endif // __U_DEBUG__
	cancelType_ = type;
} // uBaseCoroutine::setCancelType


uBaseCoroutine::Failure::Failure( const char *const msg ) : uKernelFailure( msg ) {
} // uBaseCoroutine::Failure::Failure


uBaseCoroutine::UnhandledException::UnhandledException( uBaseException * cause, const char * const msg ) : Failure( msg ), cause( cause ), multiple( 1 ) {
	cleanup = true;
} // uBaseCoroutine::UnhandledException::UnhandledException


uBaseCoroutine::UnhandledException::UnhandledException( const UnhandledException & ex ) : Failure() {
	memcpy( (void *)this, (void *)&ex, sizeof(*this) );	// relies on all fields having trivial copy constructors
	ex.cleanup = false;									// then prevent the original from deleting the cause
} // uBaseCoroutine::UnhandledException::UnhandledException

uBaseCoroutine::UnhandledException::~UnhandledException() {
	if ( cleanup ) {
		delete cause;									// clean up the stored exception object
	} // if
} // uBaseCoroutine::UnhandledException::~UnhandledException

void uBaseCoroutine::UnhandledException::triggerCause() {
	if ( cause != nullptr ) cause->reraise();
} // uBaseCoroutine::UnhandledException::triggerCause

void uBaseCoroutine::UnhandledException::defaultTerminate() {
	size_t len = cause == nullptr ? 0 : strlen( cause->message() );
	abort( "(uBaseCoroutine &)%p : Unhandled exception in task %.256s raised non-locally through %d unhandled exception(s)\nfrom coroutine/task %.256s (%p) because of %s%s%s.",
		   &uThisCoroutine(), uThisCoroutine().getName(), multiple, sourceName(), &source(), message(), len == 0 ? "" : " indicating ", len == 0 ? "" : cause->message() );
} // uBaseCoroutine::UnhandledException::defaultTerminate

void uBaseCoroutine::forwardUnhandled( UnhandledException & ex ) {
	ex.multiple += 1;
	notHalted_ = false;									// terminate coroutine
	_Resume _At resumer();								// forward original exception at resumer
} // uBaseCoroutine::forwardUnhandled

void uBaseCoroutine::handleUnhandled( uBaseException * ex ) {
	#define uBaseCoroutineSuffixMsg1 "an unhandled thrown exception of type "
	#define uBaseCoroutineSuffixMsg2 "an unhandled resumed exception of type "
	char msg[sizeof(uBaseCoroutineSuffixMsg2) - 1 + uEHMMaxName]; // use larger message
	uBaseException::RaiseKind raisekind = ex == nullptr ? uBaseException::ThrowRaise : ex->getRaiseKind();
	strcpy( msg, raisekind == uBaseException::ThrowRaise ? uBaseCoroutineSuffixMsg1 : uBaseCoroutineSuffixMsg2 );
	uEHM::getCurrentExceptionName( raisekind, msg + strlen( msg ), uEHMMaxName );
	notHalted_ = false;									// terminate coroutine
	_Resume UnhandledException( ex == nullptr ? ex : ex->duplicate(), msg ) _At resumer();
} // uBaseCoroutine::handleUnhandled


uBaseCoroutine::uCoroutineConstructor::uCoroutineConstructor( UPP::uAction f, UPP::uSerial &serial, uBaseCoroutine &coroutine, const char *name ) {
	if ( f == UPP::uYes ) {
		#pragma GCC diagnostic push
		#if __GNUC__ >= 8								// valid GNU compiler diagnostic ?
		#pragma GCC diagnostic ignored "-Wcast-function-type"
		#endif // __GNUC__ >= 8
		coroutine.startHere( reinterpret_cast<void (*)( uMachContext & )>(uMachContext::invokeCoroutine) );
		#pragma GCC diagnostic pop
		coroutine.name_ = name;
		coroutine.serial_ = &serial;					// set cormonitor's serial instance

		#ifdef __U_PROFILER__
		if ( uThisTask().profileActive && uProfiler::uProfiler_registerCoroutine && // profiling & coroutine registered for profiling ?
			 dynamic_cast<uProcessorKernel *>(&coroutine) == nullptr ) { // and not kernel coroutine
			(*uProfiler::uProfiler_registerCoroutine)( uProfiler::profilerInstance, coroutine, serial );
		} // if
		#endif // __U_PROFILER__
	} // if
} // uBaseCoroutine::uCoroutineConstructor::uCoroutineConstructor


uBaseCoroutine::uCoroutineDestructor::uCoroutineDestructor(
	#ifdef __U_PROFILER__
	UPP::uAction f,
	#endif // __U_PROFILER__
	uBaseCoroutine &coroutine )
	#ifdef __U_PROFILER__
		: f( f ), coroutine( coroutine )
	#endif // __U_PROFILER__
{
	// Clean up the stack of a non-terminated coroutine (i.e., run its destructors); a terminated coroutine's stack is
	// already cleaned up. Ignore the uProcessorKernel coroutine because it has a special shutdown sequence.

	// Because code executed during stack unwinding may access any coroutine data, unwinding MUST occur before running
	// the coroutine's destructor. A consequence of this semantics is that the destructor may not resume the coroutine,
	// so it is asymmetric with the coroutine's constructor.

	if ( coroutine.getState() != uBaseCoroutine::Halt	// coroutine not halted
		 && coroutine.last_ != nullptr					// and its main is started
		 && dynamic_cast<UPP::uProcessorKernel *>(&coroutine) == nullptr ) { // but not the processor Kernel
		// Mark for cancellation, then resume the coroutine to trigger a call to uPoll on the backside of its
		// suspend(). uPoll detects the cancellation and calls unwind_stack, which throws exception UnwindStack to
		// unwinding the stack. UnwindStack is ultimately caught inside uMachContext::uInvokeCoroutine.
		coroutine.cancel();
		coroutine.resume();
	} // if
} // uBaseCoroutine::uCoroutineDestructor::uCoroutineDestructor

#ifdef __U_PROFILER__
uBaseCoroutine::uCoroutineDestructor::~uCoroutineDestructor() {
	if ( f == uYes ) {
		if ( uThisTask().profileActive && uProfiler::uProfiler_deregisterCoroutine ) { // profiling this coroutine & coroutine registered for profiling ? 
			(*uProfiler::uProfiler_deregisterCoroutine)( uProfiler::profilerInstance, coroutine );
		} // if
	} // if
} // uBaseCoroutine::uCoroutineDestructor::~uCoroutineDestructor
#endif // __U_PROFILER__

// Local Variables: //
// compile-command: "make install" //
// End: //
