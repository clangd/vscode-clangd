//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Russell Mok 1997
// 
// uEHM.cc -- 
// 
// Author           : Russell Mok
// Created On       : Sun Jun 29 00:15:09 1997
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Sep 10 22:35:38 2024
// Update Count     : 863
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

#pragma GCC diagnostic push
#if __GNUC__ >= 12										// valid GNU compiler diagnostic ?
// For backwards compatibility, keep testing dynamic-exception-specifiers until they are no longer supported.
#pragma GCC diagnostic ignored "-Wdeprecated-declarations" // g++12 deprecates unexpected_handler
#endif // __GNUC__ >= 12

#define __U_KERNEL__
#include <uC++.h>

//#define __U_DEBUG_H__
//#include <uDebug.h>

#include <cxxabi.h>
#include <cstring>										// strlen, strncpy, strcpy


//######################### std::{C++ exception routines} ########################


void uEHM::terminate() {
	char ExName[uEHMMaxName], ResName[uEHMMaxName];

	getCurrentExceptionName( uBaseException::ThrowRaise, ExName, uEHMMaxName );
	getCurrentExceptionName( uBaseException::ResumeRaise, ResName, uEHMMaxName );

	bool exception = ExName[0] != '\0';					// optimization
	bool resumption = ResName[0] != '\0';

	if ( ! exception && ! resumption ) abort( "terminate called without an active exception" );

	uBaseException * curr = getCurrentException();		// optimization
	bool msg = curr != nullptr && curr->msg[0] != '\0';

	abort( "%s%s%s%s%s%s%s%s%s",
		   ( uThisCoroutine().unexpected_ ?
			 "Exception propagated through a function whose exception-specification does not permit exceptions of that type.\n" :
			 "Propagation failed to find a matching handler.\n"
			 "Possible cause is a missing try block with appropriate catch clause for specified or derived exception type,\n"
			 "or throwing an exception from within a destructor while propagating an exception.\n" ),
		   (exception  ? "Type of last active termination: "  : ""), (exception  ? ExName  : ""), (exception && resumption ? "\n" : ""),
		   (resumption ? "Type of last active resumption: " : ""), (resumption ? ResName : ""),
		   (msg ? ", Exception message: " : ""), (msg ? curr->msg : ""), "." ); // abort puts a "\n"
} // uEHM::terminate

void uEHM::terminateHandler() {
	try {
		(*uThisTask().terminateRtn)();
		uEHM::terminate();
	} catch( ... ) {
		uEHM::terminate();
	} // try
} // uEHM::terminateHandler

void uEHM::unexpected() {
	uThisCoroutine().unexpected_ = true;
	std::terminate();
} // uEHM::unexpected

void uEHM::unexpectedHandler() {
	(*uThisCoroutine().unexpectedRtn_)();
	std::terminate();
} // uEHM::unexpectedHandler

void std::terminate() noexcept {
	(*uThisTask().terminateRtn)();
} // std::terminate

std::terminate_handler std::set_terminate( std::terminate_handler func ) throw() {
	uBaseTask & task = uThisTask();						// optimization
	std::terminate_handler prev = task.terminateRtn;
	task.terminateRtn = func;
	return prev;
} // std::set_terminate

// Deprecated C++17
std::unexpected_handler std::set_unexpected( std::unexpected_handler func ) throw() {
	uBaseCoroutine & coroutine = uThisCoroutine();		// optimization
	std::unexpected_handler prev = coroutine.unexpectedRtn_;
	coroutine.unexpectedRtn_ = func;
	return prev;
} // std::set_unexpected


//######################### uEHM::AsyncEMsg ########################


uEHM::AsyncEMsg::AsyncEMsg( const uBaseException & event ) : hidden( false ) {
	asyncException = event.duplicate();
} // uEHM::AsyncEMsg::AsyncEMsg

uEHM::AsyncEMsg::~AsyncEMsg() {
	delete asyncException;
} // uEHM::AsyncEMsg::~AsyncEMsg


