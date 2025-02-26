//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard C. Bilson 2006
// 
// uFuture.h -- 
// 
// Author           : Peter A. Buhr and Richard C. Bilson
// Created On       : Wed Aug 30 22:34:05 2006
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Dec 31 15:34:17 2024
// Update Count     : 1279
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

//############################## uBaseFuture ##############################


// Cannot be nested in future template.

_Exception uFutureFailue {};

_Exception uCancelled : public uFutureFailue {			// raised if future cancelled
  public:
	uCancelled();
}; // uCancelled

_Exception uDelivery : public uFutureFailue {			// raised if future has value/exception
  public:
	uDelivery();
}; // uDelivery


namespace UPP {
	template<typename T> _Monitor uBaseFuture {
		T result;										// future result
	  protected:
		bool addSelect( UPP::BaseFutureDL * selectState ) {
			if ( ! available() ) {
				selectClients.addTail( selectState );
			} // if
			return available();
		} // uBaseFuture::addSelect

		void removeSelect( UPP::BaseFutureDL * selectState ) {
			selectClients.remove( selectState );
		} // uBaseFuture::removeSelect

		uCondition delay;								// clients waiting for future result
		uSequence<UPP::BaseFutureDL> selectClients;		// clients waiting for future result in selection
		uBaseException * cause;							// synchronous exception raised during future computation
		volatile bool available_, cancelled_;			// future status

		void makeavailable() {
			available_ = true;
			while ( ! delay.empty() ) delay.signal();	// unblock waiting clients ?
			if ( ! selectClients.empty() ) {			// select-blocked clients ?
				UPP::BaseFutureDL * bt;					// unblock select-blocked clients
				for ( uSeqIter<UPP::BaseFutureDL> iter( selectClients ); iter >> bt; ) {
					bt->signal();
				} // for
			} // if
		} // uBaseFuture::makeavailable

		void checkFailure() {
			if ( UNLIKELY( cancelled() ) ) _Throw uCancelled();
			if ( UNLIKELY( cause != nullptr ) ) cause->reraise(); // deliver inserted exception
		} // uBaseFuture::check

		_Mutex void doubleCheck() {
			if ( UNLIKELY( ! available() ) ) {			// second check, race with delivery
				delay.wait();							// wait for future delivery
			} // if
		} // uBaseFuture::::check
	  public:
		uBaseFuture() : cause( nullptr ), available_( false ), cancelled_( false ) {}

		_Nomutex bool available() { return available_; } // future result available ?
		_Nomutex bool cancelled() { return cancelled_; } // future result cancelled ?

		// USED BY CLIENT

		_Nomutex const T & operator()() {				// access result, possibly having to wait
			if ( UNLIKELY( ! available() ) ) {			// first check
				doubleCheck();
			} // if
			checkFailure();								// cancelled or exception ?
			return result;
		} // uBaseFuture::operator()()

		// USED BY SERVER

		void delivery( T res ) {						// make result available in the future
			if ( UNLIKELY( cancelled() || available() ) ) _Throw uDelivery(); // already set or client does not want it
			result = res;
			makeavailable();
		} // uBaseFuture::delivery

		void delivery( uBaseException * ex ) {			// make exception available in the future : exception/result mutual exclusive
			if ( UNLIKELY( available() || cancelled() ) ) _Throw uDelivery(); // already set or client does not want it
			cause = ex;
			makeavailable();							// unblock waiting clients ?
		} // uBaseFuture::delivery

		void exception( uBaseException * ex ) __attribute__(( deprecated )) {
			delivery( ex );
		} // uBaseFuture::exception

		void reset() {									// mark future as empty (for reuse)
			#ifdef __U_DEBUG__
			if ( ! delay.empty() || ! selectClients.empty() ) {
				abort( "Attempt to reset future %p with waiting tasks.", this );
			} // if
			#endif // __U_DEBUG__
			available_ = cancelled_ = false;			// reset for next value
			delete cause;
			cause = nullptr;
		} // uBaseFuture::reset
	}; // uBaseFuture
} // UPP


//############################## Future_ESM ##############################


// Caller is responsible for storage management by preallocating the future and passing it as an argument to the
// asynchronous call.  Cannot be copied.

