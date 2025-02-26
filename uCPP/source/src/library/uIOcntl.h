//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uIOcntl.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue Mar 29 16:49:48 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 19 11:26:06 2022
// Update Count     : 73
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


#include <uPoll.h>


//######################### uIOaccess #########################


struct uIOaccess {
	int fd;
	uPoll poll;
}; // uIOaccess


//######################### uIOClosure #########################


struct uIOClosure {
	uIOaccess &access;
	int &retcode;
	int errno_;

	uIOClosure( uIOaccess &access, int &retcode ) : access( access ), retcode( retcode ) {}
	virtual ~uIOClosure() {}

	void wrapper() {
		if ( access.poll.getStatus() == uPoll::PollOnDemand ) access.poll.setPollFlag( access.fd );
		for ( ;; ) {
			retcode = action();
		  if ( retcode != -1 || errno != EINTR ) break; // timer interrupt ?
		} // for
		if ( retcode == -1 ) errno_ = errno;			// preserve errno
		if ( access.poll.getStatus() == uPoll::PollOnDemand ) access.poll.clearPollFlag( access.fd );
	} // uIOClosure::wrapper

	bool select( int mask, uDuration *timeout ) {
		if ( timeout == nullptr ) {
			uThisCluster().select( *this, mask );
		} else {
			timeval t = *timeout;						// convert to timeval for select
			if ( uThisCluster().select( *this, mask, &t ) == 0 ) { // timeout ?
				return false;
			} // if
		} // if
		return true;
	} // uIOClosure::select

	virtual int action() = 0;
}; // uIOClosure


// Local Variables: //
// compile-command: "make install" //
// End: //