//######################### uEHM::AsyncEMsgBuffer ########################


// msg queue for nonlocal exceptions in a coroutine

uEHM::AsyncEMsgBuffer::AsyncEMsgBuffer() {
} // AsyncEMsgBuffer::AsyncEMsgBuffer

uEHM::AsyncEMsgBuffer::~AsyncEMsgBuffer() {
	uCSpinLock dummy( lock );
	for ( AsyncEMsg * tmp = drop(); tmp; tmp = drop() ) {
		delete tmp;
	} // for
} // uEHM::AsyncEMsgBuffer::~AsyncEMsgBuffer

void uEHM::AsyncEMsgBuffer::uAddMsg( AsyncEMsg * msg ) {
	uCSpinLock dummy( lock );
	add( msg );
} // uEHM::AsyncEMsgBuffer::uAddMsg

uEHM::AsyncEMsg * uEHM::AsyncEMsgBuffer::uRmMsg() {
	uCSpinLock dummy( lock );
	return drop();
} // uEHM::AsyncEMsgBuffer::uRmMsg

uEHM::AsyncEMsg * uEHM::AsyncEMsgBuffer::uRmMsg( AsyncEMsg * msg ) {
	// Only lock at the end or the start of the list as these are the only places where interference can occur.
	if ( msg == tail() || msg == head() ) {
		uCSpinLock dummy( lock );
		remove( msg );
	} else {
		remove( msg );
	} // if
	return msg;
} // uEHM::AsyncEMsgBuffer::uRmMsg

uEHM::AsyncEMsg * uEHM::AsyncEMsgBuffer::nextVisible( AsyncEMsg * msg ) {
	// find the next msg node that is visible or null
	do {						// do-while because current node could be visible
		msg = succ( msg );
	} while ( msg && msg->hidden );
	return msg;
} // uEHM::AsyncEMsgBuffer::nextVisible


//######################### uBaseException ########################


void uBaseException::setMsg( const char * const msg ) {
	uEHM::strncpy( uBaseException::msg, msg, uEHMMaxMsg );	// copy message
}; // uBaseException::setMsg

uBaseException::~uBaseException() {}

void uBaseException::setSrc( uBaseCoroutine & coroutine ) {
	src = &coroutine;
	uEHM::strncpy( srcName, coroutine.getName(), uEHMMaxName ); // copy source name
} // uBaseException::setSrc

void uBaseException::reraise() {
	if ( getRaiseKind() == uBaseException::ThrowRaise ) {
		uEHM::Throw( *this, boundObject );
		// CONTROL NEVER REACHES HERE!
		assert( false );
	} // if
	uEHM::Resume( *this );
} // uBaseException::reraise

void uBaseException::defaultTerminate() {
	// do nothing so invoke coroutine/task handles it
} // uBaseException::defaultTerminate

void uBaseException::defaultResume() {
	stackThrow();										// throw exception at raise
	// CONTROL NEVER REACHES HERE!
	assert( false );
} // uBaseException::defaultResume


//######################### uEHM::uResumptionHandlers ########################


uEHM::uResumptionHandlers::uResumptionHandlers( uHandlerBase * const table[], const unsigned int size ) : size( size ), table( table ) {
	uBaseCoroutine & coroutine = uThisCoroutine();		// optimization
	next = coroutine.handlerStackTop_;
	conseqNext = coroutine.handlerStackVisualTop_;
	coroutine.handlerStackTop_ = this;
	coroutine.handlerStackVisualTop_ = this;
} // uEHM::uResumptionHandlers::uResumptionHandlers

uEHM::uResumptionHandlers::~uResumptionHandlers() {
	uBaseCoroutine & coroutine = uThisCoroutine();		// optimization
	coroutine.handlerStackTop_ = next;
	coroutine.handlerStackVisualTop_ = conseqNext;
} // uEHM::uResumptionHandlers::~uResumptionHandlers


//######################### uEHM::uDeliverEStack ########################