template<typename T, typename ServerData> _Monitor Future_ESM : public UPP::uBaseFuture<T> {
	using UPP::uBaseFuture<T>::cancelled_;
	bool cancelInProgress;

	void makeavailable() {
		cancelInProgress = false;
		cancelled_ = true;
		UPP::uBaseFuture<T>::makeavailable();
	} // Future_ESM::makeavailable

	_Mutex int checkCancel() {
		if ( available() ) return 0;					// already available, can't cancel
		if ( cancelled() ) return 0;					// only cancel once
		if ( cancelInProgress ) return 1;
		cancelInProgress = true;
		return 2;
	} // Future_ESM::checkCancel

	_Mutex void compCancelled() {
		makeavailable();
	} // Future_ESM::compCancelled

	_Mutex void compNotCancelled() {
		// Race by server to deliver and client to cancel.  While the future is already cancelled, the server can
		// attempt to signal (unblock) this client before the client can block, so the signal is lost.
		if ( cancelInProgress ) {						// must recheck
			delay.wait();								// wait for cancellation
		} // if
	} // Future_ESM::compNotCancelled
  public:
	using UPP::uBaseFuture<T>::available;
	using UPP::uBaseFuture<T>::reset;
	using UPP::uBaseFuture<T>::makeavailable;
	using UPP::uBaseFuture<T>::checkFailure;
	using UPP::uBaseFuture<T>::delay;
	using UPP::uBaseFuture<T>::cancelled;

	Future_ESM() : cancelInProgress( false ) {}
	~Future_ESM() { reset(); }

	// USED BY CLIENT

	_Nomutex void cancel() {							// cancel future result
		// To prevent deadlock, call the server without holding future mutex, because server may attempt to deliver a
		// future value. (awkward code)
		size_t rc = checkCancel();
		if ( rc == 0 ) return;							// need to contact server ?
		if ( rc == 1 ) {								// need to contact server ?
			compNotCancelled();							// server computation not cancelled yet, wait for cancellation
		} else {
			if ( serverData.cancel() ) {				// synchronously contact server
				compCancelled();						// computation cancelled, announce cancellation
			} else {
				compNotCancelled();						// server computation not cancelled yet, wait for cancellation
			} // if
		} // if
	} // Future_ESM::cancel

	// USED BY SERVER

	ServerData serverData;								// information needed by server

	void delivery( T res ) {							// make result available in the future
		if ( cancelInProgress ) {
			makeavailable();
		} else {
			UPP::uBaseFuture<T>::delivery( res );
		} // if
	} // Future_ESM::delivery

	void exception( uBaseException * ex ) {				// make exception available in the future : exception and result mutual exclusive
		if ( cancelInProgress ) {
			makeavailable();
		} else {
			UPP::uBaseFuture<T>::exception( ex );
		} // if
	} // Future_ESM::exception
}; // Future_ESM


// handler 4 cases for || and &&: future operator future, future operator binary, binary operator future, binary operator binary

template< typename Future1, typename ServerData1, typename Future2, typename ServerData2 >
UPP::BinarySelector< UPP::UnarySelector< Future_ESM< Future1, ServerData1 > >, UPP::UnarySelector< Future_ESM< Future2, ServerData2 > > > operator||( Future_ESM< Future1, ServerData1 > &f1, Future_ESM< Future2, ServerData2 > &f2 ) {
	//osacquire( cerr ) << "ESM< Future1, Future2 >, Or( f1 " << &f1 << ", f2 " << &f2 << " )" << endl;
	return UPP::BinarySelector< UPP::UnarySelector< Future_ESM< Future1, ServerData1 > >, UPP::UnarySelector< Future_ESM< Future2, ServerData2 > > >( UPP::UnarySelector< Future_ESM< Future1, ServerData1 > >( f1 ), UPP::UnarySelector< Future_ESM< Future2, ServerData2 > >( f2 ), UPP::Condition::Or );
} // operator||

template< typename Left, typename Right, typename Future, typename ServerData >
UPP::BinarySelector< UPP::BinarySelector< Left, Right >, UPP::UnarySelector< Future_ESM< Future, ServerData > > > operator||( UPP::BinarySelector< Left, Right > bs, Future_ESM< Future, ServerData > &f ) {
	//osacquire( cerr ) << "ESM< BinarySelector, Future >, Or( bs " << &bs << ", f " << &f << " )" << endl;
	return UPP::BinarySelector< UPP::BinarySelector< Left, Right >, UPP::UnarySelector< Future_ESM< Future, ServerData > > >( bs, UPP::UnarySelector< Future_ESM< Future, ServerData > >( f ), UPP::Condition::Or );
} // operator||

template< typename Future, typename ServerData, typename Left, typename Right >
UPP::BinarySelector< UPP::UnarySelector< Future_ESM< Future, ServerData > >, UPP::BinarySelector< Left, Right > > operator||( Future_ESM< Future, ServerData > &f, UPP::BinarySelector< Left, Right > bs ) {
	//osacquire( cerr ) << "ESM< Future, BinarySelector >, Or( f " << &f << ", bs " << &bs << " )" << endl;
	return UPP::BinarySelector< UPP::UnarySelector< Future_ESM< Future, ServerData > >, UPP::BinarySelector< Left, Right > >( UPP::UnarySelector< Future_ESM< Future, ServerData > >( f ), bs, UPP::Condition::Or );
} // operator||


