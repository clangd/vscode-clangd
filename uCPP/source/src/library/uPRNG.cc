//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2021
// 
// uPRNG.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sat Dec 25 17:50:36 2021
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Dec 11 20:49:44 2022
// Update Count     : 83
// 
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
#include <uPRNG.h>

//=========================================================


// Cannot be inline in uC++.h because of stupid friendship problem with static.

void set_seed( size_t seed ) {
	PRNG_STATE_T & state = uThisTask().random_state;
	PRNG_SET_SEED( state, seed );
	uBaseTask::thread_random_seed = seed;
	uBaseTask::thread_random_prime = seed;
	uBaseTask::thread_random_mask = true;
} // set_seed

size_t get_seed() { return uBaseTask::thread_random_seed; }
size_t prng() { return PRNG_NAME( uThisTask().random_state ); }


// Local Variables: //
// compile-command: "make install" //
// End: //