uEHM::uDeliverEStack::uDeliverEStack( bool f, const std::type_info ** t, unsigned int msg ) : deliverFlag( f ), table_size( msg ), event_table( t ) {
	// the current node applies to all exceptions when table_size is 0
	uBaseCoroutine & coroutine = uThisCoroutine();		// optimization
	next = coroutine.DEStack_;
	coroutine.DEStack_ = this;
} // uEHM::uDeliverEStack::uDeliverEStack

uEHM::uDeliverEStack::~uDeliverEStack() {
	uThisCoroutine().DEStack_ = next;
} // uEHM::uDeliverEStack::~uDeliverEStack


//######################### uEHM::ResumeWorkHorseInit ########################


// Initialization and finalization when handling a signalled event after finding a handler. This ensures the two
// different resuming handler hierarchies are properly maintained.  As well, it maintains the currently handled
// resumption object for reraise.

class uEHM::ResumeWorkHorseInit {
	uResumptionHandlers * prevVisualTop;
	uBaseException * prevResumption;
  public:
	ResumeWorkHorseInit( uResumptionHandlers * h, uBaseException & newResumption ) {
		uBaseCoroutine & coroutine = uThisCoroutine();	// optimization
		prevResumption = coroutine.resumedObj_;
		coroutine.resumedObj_ = &newResumption;
		coroutine.topResumedType_ = &typeid(newResumption);
		uBaseCoroutine & current = coroutine;
		prevVisualTop = current.handlerStackVisualTop_;
		current.handlerStackVisualTop_ = h;
	} // uEHM::ResumeWorkHorseInit::resumeWorkHorse

	~ResumeWorkHorseInit() {
		uBaseCoroutine & coroutine = uThisCoroutine();	// optimization
		coroutine.resumedObj_ = prevResumption;
		if ( ! std::__U_UNCAUGHT_EXCEPTION__() ) {		// update top, unless it's a forceful unwind
			coroutine.topResumedType_ = prevResumption ? &typeid(prevResumption) : nullptr;
		} // if
		coroutine.handlerStackVisualTop_ = prevVisualTop;
	} // uEHM::ResumeWorkHorseInit::~resumeWorkHorse
}; // uEHM::ResumeWorkHorseInit


//######################### uEHM ########################


#ifdef __U_DEBUG__
static void Check( uBaseCoroutine & target, const char * kind ) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress"
#if __GNUC__ >= 6										// valid GNU compiler diagnostic ?
#pragma GCC diagnostic ignored "-Wnonnull-compare"
#endif // __GNUC__ >= 6
	// SKULLDUGGERY: simplify call with reference parameter then treat as pointer to check for overwritten memory.
	if ( &target == nullptr || &target == (uBaseCoroutine *)-1 ||
		 *((void **)&target) == nullptr || *((void **)&target) == (void *)-1 ) {
#pragma GCC diagnostic pop

		abort( "Attempt by task %.256s (%p) to %s a nonlocal exception at target %p, but the target is invalid or has been deleted",
			   uThisTask().getName(), &uThisTask(), kind, &target );
	} // if
} // Check
#endif // __U_DEBUG__


void uEHM::asyncToss( const uBaseException & event, uBaseCoroutine & target, uBaseException::RaiseKind raiseKind, bool rethrow ) {
#ifdef __U_DEBUG__
	Check( target, raiseKind == uBaseException::ThrowRaise ? "throw" : "resume" );
#endif // __U_DEBUG__

	if ( target.getState() != uBaseCoroutine::Halt ) {
		event.raiseKind = raiseKind;
		AsyncEMsg * temp = new AsyncEMsg( event );
		uBaseCoroutine & coroutine = uThisCoroutine();
		if ( ! rethrow ) {								// reset current raiser ?
			temp->asyncException->boundObject = (void *)&coroutine;
			temp->asyncException->setSrc( coroutine );
		} // if
		target.asyncEBuf.uAddMsg( temp );
	} // if
} // uEHM::asyncToss