template< typename Future1, typename ServerData1, typename Future2, typename ServerData2 >
UPP::BinarySelector< UPP::UnarySelector< Future_ESM< Future1, ServerData1 > >, UPP::UnarySelector< Future_ESM< Future2, ServerData2 > > > operator&&( Future_ESM< Future1, ServerData1 > &f1, Future_ESM< Future2, ServerData2 > &f2 ) {
	//osacquire( cerr ) << "ESM< Future1, Future2 >, And( f1 " << &f1 << ", f2 " << &f2 << " )" << endl;
	return UPP::BinarySelector< UPP::UnarySelector< Future_ESM< Future1, ServerData1 > >, UPP::UnarySelector< Future_ESM< Future2, ServerData2 > > >( UPP::UnarySelector< Future_ESM< Future1, ServerData1 > >( f1 ), UPP::UnarySelector< Future_ESM< Future2, ServerData2 > >( f2 ), UPP::Condition::And );
} // operator&&

template< typename Left, typename Right, typename Future, typename ServerData >
UPP::BinarySelector< UPP::BinarySelector< Left, Right >, UPP::UnarySelector< Future_ESM< Future, ServerData > > > operator&&( UPP::BinarySelector< Left, Right > bs, Future_ESM< Future, ServerData > &f ) {
	//osacquire( cerr ) << "ESM< BinarySelector, Future >, And( bs " << &bs << ", f " << &f << " )" << endl;
	return UPP::BinarySelector< UPP::BinarySelector< Left, Right >, UPP::UnarySelector< Future_ESM< Future, ServerData > > >( bs, UPP::UnarySelector< Future_ESM< Future, ServerData > >( f ), UPP::Condition::And );
} // operator&&

template< typename Future, typename ServerData, typename Left, typename Right >
UPP::BinarySelector< UPP::UnarySelector< Future_ESM< Future, ServerData > >, UPP::BinarySelector< Left, Right > > operator&&( Future_ESM< Future, ServerData > &f, UPP::BinarySelector< Left, Right > bs ) {
	//osacquire( cerr ) << "ESM< Future, BinarySelector >, And( f " << &f << ", bs " << &bs << " )" << endl;
	return UPP::BinarySelector< UPP::UnarySelector< Future_ESM< Future, ServerData > >, UPP::BinarySelector< Left, Right > >( UPP::UnarySelector< Future_ESM< Future, ServerData > >( f ), bs, UPP::Condition::And );
} // operator&&


// shared by ESM and ISM

template< typename Left1, typename Right1, typename Left2, typename Right2 >
UPP::BinarySelector< UPP::BinarySelector< Left1, Right1 >, UPP::BinarySelector< Left2, Right2 > > operator||( UPP::BinarySelector< Left1, Right1 > bs1, UPP::BinarySelector< Left2, Right2 > bs2 ) {
	//osacquire( cerr ) << "ESM/ISM< Future, BinarySelector >, Or( bs1 " << &bs1 << ", bs2 " << &bs2 << " )" << endl;
	return UPP::BinarySelector< UPP::BinarySelector< Left1, Right1 >, UPP::BinarySelector< Left2, Right2 > >( bs1, bs2, UPP::Condition::Or );
} // operator||

template< typename Left1, typename Right1, typename Left2, typename Right2 >
UPP::BinarySelector< UPP::BinarySelector< Left1, Right1 >, UPP::BinarySelector< Left2, Right2 > > operator&&( UPP::BinarySelector< Left1, Right1 > bs1, UPP::BinarySelector< Left2, Right2 > bs2 ) {
	//osacquire( cerr ) << "ESM/ISM< Future, BinarySelector >, And( bs1 " << &bs1 << ", bs2 " << &bs2 << " )" << endl;
	return UPP::BinarySelector< UPP::BinarySelector< Left1, Right1 >, UPP::BinarySelector< Left2, Right2 > >( bs1, bs2, UPP::Condition::And );
} // operator&&


//############################## Future_ISM ##############################


// Future is responsible for storage management by using reference counts.  Can be copied.

