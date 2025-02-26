//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Thierry Delisle 2016
//
// uActor.h --
//
// Author           : Peter A. Buhr and Thierry Delisle
// Created On       : Mon Nov 14 22:40:35 2016
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Nov 28 11:07:17 2024
// Update Count     : 1243
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


#pragma once


#include <uDefaultExecutor.h>
#include <uFuture.h>									// uExecutor
#include <uSemaphore.h>
#include <uQueue.h>
#include <uDebug.h>

#include <functional>


//############################## uActor ##############################


#if __GNUC__ >= 7										// C++17
template<typename T>
auto uReference( T & t ) {
	if constexpr( std::is_pointer<T>::value ) return t;
	else return &t;
} // uReference
#else													// C++11
template<typename T, typename std::enable_if<std::is_pointer<T>::value, int>::type = 0>
T & uReference( T & t ) { return t; }

template<typename T, typename std::enable_if<!std::is_pointer<T>::value, int>::type = 0>
T * uReference( T & t ) { return &t; }
#endif

#ifndef Case
#define Case( type, msg ) if ( type * msg##_d __attribute__(( unused )) = dynamic_cast< type * >( uReference( msg ) ) )
#else
#error actor must provided a "Case" macro and macro name "Case" is already in use.
#endif // ! Case

// http://doc.akka.io/docs/akka/current/java/untyped-actors.html


// Does not support parentage: all actors exist in flatland.

class uActor {
	template<typename T> friend class uActorType;		// access receive member
	template<typename T> friend _Coroutine uCorActorType;

	static uExecutor * executor_;						// executor for all actors
	static bool executorp;								// executor passed to start member
	static uSemaphore wait_;							// wait for all actors to delete
	static size_t actors_;								// number of actor objects in system
  public:
	enum Allocation { Nodelete, Delete, Destroy, Finished }; // allocation status
  protected:
	size_t ticket;										// executor-queue handle to provide FIFO message execution
	Allocation allocation_ = Nodelete;					// allocation action
	Allocation promise_allocation_ = Nodelete;			// allocation action from fast path callback
	uDEBUG( size_t pendingCallbacks = 0; )
  public:
	class Message {
	  public:
		Allocation allocation;							// allocation action
	  private:
		friend class uActor;
	  public:
		Message( Allocation allocation = Nodelete ) : allocation( allocation ) {}
		virtual ~Message() {}
	}; // Message

	class SenderMsg : public Message {
		friend class uActor;
		uActor * sender_;								// delegated sender
	  public:
		SenderMsg( Allocation allocation = Nodelete, uActor * sender = nullptr ) : Message( allocation ), sender_( sender ) {}
		SenderMsg( uActor * sender ) : Message( Delete ), sender_( sender ) {}
		virtual ~SenderMsg() {}
		uActor * sender() { return sender_; }			// sender actor
	}; // SenderMsg

	class TraceMsg : public SenderMsg {
		friend class uActor;
		struct Hop : public uColable {					// intrusive node
			uActor * actor;
			Hop( uActor * actor ) : actor( actor ) {}
		}; // Hop

		// Trace always has an actor hop, except for trace message sent from program main, which is not an actor.
		uQueue<Hop> hops;								// intrusive singlely-linked queue, head and tail pointers
		Hop * cursor = nullptr;							// current location of returns along message trace

		void pushTrace( uActor * receiver ) {
			if ( ! cursor ) {							// singleton pattern for program main
				uActor * sender = uActor::sender();		// actor receiving by executor thread
				if ( sender ) hops.addHead( new Hop( sender ) ); // program main not an actor => not part of trace
			} // if
			cursor = new Hop( receiver );
			hops.addHead( cursor );						// program main not an actor => not part of trace
		} // TraceMsg::pushTrace
	  public:
		TraceMsg( Allocation allocation = Nodelete, uActor * sender = nullptr ) : SenderMsg( allocation, sender ) {}
		TraceMsg( uActor * sender ) : SenderMsg( sender ) {}
		~TraceMsg() { if ( ! hops.empty() ) { erase(); delete hops.dropHead(); } }

