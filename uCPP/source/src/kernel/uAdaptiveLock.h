//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Richard C. Bilson 2008
// 
// uAdaptiveLock.h -- 
// 
// Author           : Richard C. Bilson
// Created On       : Sat Jan 26 11:05:42 2008
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 25 15:11:21 2023
// Update Count     : 51
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


template< int acquireSpins = 0, int trySpins = 0, int releaseSpins = 0 > class uAdaptiveLock {
	unsigned int count;									// number of recursive entries; no overflow checking
	uBaseTask *owner_;									// owner with respect to recursive entry
	uSpinLock spin;
	uQueue<uBaseTaskDL> waiting;
	volatile int spinners;
	volatile unsigned int waker;

	// NB: if tryacquireInternal returns false, "spin" is held
	//     if tryacquireInternal returns true, "spin" is released
	bool tryacquireInternal( uBaseTask &task, int spins __attribute__(( unused )) ) {
		if ( owner_ == &task ) {
			count += 1;
			return true;
		} // if

#ifdef __U_MULTI__
		if ( spins > 0 ) {
			uFetchAdd( spinners, 1 );
			for ( int adaptCount = spins; adaptCount > 0; adaptCount -= 1 ) { // adaptive
				if ( uCompareAssign( owner_, (uBaseTask *)0, &task ) ) {
					uFetchAdd( spinners, -1 );
					count = 1;
					return true;
				} // if

				// SKULLDUGGERY: We don't want to spin if the owner is blocked.  But what if owner_ gets deleted while
				// we're checking?  If getState could fault then we're toast, and we can't check.  However, if getState
				// merely returns an arbitrary value we're ok, since this is only an optimization.
				uBaseTask *testOwner = owner_;
			  if ( testOwner && testOwner->getState() == uBaseTask::Blocked ) break;
			} // for
			uFetchAdd( spinners, -1 );
		} // if
#endif // __U_MULTI__

		spin.acquire();

		// if ( ! waiting.empty() ) return false;			// be fair

		if ( uCompareAssign( owner_, (uBaseTask *)0, &task ) ) {
			spin.release();
			count = 1;
			return true;
		} // if

		return false;
	} // tryacquireInternal

  public:
	uAdaptiveLock( const uAdaptiveLock & ) = delete;	// no copy
	uAdaptiveLock( uAdaptiveLock && ) = delete;
	uAdaptiveLock & operator=( const uAdaptiveLock & ) = delete; // no assignment
	uAdaptiveLock & operator=( uAdaptiveLock && ) = delete;

	uAdaptiveLock() {
		owner_ = nullptr;								// no one owns the lock
		count = 0;										// so count is zero
		spinners = 0;
		waker = 0;
	} // uAdaptiveLock

#ifdef __U_DEBUG__
	~uAdaptiveLock() {
		spin.acquire();
		if ( ! waiting.empty() ) {
			uBaseTask *task = &(waiting.head()->task()); // waiting list could change as soon as spin lock released
			spin.release();
			abort( "Attempt to delete adaptive lock with task %.256s (%p) still on it.", task->getName(), task );
		} // if
		spin.release();
	} // uAdaptiveLock
#endif // __U_DEBUG__

	unsigned int times() const {
		return count;
	} // times

	uBaseTask *owner() const {
		return const_cast< uBaseTask * >( owner_ );
	} // times

	void acquire() {
		uBaseTask &task = uThisTask();					// optimization

#ifdef KNOT
		uThisTask().setActivePriority( uThisTask().getActivePriorityValue() + 1 );
#endif // KNOT
		if ( tryacquireInternal( task, acquireSpins ) ) {
			return;
		} // if
		for ( ;; ) {
			waiting.addTail( &(task.entryRef_) );		// suspend current task
#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::adaptive_lock_queue, 1 );
#endif // __U_STATISTICS__
			UPP::uProcessorKernel::schedule( &spin );	// atomically release owner spin lock and block
#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::adaptive_lock_queue, -1 );
#endif // __U_STATISTICS__
			if ( tryacquireInternal( task, acquireSpins ) ) {
				waker = 0;
				return;
			} // if
			waker = 0;
		} // for
	} // acquire

#if 1
	bool tryacquire() {
		uBaseTask &task = uThisTask();					// optimization

#ifdef KNOT
		uThisTask().setActivePriority( uThisTask().getActivePriorityValue() + 1 );
#endif // KNOT
		if ( owner_ == &task ) {
			count += 1;
			return true;
		} // if

#ifdef __U_MULTI__
		if ( trySpins > 0 ) {
			uFetchAdd( spinners, 1 );
			for ( int adaptCount = trySpins; adaptCount > 0; adaptCount -= 1 ) { // adaptive
				if ( uCompareAssign( owner_, (uBaseTask *)0, &task ) ) {
					uFetchAdd( spinners, -1 );
					count = 1;
					return true;
				} // if

				// SKULLDUGGERY: We don't want to spin if the owner is blocked.  But what if owner_ gets deleted while
				// we're checking?  If getState could fault then we're toast, and we can't check.  However, if getState
				// merely returns an arbitrary value we're ok, since this is only an optimization.
				uBaseTask *testOwner = owner_;
			  if ( testOwner && testOwner->getState() == uBaseTask::Blocked ) break;
			} // for
			uFetchAdd( spinners, -1 );
		} // if
#endif // __U_MULTI__

		if ( uCompareAssign( owner_, (uBaseTask *)0, &task ) ) {
			count = 1;
			return true;
		} // if

#ifdef KNOT
		uThisTask().setActivePriority( uThisTask().getActivePriorityValue() - 1 );
#endif // KNOT
		return false;
	} // tryacquire
#else
	bool tryacquire() {
		uBaseTask &task = uThisTask();					// optimization

		if ( tryacquireInternal( task, acquireSpins ) ) {
			return true;
		} // if

		spin.release();
		return false;
	} // tryacquire
#endif

	void release() {
#ifdef __U_DEBUG__
		uBaseTask &task = uThisTask();
		if ( owner_ != &uThisTask() ) {
			// there is no way to safely mention the actual owner in this error message
			abort( "Attempt by task %.256s (%p) to release adaptive lock %p, which it does not currently own.", task.getName(), &task, this );
		} // if
#endif // __U_DEBUG__

		count -= 1;										// release the lock
		if ( count == 0 ) {								// if this is the last
			owner_ = nullptr;
#ifdef __U_MULTI__
			for ( int adaptCount = releaseSpins; spinners && adaptCount > 0; adaptCount -= 1 ) { // adaptive
				if ( owner_ != nullptr ) {
					return;
				} // if
			} // for
#endif // __U_MULTI__
			// Ideally we would have some way of checking here whether there are queued waiters; if no waiters, we could
			// get out without acquiring the lock.

			// NB: it doesn't work to put this here:
			//		if ( ! waiting.empty() ) {
			// This test here would only work if it was known that a task that is attempting to queue itself based on
			// observing owner != nullptr before we cleared owner would actually succeed in doing so and that the results
			// of that operation would become visible to this task before the above test. But there is no guarantee of
			// that.

			// Remark: this test works after acquiring the spin lock because it's guaranteed that if such a task was
			// attempting to queue it has completed the queueing and that the new state of the queue is now visible to
			// this task.
			spin.acquire();
			if ( waker == 0 && ! waiting.empty() ) {
				waker = 1;
				uBaseTask *heir = &waiting.dropHead()->task();
				spin.release();
				const_cast< uBaseTask * >( heir )->wake(); // restart new owner
			} else {
				spin.release();
			} // if
		} // if
#ifdef KNOT
		uThisTask().setActivePriority( uThisTask().getActivePriorityValue() - 1 );
#endif // KNOT
	} // release
}; // uAdaptiveLock


// Local Variables: //
// compile-command: "make install" //
// End: //