template<typename T> class Future_ISM {
  public:
	struct ServerData {
		virtual ~ServerData() {}
		virtual bool cancel() = 0;
	};
  private:
	friend class UPP::UnarySelector<Future_ISM>;		// See uBaseSelector.h

	_Monitor Impl : public UPP::uBaseFuture<T> {		// mutual exclusion implementation
		using UPP::uBaseFuture<T>::cancelled_;
		using UPP::uBaseFuture<T>::cause;
		using UPP::uBaseFuture<T>::addSelect;
		using UPP::uBaseFuture<T>::removeSelect;

		size_t refCnt;									// number of references to future
		ServerData * serverData;
	  public:
		using UPP::uBaseFuture<T>::available;
		using UPP::uBaseFuture<T>::reset;
		using UPP::uBaseFuture<T>::makeavailable;
		using UPP::uBaseFuture<T>::checkFailure;
		using UPP::uBaseFuture<T>::delay;
		using UPP::uBaseFuture<T>::cancelled;

		Impl() : refCnt( 1 ), serverData( nullptr ) {}
		Impl( ServerData * serverData_ ) : refCnt( 1 ), serverData( serverData_ ) {}

		~Impl() {
			delete serverData;
		} // Impl::~Impl

		void incRef() {
			refCnt += 1;
		} // Impl::incRef

		bool decRef() {
			refCnt -= 1;
			if ( refCnt != 0 ) return false;
			delete cause;
			return true;
		} // Impl::decRef

		void cancel() {									// cancel future result
			if ( available() ) return;					// already available, can't cancel
			if ( cancelled() ) return;					// only cancel once
			cancelled_ = true;
			if ( serverData != nullptr ) serverData->cancel();
			makeavailable();							// unblock waiting clients ?
		} // Impl::cancel

		bool addSelect( UPP::BaseFutureDL * selectState ) { // forward
			return UPP::uBaseFuture<T>::addSelect( selectState );
		} // Impl::addSelect

		void removeSelect( UPP::BaseFutureDL * selectState ) { // forward
			return UPP::uBaseFuture<T>::removeSelect( selectState );
		} // Impl::removeSelect
	}; // Impl

	Impl * impl;										// storage for implementation

	bool addSelect( UPP::BaseFutureDL * selectState ) {
		return impl->addSelect( selectState );
	} // Future_ISM::addSelect

	void removeSelect( UPP::BaseFutureDL * selectState ) {
		return impl->removeSelect( selectState );
	} // Future_ISM::removeSelect
  public:
	Future_ISM() : impl( new Impl ) {}
	Future_ISM( ServerData * serverData ) : impl( new Impl( serverData ) ) {}

	~Future_ISM() {
		if ( impl->decRef() ) delete impl;
	} // Future_ISM::~Future_ISM

	Future_ISM( const Future_ISM<T> & rhs ) {
		impl = rhs.impl;								// point at new impl
		impl->incRef();									//   and increment reference count
	} // Future_ISM::Future_ISM

	Future_ISM<T> & operator=( const Future_ISM<T> & rhs ) {
	  if ( rhs.impl == impl ) return *this;
		if ( impl->decRef() ) delete impl;				// no references => delete current impl
		impl = rhs.impl;								// point at new impl
		impl->incRef();									//   and increment reference count
		return *this;
	} // Future_ISM::operator=

	// USED BY CLIENT

	bool available() { return impl->available(); }		// future result available ?
	bool cancelled() { return impl->cancelled(); }		// future result cancelled ?

	const T & operator()() {							// access result, possibly having to wait
		return (*impl)();
	} // Future_ISM::operator()()

	void cancel() {										// cancel future result
		impl->cancel();
	} // Future_ISM::cancel

	bool equals( const Future_ISM<T> &other ) {			// referential equality
		return impl == other.impl;
	} // Future_ISM::equals

	// USED BY SERVER

	void delivery( T result ) {							// make result available in the future
		impl->delivery( result );
	} // Future_ISM::delivery

	void delivery( uBaseException * cause ) {			// make exception available in the future
		impl->delivery( cause );
	} // Future_ISM::delivery

	void exception( uBaseException * cause ) __attribute__(( deprecated )) {
		impl->delivery( cause );
	} // Future_ISM::exception

	void reset() {										// mark future as empty (for reuse)
		impl->reset();
	} // Future_ISM::reset
}; // Future_ISM


// handler 3 cases for || and &&: future operator future, future operator binary, binary operator future, binary operator binary

template< typename Future1, typename Future2 >
UPP::BinarySelector< UPP::UnarySelector< Future_ISM< Future1 > >, UPP::UnarySelector< Future_ISM< Future2 > > > operator||( Future_ISM< Future1 > &f1, Future_ISM< Future2 > &f2 ) {
	//osacquire( cerr ) << "ISM< Future1, Future2 >, Or( f1 " << &f1 << ", f2 " << &f2 << " )" << endl;
	return UPP::BinarySelector< UPP::UnarySelector< Future_ISM< Future1 > >, UPP::UnarySelector< Future_ISM< Future2 > > >( UPP::UnarySelector< Future_ISM< Future1 > >( f1 ), UPP::UnarySelector< Future_ISM< Future2 > >( f2 ), UPP::Condition::Or );
} // operator||

template< typename Left, typename Right, typename Future >
UPP::BinarySelector< UPP::BinarySelector< Left, Right >, UPP::UnarySelector< Future_ISM< Future > > > operator||( UPP::BinarySelector< Left, Right > bs, Future_ISM< Future > &f ) {
	//osacquire( cerr ) << "ISM< BinarySelector, Future >, Or( bs " << &bs << ", f " << &f << " )" << endl;
	return UPP::BinarySelector< UPP::BinarySelector< Left, Right >, UPP::UnarySelector< Future_ISM< Future > > >( bs, UPP::UnarySelector< Future_ISM< Future > >( f ), UPP::Condition::Or );
} // operator||