		void erase() {									// delete all message hops except cursor hop
			while ( ! hops.empty() ) {
				if ( cursor == hops.head() ) hops.dropHead(); // do not delete cursor node
				else delete hops.dropHead();
			} // while
			hops.addHead( cursor );						// add cursor node back
		} // TraceMsg::erase

		void reset() {									// delete all message hops from head to cursor
			while ( hops.head() != cursor ) delete hops.dropHead();
		} // TraceMsg::reset

		bool Return() {									// keyword "return" conflict
			Hop * temp = (Hop *)cursor->getnext();		// start point
			if ( temp ) {
				cursor = temp;							// change before send
				*temp->actor | *(Message *)this;		// ignore hop via cast
				return true;
			} // if
			return false;
		} // TraceMsg::Return

		void retSender() {
			cursor = hops.tail();						// start point
			*cursor->actor | *(Message *)this;			// ignore hop via cast
		} // TraceMsg::retSender

		bool returned() { return hops.head() != cursor; } // messaged been returned ?

		void resume() {
			cursor = hops.head();						// reset cursor back to head
			*cursor->actor | *(Message *)this;			// ignore hop via cast
		} // TraceMsg::resume

		void print();									// print message trace, useful for debugging
	}; // TraceMsg

	// Promise is responsible for storage management by using reference counts. Can be copied.
	template<typename Result> class Promise {
		class Impl {
			enum Status { EMPTY, CHAINED, FULFILLED };
			std::function< Allocation ( Result ) > callback; // functor type
			uActor * maybeActor;
			volatile Status lock = EMPTY;
			volatile size_t refCnt = 1;					// number of references to promise
			Result result_;								// promise result
		  public:
			void incRef() {
				uFetchAdd( refCnt, 1, __ATOMIC_RELAXED );
			} // Impl::incRef

			bool decRef() {
				if ( uFetchAdd( refCnt, -1, __ATOMIC_RELAXED ) != 1 ) return false; // old value 1 => new value 0
				return true;
			} // Impl::decRef

			template< typename Func > bool maybe( Func callback_ ) { // check result
				if ( lock == FULFILLED ) return true;
				Status comp = EMPTY;
				// Race on assignment, but only one thread sets the callback. Multiple assignments from the same or
				// different threads is detected by the lock.
				callback = callback_;
				maybeActor = sender();
				if ( uCompareAssignValue( lock, comp, CHAINED ) ) { // empty ?
					uDEBUG( if ( maybeActor ) maybeActor->pendingCallbacks += 1; );
					return false;
				} // if
				if ( comp == CHAINED ) abort( "Duplicate callback for promise." ); // _Throw DupCallback(); // duplicate chaining
				assert( comp == FULFILLED && lock == FULFILLED );
				return true;
			} // Impl::maybe

			template< typename Func > bool then( Func callback_ ) { // access result
				if ( maybe( callback_ ) ) {
					// if callback recipient is not program main set allocation
					if ( maybeActor ) maybeActor->promise_allocation( callback_( result_ ) );
					else callback_( result_ );
					return true;
				} // if
				return false;
			} // Impl::then

			void delivery( Result res ) {				// make result available in promise
				// Race on assignment, but only one thread sets the callback. Multiple assignments from the same or
				// different threads is detected by the lock.
				result_ = res;							// store result
				Status prev;
				if ( lock != FULFILLED ) {
					prev = uFetchAssign( lock, FULFILLED ); // mark delivered
					if ( prev == FULFILLED ) abort( "Duplicate delivery to promise; must reset promise." ); //_Throw DupDelivery();
				} else prev = FULFILLED;

				// if maybeActor is null then the program main called maybe so use a default ticket of 0
				if ( prev == CHAINED ) {
					executor_->send(
						Deliver_Callback_( maybeActor, [=, result = result_, call = callback]() { return call( result ); } ),
						maybeActor ? maybeActor->ticket : 0 );
				} // if
			} // Impl::delivery

			Result result() {
				if ( lock != FULFILLED ) abort( "No result %d.", lock );	// _Throw NoResult(); // no result
				return result_;
			} // Impl::access

			void reset() {								// mark promise as empty (for reuse)
				if ( lock == CHAINED ) abort( "Duplicate delivery to promise; must reset promise." ); // _Throw ChainedReset();
				lock = EMPTY;
			} // Impl::reset
		}; // Impl

		Impl * impl;									// storage for implementation
	  public:
		_Exception PromiseFailure {};
		_Exception DupCallback : public PromiseFailure {};	// raised if duplicate callback
		_Exception DupDelivery : public PromiseFailure {};	// raised if duplicate delivery
		_Exception NoResult : public PromiseFailure {};		// raised if no result
		_Exception ChainedReset : public PromiseFailure {};	// raised if pending chained callback

		Promise() : impl( new Impl() ) {}

		~Promise() {
			if ( impl && impl->decRef() ) delete impl;
		} // Promise::~Promise

		Promise( const Promise<Result> & rhs ) {
			impl = rhs.impl;							// point at new impl
			impl->incRef();								//   and increment reference count
		} // Promise::Promise

		Promise<Result> & operator=( const Promise<Result> & rhs ) {
			if ( rhs.impl == impl ) return *this;
			if ( impl->decRef() ) delete impl;			// no references => delete current impl
			impl = rhs.impl;							// point at new impl
			impl->incRef();								//   and increment reference count
			return *this;
		} // Promise::operator=

		Promise( Promise<Result> && rhs ) {
			impl = rhs.impl;							// point at new impl
			rhs.impl = nullptr;
		} // Promise::Promise

		Promise<Result> & operator=( Promise<Result> && rhs ) {
			if ( rhs.impl == impl ) return *this;
			if ( impl->decRef() ) delete impl;			// no references => delete current impl
			impl = rhs.impl;							// point at new impl
			rhs.impl = nullptr;
			return *this;
		} // Promise::operator=

		// USED BY CLIENT

		bool maybe( std::function<Allocation ( Result )> callback_ ) { // access result
			return impl->maybe( callback_ );
		} // Promise::maybe

		bool then( std::function<Allocation ( Result )> callback_ ) { // access result
			return impl->then( callback_ );
		} // Promise::then

		Result result() { return impl->result(); }
		Result operator()() { return result(); }		// alternate syntax for result

		void reset() { impl->reset(); }					// mark promise as empty (for reuse)

		// USED BY SERVER

		void delivery( Result result ) { impl->delivery( result ); } // make result available in the promise
	}; // Promise

