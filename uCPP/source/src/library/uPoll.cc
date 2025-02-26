//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uPoll.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue Mar 29 17:02:07 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 19 16:42:23 2022
// Update Count     : 44
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
#include <uC++.h>
//#include <uDebug.h>
#include <uPoll.h>

#include <sys/stat.h>
#include <fcntl.h>


void uPoll::setPollFlag( int fd ) {
	int flags;

	for ( ;; ) {
		flags = ::fcntl( fd, F_GETFL, 0 );
	  if ( flags != -1 || errno != EINTR ) break;		// timer interrupt ?
	} // for
	if ( flags == -1 ) {
		uStatus = NeverPoll;
	} else {
		for ( ;; ) {
			flags = ::fcntl( fd, F_SETFL, flags | U_FNDELAY );
		  if ( flags != -1 || errno != EINTR ) break;	// timer interrupt ?
		} // for
		if ( flags == -1 ) {
			uStatus = NeverPoll;
		} // if
	} // if
	uDEBUGPRT( uDebugPrt( "(uPoll &)%p.setPollFlag( %d ), uStatus:%d\n", this, fd, uStatus ); );
} // uPoll::setPollFlag


void uPoll::clearPollFlag( int fd ) {
	int flags;

	for ( ;; ) {
		flags = ::fcntl( fd, F_GETFL, 0 );
	  if ( flags != -1 || errno != EINTR ) break;		// timer interrupt ?
	} // for
	if ( flags == -1 ) {
		uStatus = NeverPoll;
	} else {
		for ( ;; ) {
			flags = ::fcntl( fd, F_SETFL, flags & ~ U_FNDELAY );
		  if ( flags != -1 || errno != EINTR ) break;	// timer interrupt ?
		} // for
		if ( flags == -1 ) {
			uStatus = NeverPoll;
		} // if
	} // if
} // uPoll::clearPollFlag


void uPoll::computeStatus( int fd ) {
	struct stat buf;
	int flags;

	for ( ;; ) {
		flags = ::fstat( fd, &buf );
	  if ( flags != -1 || errno != EINTR ) break;		// timer interrupt ?
	} // for
	uDEBUGPRT( uDebugPrt( "(uPoll &)%p.computeStatus( %d ), flags:%d, st_mode:0x%x\n", this, fd, flags, buf.st_mode ); );
	if ( flags ) {
		uStatus = NeverPoll;							// if the file cannot be stat'ed, never poll
	} else if ( S_ISCHR( buf.st_mode ) ) {
		uStatus = PollOnDemand;							// if a tty, poll on demand
#ifdef S_IFIFO
	} else if ( S_ISFIFO( buf.st_mode ) ) {
		uStatus = AlwaysPoll;							// if a pipe, always poll
#endif
	} else if ( S_ISSOCK( buf.st_mode ) ) {
		uStatus = AlwaysPoll;							// if a socket, always poll
	} else {
		uStatus = NeverPoll;							// otherwise, never
	} // if
	uDEBUGPRT( uDebugPrt( "(uPoll &)%p.computeStatus( %d ), uStatus:%d\n", this, fd, uStatus ); );
} // uPoll::computeStatus


// Local Variables: //
// compile-command: "make install" //
// End: //