template< typename Future, typename Left, typename Right >
UPP::BinarySelector< UPP::UnarySelector< Future_ISM< Future > >, UPP::BinarySelector< Left, Right > > operator||( Future_ISM< Future > &f, UPP::BinarySelector< Left, Right > bs ) {
	//osacquire( cerr ) << "ISM< Future, BinarySelector >, Or( f " << &f << ", bs " << &bs << " )" << endl;
	return UPP::BinarySelector< UPP::UnarySelector< Future_ISM< Future > >, UPP::BinarySelector< Left, Right > >( UPP::UnarySelector< Future_ISM< Future > >( f ), bs, UPP::Condition::Or );
} // operator||


template< typename Future1, typename Future2 >
UPP::BinarySelector< UPP::UnarySelector< Future_ISM< Future1 > >, UPP::UnarySelector< Future_ISM< Future2 > > > operator&&( Future_ISM< Future1 > &f1, Future_ISM< Future2 > &f2 ) {
	//osacquire( cerr ) << "ISM< Future1, Future2 >, And( f1 " << &f1 << ", f2 " << &f2 << " )" << endl;
	return UPP::BinarySelector< UPP::UnarySelector< Future_ISM< Future1 > >, UPP::UnarySelector< Future_ISM< Future2 > > >( UPP::UnarySelector< Future_ISM< Future1 > >( f1 ), UPP::UnarySelector< Future_ISM< Future2 > >( f2 ), UPP::Condition::And );
} // operator&&

template< typename Left, typename Right, typename Future >
UPP::BinarySelector< UPP::BinarySelector< Left, Right >, UPP::UnarySelector< Future_ISM< Future > > > operator&&( UPP::BinarySelector< Left, Right > bs, Future_ISM< Future > &f ) {
	//osacquire( cerr ) << "ISM< BinarySelector, Future >, And( bs " << &bs << ", f " << &f << " )" << endl;
	return UPP::BinarySelector< UPP::BinarySelector< Left, Right >, UPP::UnarySelector< Future_ISM< Future > > >( bs, UPP::UnarySelector< Future_ISM< Future > >( f ), UPP::Condition::And );
} // operator&&

template< typename Future, typename Left, typename Right >
UPP::BinarySelector< UPP::UnarySelector< Future_ISM< Future > >, UPP::BinarySelector< Left, Right > > operator&&( Future_ISM< Future > &f, UPP::BinarySelector< Left, Right > bs ) {
	//osacquire( cerr ) << "ISM< Future, BinarySelector >, And( f " << &f << ", bs " << &bs << " )" << endl;
	return UPP::BinarySelector< UPP::UnarySelector< Future_ISM< Future > >, UPP::BinarySelector< Left, Right > >( UPP::UnarySelector< Future_ISM< Future > >( f ), bs, UPP::Condition::And );
} // operator&&


//############################## uWaitQueue_ISM ##############################


template< typename Selectee >
class uWaitQueue_ISM {
	struct DL;

	struct DropClient {
		UPP::uSemaphore sem;							// selection client waits if no future available
		size_t tst;										// test-and-set for server race
		DL * winner;									// indicate winner of race

		DropClient() : sem( 0 ), tst( 0 ) {};
	}; // DropClient

	struct DL : public uSeqable {
		struct uBaseFutureDL : public UPP::BaseFutureDL {
			DropClient * client;						// client data for server
			DL * s;										// iterator corresponding to this DL

			uBaseFutureDL( DL * t ) : s( t ) {}

			virtual void signal() {
				if ( uTestSet( client->tst ) == 0 ) {	// returns 0 or non-zero
					client->winner = s;
					client->sem.V();					// client see changes because semaphore does memory barriers
				} // if
			} // signal
		}; // uBaseFutureDL

		uBaseFutureDL selectState;
		Selectee selectee;

		DL( Selectee t ) : selectState( this ), selectee( t ) {}
	}; // DL

	uSequence< DL > q;
  public:
	uWaitQueue_ISM( const uWaitQueue_ISM & ) = delete;	// no copy
	uWaitQueue_ISM( uWaitQueue_ISM && ) = delete;
	uWaitQueue_ISM & operator=( const uWaitQueue_ISM & ) = delete; // no assignment
	uWaitQueue_ISM & operator=( uWaitQueue_ISM && ) = delete;

	uWaitQueue_ISM() {}

	template< typename Iterator > uWaitQueue_ISM( Iterator begin, Iterator end ) {
		add( begin, end );
	} // uWaitQueue_ISM::uWaitQueue_ISM

	~uWaitQueue_ISM() {
		DL * t;
		for ( uSeqIter< DL > i( q ); i >> t; ) {
			delete t;
		} // for
	} // uWaitQueue_ISM::~uWaitQueue_ISM