	template< typename Result > class PromiseMsg : public SenderMsg {
		friend class uActor;
		Promise< Result > result_;						// delivered promise (should be private to ask)
	  public:
		PromiseMsg( const PromiseMsg & ) = delete;
		PromiseMsg( PromiseMsg && ) = delete;
		PromiseMsg & operator=( const PromiseMsg & ) = delete;
		PromiseMsg & operator=( const PromiseMsg && ) = delete;

		PromiseMsg( Allocation allocation = Nodelete, uActor * sender = nullptr ) : SenderMsg( allocation, sender ) {}
		PromiseMsg( uActor * sender ) : SenderMsg( Delete, sender ) {}
		void delivery( Result res ) { result_.delivery( res ); } // make result available in promise
		Result result() { return result_.result(); }
		Result operator()() { return result(); }		// alternate syntax for result
	}; // PromiseMsg
  private:
	static inline void checkMsg( Message & msg ) {
		switch ( msg.allocation ) {						// analyze message status
		  case Nodelete: break;
		  case Delete: delete &msg; break;
		  case Destroy: msg.~Message(); break;
		  case Finished: break;
		} // switch
	} // uActor::checkMsg

	static inline void checkActor( uActor & actor ) {
		if ( actor.allocation_ != Nodelete ) {
			uDEBUG( if ( actor.pendingCallbacks > 0 ) abort( "Destroying/deleting/finishing an actor with pending callbacks." ); );
			switch ( actor.allocation_ ) {				// analyze actor allocation status
			  case Delete: delete &actor; break;
              case Destroy:
				uDEBUG( actor.ticket = SIZE_MAX; );		// mark as terminated
				actor.~uActor(); break;
              case Finished:
				uDEBUG( actor.ticket = SIZE_MAX; );		// mark as terminated
				break;
			  default: ;								// stop warning
			} // switch
			if ( UNLIKELY( uFetchAdd( actors_, -1, __ATOMIC_RELAXED ) == 1 ) ) { // 1 => count is zero and close actor system
				uFence();
				wait_.V();
			} // if
		} // if
	} // uActor::checkActor