void uEHM::asyncReToss( uBaseCoroutine & target, uBaseException::RaiseKind raiseKind ) {
	uBaseException * r;

	if ( raiseKind == uBaseException::ResumeRaise ) {
		r = getCurrentResumption();
		if ( r == nullptr ) {
			r = getCurrentException();
		} // if
	} else {
		r = getCurrentException();
		if ( r == nullptr ) {
			r = getCurrentResumption();
		} // if
	} // if
	
	if ( r == nullptr ) {								// => there is nothing to resume => terminate
		terminateHandler();
	} else {
		asyncToss( *r, target, raiseKind, true );
	} // if
} // uEHM::asyncReToss


void uEHM::Throw( const uBaseException & event, void * const bound ) {
	uDEBUGPRT( uDebugPrt( "uEHM::Throw( uBaseException::event:%p, bound:%p ) from task %.256s (%p)\n",
						  &event, bound, uThisTask().getName(), &uThisTask() ); )
	event.boundObject = bound;
	event.raiseKind = uBaseException::ThrowRaise;
	event.stackThrow();
	// CONTROL NEVER REACHES HERE!
	abort();											// TEMPORARY, spurious warning gcc-4.9.2
} // uEHM::Throw

void uEHM::ReThrow() {
	uBaseException * e = getCurrentException();			// optimization
	if ( ! e ) {
		abort( "Attempt to rethrow but no active exception.\n"
			   "Possible cause is a rethrow not directly or indirectly performed from a catch clause." );
	} // if
	e->stackThrow();
	throw;
} // uEHM::ReThrow


void uEHM::Resume( const uBaseException & event, void * const bound, bool conseq ) {
	uDEBUGPRT( uDebugPrt( "uEHM::Resume( uBaseException::event:%p, bound:%p ) from task %.256s (%p)\n",
						  &event, bound, uThisTask().getName(), &uThisTask() ); )
	event.boundObject = bound;
	event.raiseKind = uBaseException::ResumeRaise;
	resumeWorkHorse( event, conseq );
} // uEHM::Resume

void uEHM::ReResume( bool conseq ) {
	uBaseException * r = getCurrentResumption();		// optimization
	if ( ! r ) {
		abort( "Attempt to reresume but no active exception.\n"
			   "Possible cause is a reresume not directly or indirectly performed from a _CatchResume clause." );
	} // if
	resumeWorkHorse( *r, conseq );
}; // uEHM::ReResume


int uEHM::poll() {										// handle pending nonlocal exceptions
	struct RAIIdelete {									// ensure cleanup even when an exception is thrown
		AsyncEMsg * msg;
		AsyncEMsgBuffer & buf;
		RAIIdelete( AsyncEMsg * msg, AsyncEMsgBuffer & buf ) : msg( msg ), buf( buf ) {}
		~RAIIdelete() {
			buf.uRmMsg( msg );
			delete msg;
		} // RAIIdelete::~RAIIdelete
	}; // RAIIdelete

	uBaseCoroutine & coroutine = uThisCoroutine();		// optimzation
  if ( coroutine.cancelInProgress() ) return 0;			// skip async handling if cancelling
	if ( coroutine.getCancelState() != uBaseCoroutine::CancelDisabled && coroutine.cancelled() ) { // check for cancellation first, but only if it is enabled	
		coroutine.unwindStack();						// not cancelling, so unwind stack
		// CONTROL NEVER REACHES HERE!
		assert( false );
	} // if

	AsyncEMsgBuffer & msgbuf = coroutine.asyncEBuf;		// optimization
	AsyncEMsg * asyncMsg = msgbuf.head();				// find first node in queue

  if ( asyncMsg == nullptr ) return 0;

	if ( asyncMsg->hidden ) {
		asyncMsg = msgbuf.nextVisible( asyncMsg );		// from there, find first visible node
	} // if

	// For each visible node, check if responsible for its delivery. If yes, hide it from any recursive poll call,
	// handle the event, and remove it from the queue through the automatic resource clean-up.

	int handled = 0;									// number of exceptions handled
	while ( asyncMsg != nullptr ) {
		if ( deliverable_exception( asyncMsg->asyncException->getExceptionType() ) ) {
			RAIIdelete dummy( asyncMsg, msgbuf );		// ensure deletion of node even if exception raised

			asyncMsg->hidden = true;					// hide node from recursive children of poll
			uBaseException * asyncException = asyncMsg->asyncException; // optimization

			// Recover memory allocated for async event message. Resuming handler should not destroyed raised exception
			// regardless of whether the exception is normal or nonlocal.

			if ( asyncException->raiseKind == uBaseException::ThrowRaise ) { // throw event ?
				asyncException->stackThrow();
				// CONTROL NEVER REACHES HERE!
				assert( false );
			} // if

			// Note, implicit context switches only call uYieldNoPoll(), hence poll can only be called from
			// resumeWorkHorse again, which is a safe recursion.

			resumeWorkHorse( *asyncException, false );		// handle event: potential recursion and exception
			handled += 1;
			asyncMsg = msgbuf.nextVisible( asyncMsg );	// advance to next visible node

			// NOTE: asyncMsg IS DELETED HERE !!
		} else {
			asyncMsg = msgbuf.nextVisible( asyncMsg );	// advance to next visible node (without deleting)
		} // if
	} // while

	return handled;
} // uEHM::poll


