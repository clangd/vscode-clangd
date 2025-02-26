//                               -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Martin Karsten 2019
// Copyright (C) Peter A. Buhr 2019
// 
// uReadersWriterLock.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue Jun 25 13:32:36 2019
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Apr  3 10:13:28 2022
// Update Count     : 8
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

#include <uAtomic.h>

#if 1
// simple spinning RW lock: all racing - starvation possible
class uRWspinlock {
  public:
	volatile ssize_t value = 0;							// -1 writer, 0 open, >0 readers
	bool tryAcquireRead() {
		ssize_t s = value;
		return (s >= 0) && __atomic_compare_exchange_n( &value, &s, s + 1, false, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED );
	}
	void acquireRead() {
		while ( ! tryAcquireRead() ) uPause();
	}
	bool tryAcquireWrite() {
		ssize_t s = value;
		return (s == 0) && __atomic_compare_exchange_n( &value, &s, s - 1, false, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED );
	}
	void acquireWrite() {
		while ( ! tryAcquireWrite() ) uPause();
	}
	void release() {
		assert( value != 0 );
		if ( value < 0) __atomic_add_fetch( &value, 1, __ATOMIC_SEQ_CST );
		else __atomic_sub_fetch( &value, 1, __ATOMIC_SEQ_CST );
	}
};

#else

class uRWspinlock {
	volatile size_t lock = 0; // contains two different bit fields: [ number of readers ] [ 3 lock-state bits ]
  public:
	inline void init() {
		lock = 0;
	}
	inline bool isWriteLocked() {
		return lock & 1;
	}
	inline bool isReadLocked() {
		return lock & ~3;
	}
	inline bool isUpgrading() {
		return lock & 2;
	}
	inline bool isLocked() {
		return lock;
	}
	inline void readLock() {
		__sync_add_and_fetch(&lock, 4);
		while ( isWriteLocked() ) uPause();
		return;
	}
	inline void readUnlock() {
		__sync_add_and_fetch(&lock, -4);
	}
	// can call this only if you already read-locked.  upgrade the read lock to a write lock by first taking an upgrade
	// bit (2) and then (repeatedly) casing from an upgrader & zero readers (=2) to writer & zero readers (=1) (if
	// someone else takes the upgrade bit, we must give up and release our reader locks to prevent deadlock)
	inline bool upgradeLock() {
		while (1) {
			auto expval = lock;
			if (expval & 2) return false;
			auto seenval = __sync_val_compare_and_swap(&lock, expval, (expval - 4) | 2 /* subtract our reader count and covert to upgrader */);
			if (seenval == expval) {					// cas success
				// cas to writer
				while (1) {
					while ( lock & ~2 /* locked by someone else */ ) uPause();
					if (__sync_bool_compare_and_swap(&lock, 2, 1)) {
						return true;
					}
				}
			}
		}
	}
	inline void writeLock() {
		while (1) {
			while ( isLocked() ) uPause();
			if (__sync_bool_compare_and_swap(&lock, 0, 1)) {
				return;
			}
		}
	}
	inline void writeUnlock() {
		__sync_add_and_fetch(&lock, -1);
	}
};

#endif // 0/1

// Local Variables: //
// compile-command: "make install" //
// End: //