	struct Deliver_ {
		uActor & actor;
		Message & msg;

		Deliver_( uActor & actor, Message & msg ) : actor( actor ), msg( msg ) {}

		void operator()() {								// functor
			uDEBUG( if ( actor.allocation_ != Nodelete ) { abort( "Sending message to terminated actor." ); } );
			try {
				Allocation returnedAlloc = actor.process_( msg ); // call current message handler and get returned allocation
				// if promise_allocation or returned allocation is Nodelete it gets overriden by the other
				if ( returnedAlloc == Nodelete ) actor.allocation( actor.promise_allocation_ ); 
				else if ( actor.promise_allocation_ == Nodelete ) actor.allocation( returnedAlloc );
				// if both not Nodelete, this is erroneous behaviour so abort
				else abort( "Changing allocation status of an actor being destroyed/deleted/finished." );

				// Process message first because it might be in actor-related storage (Finished) that is deallocated
				// after the actor terminates.
				checkMsg( msg );						// process message
				checkActor( actor );
			} catch ( uBaseException & ex ) {
				// To have a zero-cost try block, the checkMsg call is duplicated above/below.
				checkMsg( msg );						// process message
				_Throw;									// fail to worker thread
			} catch ( ... ) {
				abort( "C++ exception unsupported from throw in actor for promise message." );
			} // try
		} // Deliver_::operator()
	}; // uActor::Deliver_

	template< typename Func > struct Deliver_Callback_ {
		uActor * actor;
		Func func;

		Deliver_Callback_( uActor * actor, Func func ) : actor( actor ), func( func ) {}

		void operator()() {								// functor
			try {
				if ( UNLIKELY( ! actor ) ) { func(); return; } // actor is nullptr when program main sent callback
				uDEBUG( if ( actor->allocation_ != Nodelete ) { abort( "Sending message to terminated actor." ); } );
				uDEBUG( actor->pendingCallbacks -= 1; );
				actor->allocation_ = func();			// call current callback
				checkActor( *actor );
			} catch ( uBaseException & ex ) {
				_Throw;									// fail to worker thread
			} catch ( ... ) {
				abort( "C++ exception unsupported from throw in actor for promise message." );
			} // try
		} // Deliver_Callback_::operator()
	}; // uActor::Deliver_Callback_

	virtual Allocation process_( Message & msg ) = 0;	// type-safe access to subclass receivePtr
	// Do NOT make pure to allow replacement by "become" in constructor.
	virtual Allocation receive( Message & ) {			// user supplied message handler
		abort( "Must supply receive member for actor." );
		return Delete;
	} // uActor::receive

	void setAllocation( Allocation & to, Allocation from ) {
		if ( to != Nodelete ) abort( "Cannot change allocation status of an actor being deleted/destroyed/finished." );
		to = from;										// can always overwrite Nodelete
	} // uActor::setAllocation
	void promise_allocation( Allocation alloc ) { setAllocation( promise_allocation_, alloc ); } // setter

	static inline uActor * sender() {
		// SKULLDUGGERY: For a call actor.tell(...) in a "receive" member, the "this" in "tell" is the actor receiving
		// the message not the actor sending the message. For "tell" to obtain the sender actor performing the call,
		// requires looking inside the executor thread currently processing the last actor taken from its mailbox.
		uExecutor::Worker * thread = dynamic_cast<uExecutor::Worker *>( &uThisTask() );
		if ( thread == nullptr ) return nullptr;		// program main ?
		return &((uExecutor::VRequest<Deliver_> *)(thread->uThisRequest()))->action.actor;
	} // uActor::sender
  protected:
	template< typename Func > void send_( Func action ) { executor_->send( action, ticket ); }
	virtual void preStart() { /* default empty */ };	// user supplied actor initialization

	// Storage management

	Allocation allocation() { return allocation_; }		// getter
	void allocation( Allocation alloc ) { setAllocation( allocation_, alloc ); } // setter