uBaseException * uEHM::getCurrentException() {
	// check if exception type derived from uBaseException (works for basic/POD types)
	const std::type_info * t = __cxxabiv1::__cxa_current_exception_type();
	if ( t && uEHM::match_exception_type( t, &typeid( uBaseException ) ) ) {
		__cxxabiv1::__cxa_eh_globals * globals = __cxxabiv1::__cxa_get_globals();
		if ( globals != nullptr ) {
			__cxxabiv1::__cxa_exception * header = globals->caughtExceptions;
			if ( header != nullptr ) {					// handling an exception and want to turn it into resumption
				return (uBaseException *)((char *)header + sizeof(__cxxabiv1::__cxa_exception));
			} // if
		} // if
	} // if
	return nullptr;
} // uEHM::getCurrentException

uBaseException * uEHM::getCurrentResumption() {
	return uThisCoroutine().resumedObj_;				// optimization
} // uEHM::getCurrentResumption

const std::type_info * uEHM::getTopResumptionType() {
	return uThisCoroutine().topResumedType_;
} // uEHM::getCurrentResumptionType()


char * uEHM::getCurrentExceptionName( uBaseException::RaiseKind raiseKind, char * s1, size_t n ) {
	const std::type_info * t;

	if ( raiseKind == uBaseException::ResumeRaise ) {
		t = uEHM::getTopResumptionType();
	} else {
		t = __cxxabiv1::__cxa_current_exception_type();
	} // if

	if ( t != nullptr ) {
		int status;
		char * s2 = __cxxabiv1::__cxa_demangle( t->name(), nullptr, 0, &status );
		strncpy( s1, s2 ? s2 : "*unknown*", n );		// TEMPORARY: older g++ may generate a null name for elementary types
		free( s2 );
	} else {
		s1[0] = '\0';
	} // if
	return s1;
} // uEHM::getCurrentExceptionName


char * uEHM::strncpy( char * s1, const char * s2, size_t n ) {
	::strncpy( s1, s2, n );
	if ( strlen( s2 ) > n ) {							// name too long ?
		strcpy( &s1[n - 4], "..." );					// add 4 character ...
	} // if
	return s1;
} // uEHM::strncpy