	bool empty() const {
		return q.empty();
	} // uWaitQueue_ISM::empty

	void add( Selectee n ) {
		q.add( new DL( n ) );
	} // uWaitQueue_ISM::add

	template< typename Iterator > void add( Iterator begin, Iterator end ) {
		for ( Iterator i = begin; i != end; ++i ) {
			add( *i );
		} // for
	} // uWaitQueue_ISM::add

	void remove( Selectee n ) {
		DL * t = 0;
		for ( uSeqIter< DL > i( q ); i >> t; ) {
			if ( t->selectee.equals( n ) ) {
				q.remove( t );
				delete t;
			} // if
		} // for
	} // uWaitQueue_ISM::remove

	Selectee drop() {
		if ( q.empty() ) abort( "uWaitQueue_ISM: attempt to drop from an empty queue" );

		DropClient client;
		DL * t = 0;
		for ( uSeqIter< DL > i( q ); i >> t; ) {
			t->selectState.client = &client;
			if ( t->selectee.addSelect( &t->selectState ) ) {
				DL * s;
				for ( uSeqIter< DL > j( q ); j >> s && s != t; ) {
					s->selectee.removeSelect( &s->selectState );
				} // for
				goto cleanup;
			} // if
		} // for

		client.sem.P();
		t = client.winner;
		DL * s;
		for ( uSeqIter< DL > i( q ); i >> s; ) {
			s->selectee.removeSelect( &s->selectState );
		} // for

	  cleanup:
		Selectee selectee = t->selectee;
		q.remove( t );
		delete t;
		return selectee;
	} // uWaitQueue_ISM::drop

	// not implemented, since the "head" of the queue is not fixed i.e., if another item comes ready it may become the
	// new "head" use "drop" instead
	// T * head() const;
}; // uWaitQueue_ISM


//############################## uWaitQueue_ESM ##############################


template< typename Selectee >
class uWaitQueue_ESM {
	struct Helper {
		Selectee * s;
		Helper( Selectee * s ) : s( s ) {}
		bool available() const { return s->available(); }
		bool addSelect( UPP::BaseFutureDL * selectState ) { return s->addSelect( selectState ); }
		void removeSelect( UPP::BaseFutureDL * selectState ) { return s->removeSelect( selectState ); }
		bool equals( const Helper &other ) const { return s == other.s; }
	}; // Helper

	uWaitQueue_ISM< Helper > q;
  public:
	uWaitQueue_ESM( const uWaitQueue_ESM & ) = delete;	// no copy
	uWaitQueue_ESM( uWaitQueue_ESM && ) = delete;
	uWaitQueue_ESM & operator=( const uWaitQueue_ESM & ) = delete; // no assignment
	uWaitQueue_ESM & operator=( uWaitQueue_ESM && ) = delete;

	uWaitQueue_ESM() {}

	template< typename Iterator > uWaitQueue_ESM( Iterator begin, Iterator end ) {
		add( begin, end );
	} // uWaitQueue_ESM::uWaitQueue_ESM

	bool empty() const {
		return q.empty();
	} // uWaitQueue_ESM::empty

	void add( Selectee * n ) {
		q.add( Helper( n ) );
	} // uWaitQueue_ESM::add

	template< typename Iterator > void add( Iterator begin, Iterator end ) {
		for ( Iterator i = begin; i != end; ++i ) {
			add( &*i );
		} // for
	} // uWaitQueue_ESM::add

	void remove( Selectee * s ) {
		q.remove( Helper( s ) );
	} // uWaitQueue_ESM::remove

	Selectee * drop() {
		return empty() ? 0 : q.drop().s;
	} // uWaitQueue_ESM::drop
}; // uWaitQueue_ESM


//############################## uExecutor ##############################

#include <uDefaultExecutor.h>

class uExecutor {
	friend class uActor;
  private:
	// Mutex buffer is embedded in the nomutex executor to allow the executor to delete the workers without causing a
	// deadlock.  If the executor is the monitor and the buffer is class, the thread calling the executor's destructor
	// (which is mutex) blocks when deleting the workers, preventing outstanding workers from calling remove to drain
	// the buffer.

//#define LOCKFREE
#define SPINLOCK

#if defined( LOCKFREE )
	#define CALIGN __attribute__(( aligned (64) ))

	// http://www.1024cores.net/home/lock-free-algorithms/queues/intrusive-mpsc-node-based-queue

	#define UCOLABLE Buffer_Colable

	struct Buffer_Colable {
		Buffer_Colable * volatile next;
	}; // Buffer_Colable