	struct uActorConstructor {							// translator creates instance in actor constructor
		UPP::uAction action;
		uActor &actor;
		uActorConstructor( UPP::uAction action, uActor &actor ) : action( action ), actor( actor ) {}
		~uActorConstructor( ) {
			uActor &actor = uActorConstructor::actor;
			if ( action == UPP::uYes ) {
				actor.send_( [&actor]() { actor.preStart(); } ); // send preStart call
			} // if
		} // uActorConstructor::uActorConstructor
	}; // uActorConstructor
  public:
	uActor( const uActor & ) = delete;					// no copy
	uActor( uActor && ) = delete;
	uActor & operator=( const uActor & ) = delete;		// no assignment
	uActor & operator=( uActor && ) = delete;

	uActor() {
		// Once an actor is allocated it must be sent a message or the actor system cannot stop. Hence, its receive
		// member must be called to end it, and therefore, the "allocation" field is always initialized by the executor
		// using the return value from "receive" meaning "allocation" does not need to be initialized here. However,
		// there are error checks in debug mode that need "allocation" set to Nodelete.
		uDEBUG( if ( ! executor_ ) { abort( "Creating actor before calling uActor::start()." ); } );
		ticket = executor_->tickets();					// get executor queue handle
		uFetchAdd( actors_, 1, __ATOMIC_RELAXED );		// number of actors in system
	} // uActor::uActor

	virtual ~uActor() noexcept( false ) {				// Unhandled Exception from destructor in uCorActorType
	} // uActor::~uActor

	// Communication

	uActor & tell( Message & msg ) {					// async call, no return value
		uDEBUG( if ( ticket == SIZE_MAX ) abort( "Sending message to terminated actor." ); );
		executor_->send( Deliver_( *this, msg ), ticket ); // copy functor
		return *this;
	} // uActor::tell

	uActor & tell( SenderMsg & msg ) {					// async call, no return value
		msg.sender_ = sender();
		return tell( (Message &)msg );					// cast prevents recursive call
	} // uActor::tell

	uActor & tell( SenderMsg & msg, uActor * sender ) {	// async call, no return value
		msg.sender_ = sender;
		return tell( (Message &)msg );
	} // uActor::tell

	uActor & tell( TraceMsg & msg ) {					// async call, no return value
		msg.pushTrace( this );
		return tell( msg, sender() );					// automatically insert sender into message
	} // uActor::tell

	uActor & operator | ( Message & msg ) {				// operator async call, no return value
		return tell( msg );
	} // uActor::operator|

	uActor & operator | ( SenderMsg & msg ) {			// operator async call, no return value
		return tell( msg );
	} // uActor::operator|

	uActor & operator | ( TraceMsg & msg ) {			// operator async call, no return value
		return tell( msg );
	} // uActor::operator|

	uActor & forward( Message & msg ) {					// async call, no return value
		return tell( msg );
	} // uActor::forward

	uActor & forward( SenderMsg & msg ) {				// async call, no return value
		return tell( msg, msg.sender_ );				// do not update message sender
	} // uActor::forward

	uActor & forward( TraceMsg & msg ) {				// async call, no return value
		msg.pushTrace( this );
		return tell( msg, msg.sender_ );				// do not update message sender
	} // uActor::forward

	template< typename Result > Promise< Result > ask( PromiseMsg< Result > & msg, uActor * sender = nullptr ) { // async call, return promise
		msg.sender_ = sender;
		// Race on send, which publishes the message to this actor. This actor can process and delete the promise result
		// in the message before it is copied by value at the return. Hence, the promise result is copied before
		// publishing and the copy returned.
		auto ret = msg.result_;							// copy
		executor_->send( Deliver_( *this, msg ), ticket ); // publish
		return ret;
	} // uActor::ask

	template< typename Result > Promise< Result > operator || ( PromiseMsg< Result > &msg ) { // operator async call, return promise
		return ask( msg, nullptr );
	} // uActor::operator||

	// Administration

	// use processors on current cluster
	#define uActorStart() uActor::start()				// deprecated
	static void start( uExecutor * executor = nullptr ) { // create executor to run actors
		uDEBUG( if ( executor_ ) { abort( "Duplicate call to uActor::start()." ); } );
		if ( ! executor ) {
			executor_ = new uExecutor( 0, uThisCluster().getProcessors(), false, -1 );
		} else {
			executor_ = executor;
			executorp = true;
		} // if
	} // uActor::start

