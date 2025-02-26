//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// assert.h --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 16:01:41 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Dec 24 18:21:20 2021
// Update Count     : 38
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

// This include file is not suppose to be idempotent, so there is no guard.


#undef	assert

#ifdef NDEBUG

#define uassert( expr ) ((void)0)

#else

#include <iostream>
#include <cstdlib>					// exit

using std::cerr;
using std::endl;

#include "output.h"
#include "gen.h"

#define uassert( expr ) \
	if ( ! ( expr ) ) { \
	cerr << __FILE__ << ":" << __LINE__ << ": assertion failed" << endl; \
		gen_error( ahead, "assertion failed at this line." ); \
		write_all_output(); \
		exit( EXIT_FAILURE ); \
	} // if

#endif // NDEBUG


// Local Variables: //
// compile-command: "make install" //
// End: //