	template< typename ELEMTYPE > class Buffer {
	  protected:
		ELEMTYPE pad1 CALIGN, * volatile head CALIGN, * tail CALIGN, stub CALIGN, pad2 CALIGN;
	  public:
		Buffer() {
			tail = head = &stub;
			stub.next = 0;
		} // Buffer::Buffer

		void insert( ELEMTYPE * n ) {
			n->next = 0;
			ELEMTYPE * prev = __sync_lock_test_and_set( &head, n );
			prev->next = n;
		} // Buffer::insert

		ELEMTYPE * remove() {
			ELEMTYPE * tail_ = tail, * next_ = (ELEMTYPE *)(tail_->next);
			if ( tail_ == &stub ) {
				if ( next_ == nullptr ) return nullptr;
				tail = next_;
				tail_ = next_;
				next_ = (ELEMTYPE *)(next_->next);
			} // if

			if ( next_ ) {
				tail = next_;
				return tail_;
			} // if

			Buffer_Colable * head_ = head;
			if ( tail_ != head_ ) return nullptr;
			insert( &stub );
			next_ = (ELEMTYPE *)(tail_->next);
			if ( next_ ) { tail = next_; return tail_; }
			return nullptr;
		} // Buffer::remove
	}; // Buffer

#elif defined( SPINLOCK )
	#define UCOLABLE uColable
#if 0
	template< typename ELEMTYPE > class Buffer {		// unbounded buffer
		uSpinLock mutex;
		uQueue< ELEMTYPE > buf;							// unbounded list of work requests
	  public:
		void insert( ELEMTYPE * elem ) {
			mutex.acquire();
			buf.addTail( elem );						// insert element into buffer
			mutex.release();
		} // Buffer::insert

		ELEMTYPE * remove() {
			mutex.acquire();
			ELEMTYPE * ret = buf.dropHead();
			mutex.release();
			return ret;
		} // Buffer::remove
	}; // Buffer
#else
	template< typename ELEMTYPE > class Buffer {		// unbounded buffer
		uSpinLock mutex;
		uQueue< ELEMTYPE > input;						// unbounded list of work requests
	  public:
		void insert( ELEMTYPE * elem ) {
			mutex.acquire();
			input.addTail( elem );						// insert element into buffer
			mutex.release();
		} // Buffer::insert

		void transfer( uQueue< ELEMTYPE > * transferTo ) {
		  if ( input.empty() ) return;					// optimize race: if missed this round get it next round
			mutex.acquire();
			transferTo->transfer( input );				// transfer input to output
			mutex.release();
		} // Buffer::transfer
	}; // Buffer
#endif // 0

#else // monitor

	#define UCOLABLE uColable
	template< typename ELEMTYPE > _Monitor Buffer {		// unbounded buffer
		uQueue< ELEMTYPE > buf;							// unbounded list of work requests
		uCondition delay;
	  public:
		void insert( ELEMTYPE * elem ) {
			buf.addTail( elem );						// insert element into buffer
			delay.signal();								// restart
		} // Buffer::insert

		ELEMTYPE * remove() {
//			if ( buf.empty() ) delay.wait();			// no request to process ? => wait
			if ( buf.empty() ) return nullptr;			// no request to process ? => spin
			return buf.dropHead();
		} // Buffer::remove
	}; // Buffer

#endif // LOCKTYPE

	struct WRequest : public UCOLABLE {					// worker request
		virtual ~WRequest() {};							// required for FRequest's result
		virtual bool stop() { return true; };
		virtual void doit() { assert( false ); };		// not abstract as used for sentinel
	}; // WRequest

	template< typename F > struct VRequest : public WRequest { // client request, no return
		F action;
		bool stop() { return false; };
		void doit() { action(); }
		VRequest( F action ) : action( action ) {}
	}; // VRequest

	// Each worker has its own set (when requests buffers > workers) of work buffers to reduce contention between client
	// and server, where work requests arrive and are distributed into buffers in a roughly round-robin order.
	_Task Worker {
		Buffer< WRequest > * requests;
		uQueue< WRequest > output;
		WRequest * request;
		size_t start, range;
		// size_t doits = 0, spins = 0;

		void main() {
			setName( "Executor Worker" );
		  Exit:
			for ( size_t i = 0;; i = (i + 1) % range ) { // cycle through set of request buffers
				requests[i + start].transfer( &output );
				while ( ! output.empty() ) {
					request = output.dropHead();
					if ( ! request ) {
						#if ! defined( __U_MULTI__ )
						uThisTask().uYieldNoPoll();
						#endif // ! __U_MULTI__
						// spins += 1;
						continue;
					} // if
			  if ( request->stop() )  {
						//printf( "worker %p requests %d spins %d\n", this, doits, spins );
						break Exit;
					} // exit

					//doits += 1;
					request->doit();
					//printf( "worker start %p %d %d\n", this, start, range );
					delete request;
				} // while
			} // for
		} // Worker::main
	  public:
		Worker( uCluster & wc, Buffer< WRequest > * requests, size_t start, size_t range ) :
			uBaseTask( wc ), requests( requests ), request( nullptr ), start( start ), range( range ) {}

		WRequest * uThisRequest() { return request; }
	}; // Worker

