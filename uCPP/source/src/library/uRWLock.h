//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2009
// 
// uRWLock.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue May  5 12:53:33 2009
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 19 16:43:58 2022
// Update Count     : 15
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


class uRWLock {
	enum RW { READER, WRITER };							// kinds of tasks
	uSequence<uBaseTaskDL> waiting;
	// Cannot pass ownership of spinlock to another task.
	uSpinLock entry;
	unsigned int rwdelay, rcnt, wcnt;

	void wunblock() {
		wcnt += 1;
		rwdelay -= 1;
		uBaseTaskDL *node = waiting.dropHead();			// remove task at head of waiting list
		entry.release();								// put baton down
		node->task().wake();							// and wake task
	} // uRWLock::wunblock

	void block( RW kind ) {
		rwdelay += 1;
		uBaseTask &task = uThisTask();					// optimization
		task.info_ = kind;								// store the kind with this task
		waiting.addTail( &(task.entryRef_) );			// block current task
#ifdef __U_STATISTICS__
		uFetchAdd( UPP::Statistics::owner_lock_queue, 1 );
#endif // __U_STATISTICS__
		UPP::uProcessorKernel::schedule( &entry );		// atomically release spin lock and block
#ifdef __U_STATISTICS__
		uFetchAdd( UPP::Statistics::owner_lock_queue, -1 );
#endif // __U_STATISTICS__
	} // uRWLock::block
  public:
	uRWLock() {
		rwdelay = rcnt = wcnt = 0;
	} // uRWLock::uRWLock

	inline unsigned int rdcnt() const { return rcnt; }
	inline unsigned int wrcnt() const { return wcnt; }

	void rdacquire() {
		entry.acquire();								// entry protocol
		if ( wcnt > 0 || rwdelay > 0 ) {				// resource in use ?
			block( READER );
		} else {
			rcnt += 1;
			entry.release();							// put baton down
		} // if
	} // uRWLock::rdacquire

	void rdrelease() {
		entry.acquire();								// exit protocol
		rcnt -= 1;
		if ( rcnt == 0 && rwdelay > 0 ) {				// last reader ?
			wunblock();									// must be a writer at head of queue
		} else {
			entry.release();							// put baton down
		} // if
	} // uRWLock::rdrelease

	void wracquire() {
		entry.acquire();								// entry protocol
		if ( rcnt > 0 || wcnt > 0 ) {					// resource in use ?
			block( WRITER );
		} else {
			wcnt += 1;
			entry.release();							// put baton down
		} // if
	} // uRWLock::wracquire

	void wrrelease() {
		entry.acquire();								// exit protocol
		wcnt -= 1;
		if ( rwdelay > 0 ) {							// anyone waiting ?
			if ( waiting.head()->task().info_ == WRITER ) {
				wunblock();
			} else {
				uBaseTaskDL *n = waiting.head();
				for ( ;; ) {							// more readers ?
					rcnt += 1;
					rwdelay -= 1;
					n->task().setState( uBaseTask::Ready );
					if ( rwdelay == 0 || waiting.succ( n )->task().info_ != READER ) break;
					n = waiting.succ( n );
				} // for
				uSequence<uBaseTaskDL> unblock;
				int lrcnt = rcnt;
				unblock.split( waiting, n );
				entry.release();						// put baton down
				// for ( n = unblock.dropHead(); n != nullptr; n = unblock.dropHead() ) {
				// 	cout << &uThisTask() << " " << n << " " << n->task().info_ << endl;
				// 	n->task().wake();					// and wake task
				// } // for
				uThisCluster().makeTaskReady( unblock, lrcnt );
			} // if
		} else {
			entry.release();							// put baton down
		} // if
	} // uRWLock::wrrelease
}; // uRWLock


// Local Variables: //
// compile-command: "make install" //
// End: //
