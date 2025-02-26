//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2002
// 
// semaphore.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sun Jan 20 20:34:08 2002
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 19 11:25:13 2022
// Update Count     : 60
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


#define __U_KERNEL__
#include <uC++.h>
#include <uSemaphore.h>
#include <semaphore.h>
#include <cerrno>										// access: EBUSY, EAGAIN, ENOSYS

//#include <uDebug.h>


extern "C" {
	int sem_init( sem_t *sem, int /* pshared */, unsigned int value ) __THROW {
		// storage for a sem_t must be >= u_sem_t
		static_assert( sizeof(sem_t) >= sizeof(uSemaphore), "sizeof(sem_t) < sizeof(uSemaphore)" );
		new( sem ) uSemaphore( value );					// run constructor on supplied storage
		// pshared ignored
		return 0;
	} // sem_init

	int sem_wait( sem_t *sem ) __OLD_THROW {
		((uSemaphore *)sem)->P();
		return 0;
	} // sem_wait

// sem_timedwait()         XOPEN extension

	int sem_trywait( sem_t *sem ) __THROW {
		if ( ! ((uSemaphore *)sem)->TryP() ) {
			errno = EAGAIN;
			return -1;
		} // if
		return 0;
	} // sem_trywait

	int sem_post( sem_t *sem ) __THROW {
		((uSemaphore *)sem)->V();
		return 0;
	} // sem_post

	int sem_getvalue( sem_t *sem, int *sval ) __THROW {
		*sval = ((uSemaphore *)sem)->counter();
		return 0;
	} // sem_getvalue

	int sem_destroy( sem_t *sem ) __THROW {
		if ( ! ((uSemaphore *)sem)->empty() ) {
			errno = EBUSY;
			return -1;
		} else {
			return 0;
		} // if
	} // sem_destroy

	sem_t *sem_open( const char * /* name */, int /* oflag */, ... ) __THROW {
		errno = ENOSYS;
		return SEM_FAILED;
	} // sem_open

	int sem_close( sem_t * /* sem */ ) __THROW {
		errno = ENOSYS;
		return -1;
	} // sem_close

	int sem_unlink( const char * /* name */ ) __THROW {
		errno = ENOSYS;
		return -1;
	} // sem_unlink
} // extern "C"


// Local Variables: //
// compile-command: "make install" //
// End: //