	#define uActorStop() uActor::stop()					// deprecated
	static bool stop( uDuration duration = 0 ) {		// wait for all actors to terminate or timeout
		uDEBUG( if ( ! executor_ ) { abort( "Calling uActor::stop before calling uActor::start()." ); } );
		bool stopped = true;
		if ( actors_ != 0 ) {							// actors running ?
			if ( duration == 0 ) {						// optimization
				wait_.P();
			} else {
				stopped = wait_.P( duration );			// true => Ved, false => timeout
			} // if
		} // if

		if ( ! executorp ) delete executor_;			// delete if created
		executor_ = nullptr;
		return stopped;									// true => stop, false => timeout
	} // uActor::stop

	// Messages

	static struct StartMsg : public SenderMsg {} startMsg; // start actor
	static struct StopMsg : public SenderMsg {} stopMsg; // terminate actor
	static struct UnhandledMsg : public SenderMsg {} unhandledMsg; // tell error
}; // uActor


// Next two classes allow receivePtr_ to be initialized to the default "receive" member in the actor, where receivePtr_
// is needed to make "become" work.

template< typename Actor > class uActorType : public uActor {
  protected:
	typedef Allocation (Actor:: * Handler)( Message & msg ); // message handler type
  private:
	Handler receivePtr_ = &uActorType<Actor>::receive;	// message handler pointer

	virtual Allocation process_( Message & msg ) override final {
		return (((Actor *)this)->*receivePtr_)(msg);
	} // uActorType::process

	void restart_() {
		receivePtr_ = &uActorType<Actor>::receive;		// restart message-handler pointer
		preStart();										// rerun preStart
	} // uActorType::restart_
  protected:
	// Must be done from within the actor not from outside the actor.  Does not provide a stack of message handlers =>
	// no "unbecome".
	Handler become( Handler handler ) {					// dynamically change message handler
		Handler temp = receivePtr_;
		receivePtr_ = handler;
		return temp;									// return previous message handler
	} // uActorType::become
  public:
	// Administration

	void restart() {									// reset actor to initial state
		send_( [this]() { this->restart_(); } );		// run restart message
	} // uActorType::restart
}; // uActorType


// Multiple inheritance between uBaseCoroutine and uActor, where uBaseCoroutine is first. Hence, coroutine is the
// default address for coroutine actors, so cast to uActor is necessary to match actor address with trace.
template< typename Actor > _Coroutine uCorActorType : public uActor {
  protected:
	typedef Allocation (Actor:: * Handler)( Message & msg ); // message handler type
  private:
	Handler receivePtr_ = &uCorActorType<Actor>::receive; // message handler pointer

	virtual Allocation process_( Message & msg ) override final {
		return (((Actor *)this)->*receivePtr_)(msg);
	} // uCorActorType::process

	void restart_() {
		receivePtr_ = &uCorActorType<Actor>::receive;	// restart message-handler pointer
		preStart();										// rerun preStart
	} // uCorActorType::restart_

	void corFinish() override final __attribute__(( noreturn )) {
		// SKULLDUGGERY: on termination, a coroutine actor cannot return to its starter because its starter is a random
		// executor thread. Instead, the coroutine actor always returns back to its last resumer, which is accomplished
		// by reseting the starter to the last resumer and then shutting down the coroutine.
		starter_ = last_;
		uBaseCoroutine::corFinish();					// shutdown coroutine
	} // uCorActorType::corFinish
  protected:
	// Must be done from within the actor not from outside the actor.  Does not provide a stack of message handlers =>
	// no "unbecome".
	Handler become( Handler handler ) {					// dynamically change message handler
		Handler temp = receivePtr_;
		receivePtr_ = handler;
		return temp;									// return previous message handler
	} // uCorActorType::become
  public:
	// Administration

	void restart() {									// reset actor to initial state
		send_( [this]() { this->restart_(); } );		// run restart message
	} // uCorActorType::restart
}; // uCorActorType


// Local Variables: //
// compile-command: "make install" //
// End: //