bool uEHM::match_exception_type( const std::type_info * derived_type, const std::type_info * parent_type ) {
	// return true if derived_type event is derived from parent_type event
	void * dummy;
#ifdef __DEBUG__
	if ( ! parent_type ) abort( "internal error, error in setting up guarded region." );
#endif // __DEBUG__

	// Problem: version of g++ and stdc++ must match because of this virtual call to do the handler matching.  If the
	// cxxabi.h used to compile u++ is different from the one libstdc++ is compiled with, the virtual table lookups call
	// the wrong member routine.  This problem does not occur with plain routines called to communicate with the
	// run-time because they are not polymorphic. There is a plain routine inside libstdc++ that encapsulates this
	// particular call, but the routine is declared static, and hence is unaccessible.
	return parent_type->__do_catch( derived_type, &dummy, 0 );
} // uEHM::match_exception_type


bool uEHM::deliverable_exception( const std::type_info * event_type ) {
	for ( uDeliverEStack * tmp = uThisCoroutine().DEStack_; tmp; tmp = tmp->next ) {
		if ( tmp->table_size == 0 ) {					// table_size == 0 is a short hand for all exceptions
			return tmp->deliverFlag;
		} // if

		for ( int i = 0; i < tmp->table_size; i += 1 ) {
			if ( match_exception_type( event_type, tmp->event_table[i] ) ) {
				return tmp->deliverFlag;
			} // if
		} // for
	} // for
	return false;
} // uEHM::deliverable_exception


// Find and execute resumption handlers. If conseq == false, it means a non-consequential resumption (i.e., one newly
// raised outside of handling code) is being handled; hence, begin looking for handlers at the real top of the stack. If
// conseq == true, it means a consequential resumption is being handled, but in order to avoid recursive handling,
// should not consider earlier resumption handlers. In this case, the virtual top (handlerStackVisualTop) of the handler
// stack is used.

void uEHM::resumeWorkHorse( const uBaseException & event, bool conseq ) {
	uDEBUGPRT( uDebugPrt( "uEHM::resumeWorkHorse( ex:%p, conseq:%d ) from task %.256s (%p)\n",
						  &event, conseq, uThisTask().getName(), &uThisTask() ); )

		const std::type_info * raisedtype = event.getExceptionType();
	uResumptionHandlers * tmp = (conseq) ? uThisCoroutine().handlerStackVisualTop_ : uThisCoroutine().handlerStackTop_;

	while ( tmp ) {
		uDEBUGPRT( uDebugPrt( "uEHM::resumeWorkHorse tmp:%p, raisedtype:%p\n", tmp, raisedtype ); );

		uResumptionHandlers * next = (conseq) ? tmp->conseqNext : tmp->next;

		// search all resumption handlers in the same handler clause
		for ( unsigned int i = 0; i < tmp->size; i += 1 ) {
			uHandlerBase * elem = tmp->table[i];		// optimization
			const void * bound = event.boundObject;
			uDEBUGPRT( uDebugPrt( "uEHM::resumeWorkHorse table[%d]:%p, originalThrower:%p, ExceptionType:%p, bound:%p\n",
								  i, elem, elem->getMatchBinding(), elem->getExceptionType(), bound ); )
				// if (no binding OR binding match) AND (type match OR resume_any) => handler found
				if ( ( elem->getMatchBinding() == nullptr || bound == elem->getMatchBinding() ) &&
					 ( elem->getExceptionType() == nullptr || match_exception_type( raisedtype, elem->getExceptionType() ) ) ) {
					uDEBUGPRT( uDebugPrt( "uEHM::resumeWorkHorse match\n" ); )
						ResumeWorkHorseInit newStackVisualTop( next, (uBaseException &)event );
					elem->uHandler( (uBaseException &)event );
					return;								// return after handling the exception
				} // if
		} // for
		tmp = next;										// next resumption handler clause
	} // while

	// cannot find a handler, use the default handler
	ResumeWorkHorseInit newStackVisualTop( nullptr, (uBaseException &)event ); // record the current resumption
	// default routine for event may change the event, so remove const used to allow both const and non-const events
	const_cast<uBaseException &>(event).defaultResume(); // default handler can change the exception
} // uEHM::resumeWorkHorse

#pragma GCC diagnostic pop

// Local Variables: //
// compile-command: "make install" //
// End: //
