//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uSemaphore.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue Mar 29 13:42:33 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 19 11:24:25 2022
// Update Count     : 84
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


#ifndef __U_SEMAPHORE_MONITOR__

using UPP::uSemaphore;

#else

_Monitor uSemaphore {
	int count;											// semaphore counter
	uCondition blockedTasks;
  public:
	uSemaphore( int count = 1 ) : count( count ) {
#ifdef __U_STATISTICS__
	    uFetchAdd( UPP::Statistics::uSemaphores, 1 );
#endif // __U_STATISTICS__
#ifdef __U_DEBUG__
		if ( count < 0 ) {
			abort( "Attempt to initialize uSemaphore %p to %d that must be >= 0.", this, count );
		} // if
#endif // __U_DEBUG__
	} // uSemaphore::uSemaphore

	void P() {											// wait on a semaphore
		count -= 1;										// decrement semaphore counter
		if ( count < 0 ) blockedTasks.wait();			// if semaphore less than zero, wait for next V
	} // uSemaphore::P

	// This routine forces the implementation to use internal scheduling, otherwise there is a deadlock problem with    // accepting V in the previous P routine, which prevents calls entering this routine to V the parameter and wait.
	// Essentially, it is necessary to enter the monitor and do some work *before* possibly blocking. To use external
	// scheduling requires accepting either the V routine OR this P routine, which is currently impossible because there
	// is no differentiation between overloaded routines.
	//
	//  _Monitor Semaphore {
	//      int cnt;
	//    public:
	//      Semaphore( int cnt = 1 ) : cnt(cnt) {}
	//      void V() {
	//  	cnt += 1;
	//      }
	//      void P() {
	//  	if ( cnt == 0 ) _Accept( V, P( Semaphore ) );
	//  	cnt -= 1;
	//      }
	//      void P( Semaphore &s ) {
	//  	s.V();
	//  	P();  // reuse code
	//      }
	//  };
	//
	// I'm not sure the external scheduling solution would be any more efficient than the internal scheduling solution.

	void P( uSemaphore &s ) {							// wait on a semaphore and release another
		s.V();											// release other semaphore
		P();											// wait
	} // uSemaphore::P

	bool TryP() {										// conditionally wait on a semaphore
		if ( count > 0 ) {
			count -= 1;									// decrement semaphore counter
			return true;
		};
		return false;
	} // uSemaphore::TryP

	void V( int inc = 1 ) {								// signal a semaphore
#ifdef __U_DEBUG__
		if ( inc < 0 ) {
			abort( "Attempt to advance uSemaphore %p to %d that must be >= 0.", this, inc );
		} // if
#endif // __U_DEBUG__
		count += inc;									// increment semaphore counter
		for ( int i = 0; i < inc; i += 1 ) {			// wake up required number of tasks
			blockedTasks.signal();
		} // for
	} // uSemaphore::V

	_Nomutex int counter() const {						// semaphore counter
		return count;
	} // uSemaphore::counter

	_Nomutex bool empty() const {						// no tasks waiting on semaphore ?
		return count >= 0;
	} // uSemaphore::empty
}; // uSemaphore

#endif // ! __U_SEMAPHORE_MONITOR__


// Local Variables: //
// compile-command: "make install" //
// End: //