	uCluster * cluster;									// if workers execute on separate cluster
	uNoCtor< uProcessor > * processors;					// array of virtual processors adding parallelism for workers
	Buffer< WRequest > * requests;						// list of work requests
	uNoCtor< Worker >* workers;							// array of workers executing work requests
	const size_t nprocessors, nworkers, nrqueues;		// number of processors/threads/request queues
	const bool sepClus;									// use same or separate cluster for executor
	static size_t next;									// demultiplex across worker buffers

	size_t tickets() {
		return uFetchAdd( next, 1 ) % nrqueues;
		// return next++ % nrqueues;						// no locking, interference randomizes
	} // uExecutor::tickets

	template< typename Func > void send( Func action, size_t ticket ) { // asynchronous call, no return value
		VRequest< Func > * node = new VRequest< Func >( action );
		requests[ticket].insert( node );
	} // uExecutor::send
  public:
	uExecutor( size_t nprocessors, size_t nworkers, size_t nrqueues, bool sepClus = uDefaultExecutorSepClus(), int affAffinity = uDefaultExecutorAffinity() ) :
			nprocessors( nprocessors ), nworkers( nworkers ), nrqueues( nrqueues ), sepClus( sepClus ) {
		if ( nrqueues < nworkers ) abort( "nrqueues >= nworkers\n" );
		cluster = sepClus ? new uCluster( "uExecutor" ) : &uThisCluster();
		processors = new uNoCtor< uProcessor >[ nprocessors ];
		requests = new Buffer< WRequest >[ nrqueues ];
		workers = new uNoCtor< Worker >[ nworkers ];

		//uDEBUGPRT( uDebugPrt( "uExecutor::uExecutor nprocessors %u nworkers %u nrqueues %u sepClus %d affAffinity %d\n", nprocessors, nworkers, nrqueues, sepClus, affAffinity ); )
		//printf( "uExecutor::uExecutor nprocessors %u nworkers %u nrqueues %u sepClus %d affAffinity %d\n", nprocessors, nworkers, nrqueues, sepClus, affAffinity );
		for ( size_t i = 0; i < nprocessors; i += 1 ) {
			processors[ i ].ctor( *cluster );
			if ( affAffinity != -1 ) {
				processors[i]->setAffinity( i + affAffinity );
			} // if
		} // for

		size_t reqPerWorker = nrqueues / nworkers, extras = nrqueues % nworkers;
		for ( size_t i = 0, start = 0, range; i < nworkers; i += 1, start += range ) {
			range = reqPerWorker + ( i < extras ? 1 : 0 );
			workers[i].ctor( *cluster, requests, start, range );
		} // for
	} // uExecutor::uExecutor

	uExecutor( size_t nprocessors, size_t nworkers, bool sepClus = uDefaultExecutorSepClus(), int affAffinity = uDefaultExecutorAffinity() ) : uExecutor( nprocessors, nworkers, nworkers, sepClus, affAffinity ) {}
	uExecutor( size_t nprocessors, bool sepClus = uDefaultExecutorSepClus(), int affAffinity = uDefaultExecutorAffinity() ) : uExecutor( nprocessors, nprocessors, nprocessors, sepClus, affAffinity ) {}
	uExecutor() : uExecutor( uDefaultExecutorProcessors(), uDefaultExecutorWorkers(), uDefaultExecutorRQueues(), uDefaultExecutorSepClus(), uDefaultExecutorAffinity() ) {}

	~uExecutor() {
		// Add one sentinel per worker to stop them. Since in destructor, no new external work should be queued after
		// sentinel.
		WRequest sentinel[nworkers];
		size_t reqPerWorker = nrqueues / nworkers, extras = nrqueues % nworkers;
		for ( size_t i = 0, start = 0, range; i < nworkers; i += 1, start += range ) {
			range = reqPerWorker + ( i < extras ? 1 : 0 );
			requests[start].insert( &sentinel[i] );		// force eventually termination
		} // for

		delete [] workers;
		delete [] requests;
		delete [] processors;
		if ( sepClus ) { delete cluster; }
	} // uExecutor::~uExecutor

	template< typename Func > void send( Func action ) { // asynchronous call, no return value
		send( action, tickets() );
	} // uExecutor::send

	template< typename Func, typename R > void sendrecv( Func & action, Future_ISM<R> & future ) { // asynchronous call, output return value
		send( [&future, &action]() { future.delivery( action() ); }, tickets() );
	} // uExecutor::send

	template< typename Func, typename R > void sendrecv( Func && action, Future_ISM<R> & future ) { // asynchronous call, output return value
		send( [&future, &action]() { future.delivery( action() ); }, tickets() );
	} // uExecutor::send
}; // uExecutor


// Local Variables: //
// compile-command: "make install" //
// End: //
