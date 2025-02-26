//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2019
// 
// uFuture.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Jun  3 18:06:58 2019
// Last Modified By : Peter A. Buhr
// Last Modified On : Wed Jan  1 15:30:28 2025
// Update Count     : 13
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
#include <uFuture.h>
size_t uExecutor::next = 0;								// demultiplex across worker buffers

uCancelled::uCancelled() {
	setMsg( "\nAttempt to deliver a result to a future that is cancelled.\n"
			"Possible cause is not checking for cancellation before calling delivery" );
} // uCancelled::uCancelled

uDelivery::uDelivery() {
	setMsg( "\nAttempt to deliver a result to a future already containing a result.\n"
			"Possible cause is forgetting to reset the future before calling delivery." );
} // uDelivery::uDelivery

// Local Variables: //
// compile-command: "make install" //
// End: //
