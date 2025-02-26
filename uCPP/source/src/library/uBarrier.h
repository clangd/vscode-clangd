//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1995
// 
// uBarrier.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sat Sep 16 20:56:38 1995
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Oct  7 07:54:31 2023
// Update Count     : 58
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


_Mutex _Coroutine uBarrier {
	uCondition waiters_;
	unsigned int total_, count_;

	void init( unsigned int total ) {
		count_ = 0;
		total_ = total;
	} // uBarrier::init
  protected:
	void main() {
		for ( ;; ) {
			suspend();
		} // for
	} // uBarrier::main

	virtual void last() {								// called by last task to reach the barrier
		resume();
	} // uBarrier::last
  public:
	_Exception BlockFailure {};							// raised if waiting tasks flushed

	uBarrier() {										// for use with reset
		init( 0 );
	} // uBarrier::uBarrier

	uBarrier( unsigned int total ) {
		init( total );
	} // uBarrier::uBarrier

	virtual ~uBarrier() {
	} // uBarrier::~uBarrier

	_Nomutex unsigned int total() const {				// total participants in the barrier
		return total_;
	} // uBarrier::total

	_Nomutex unsigned int waiters() const {				// number of waiting tasks
		return count_;
	} // uBarrier::waiters

	void flush() {
		if ( count_ != 0 ) {
			for ( ; ! waiters_.empty(); ) {				// restart all waiting tasks
				_Resume BlockFailure() _At *(uBaseCoroutine *)waiters_.front();
				waiters_.signal();						// LIFO release, N - 1 cxt switches
			} // for
		} // if
	} // uBarrier::flush

	void reset( unsigned int total ) {
		if ( count_ != 0 ) {
			abort( "(uBarrier &)%p.reset( %d ) : Attempt to reset barrier total while tasks blocked on barrier.", this, total );
		} // if
		init( total );
	} // uBarrier::reset

	virtual void block() {
		count_ += 1;
		if ( count_ < total_ ) {						// all tasks arrived ?
			waiters_.wait( (uintptr_t)&uThisTask() );	// shadow info used by flush
		} else {
			last();										// call the last routine
			for ( ; ! waiters_.empty(); ) {				// restart all waiting tasks
				waiters_.signal();						// LIFO release, N - 1 cxt switches
//				waiters_.signalBlock();					// FIFO release, 2N cxt switches
			} // for
		} // if
		count_ -= 1;
	} // uBarrier::block
}; // uBarrier


// Local Variables: //
// compile-command: "make install" //
// End: //
