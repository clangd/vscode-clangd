//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uFloat.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Oct 10 08:30:46 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 25 15:21:00 2023
// Update Count     : 43
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


#ifdef __U_FLOATINGPOINTDATASIZE__
uFloatingPointContext::uFloatingPointContext() : uContext( &uniqueKey ) {
} // uFloatingPointContext::uFloatingPointContext
#endif // __U_FLOATINGPOINTDATASIZE__

void uFloatingPointContext::save() {
#if defined( __i386__ )
	// saved by caller
#elif defined( __x86_64__ )
	// saved by caller
#elif defined( __arm_64__ )
	// saved by context switch
#else
	#error uC++ : internal error, unsupported architecture
#endif
} // uFloatingPointContext::save


void uFloatingPointContext::restore() {
#if defined( __i386__ )
	// restored by caller
#elif defined( __x86_64__ )
	// restored by caller
#elif defined( __arm_64__ )
	// restored by context switch
#else
	#error uC++ : internal error, unsupported architecture
#endif
} // uFloatingPointContext::restore


// Local Variables: //
// compile-command: "make install" //
// End: //
