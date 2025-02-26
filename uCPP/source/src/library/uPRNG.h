//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2021
// 
// uPRNG.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sat Dec 25 17:48:48 2021
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Oct  6 07:10:12 2023
// Update Count     : 56
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


#pragma once

#include <cstdint>										// uint32_t

// Sequential Pseudo Random-Number Generator : generate repeatable sequence of values that appear random.
//
// Declaration :
//   PRNG sprng( 1009 ) - set starting seed versus random seed
//
// Interface :
//   sprng.set_seed( 1009 ) - set starting seed for ALL kernel threads versus random seed
//   sprng.get_seed() - read seed
//   sprng() - generate random value in range [0,UINT_MAX]
//   sprng( u ) - generate random value in range [0,u)
//   sprng( l, u ) - generate random value in range [l,u]
//   sprng.calls() - number of generated random value so far
//
// Examples : generate random number between 5-21
//   sprng() % 17 + 5;	values 0-16 + 5 = 5-21
//   sprng( 16 + 1 ) + 5;
//   sprng( 5, 21 );
//   sprng.calls();

class PRNG32 {
	uint32_t callcnt = 0;
	uint32_t seed;										// local seed
	PRNG_STATE_32_T state;								// random state

	PRNG32( const PRNG32 & ) = delete;					// no copy
	PRNG32 & operator=( const PRNG32 & ) = default;		// no public assignment
  public:
	void set_seed( uint32_t seed_ ) { seed = seed_; PRNG_SET_SEED_32( state, seed ); } // set seed
	uint32_t get_seed() const __attribute__(( warn_unused_result )) { return seed; } // get local seed
	PRNG32() { set_seed( uRdtsc() ); }					// random seed
	PRNG32( uint32_t seed ) { set_seed( seed ); }		// fixed seed
	uint32_t operator()() __attribute__(( warn_unused_result )) { callcnt += 1; return PRNG_NAME_32( state ); } // [0,UINT_MAX]
	uint32_t operator()( uint32_t u ) __attribute__(( warn_unused_result )) { return operator()() % u; } // [0,u)
	uint32_t operator()( uint32_t l, uint32_t u ) __attribute__(( warn_unused_result )) { return operator()( u - l + 1 ) + l; } // [l,u]
	uint32_t calls() const __attribute__(( warn_unused_result )) { return callcnt; }
	void copy( PRNG32 & src ) { *this = src; }			// checkpoint PRNG state
}; // PRNG32

class PRNG64 {
	uint64_t callcnt = 0;
	uint64_t seed;										// local seed
	PRNG_STATE_64_T state;								// random state

	PRNG64( const PRNG64 & ) = delete;					// no copy
	PRNG64 & operator=( const PRNG64 & ) = default;		// no public assignment
  public:
	void set_seed( uint64_t seed_ ) { seed = seed_; PRNG_SET_SEED_64( state, seed ); } // set seed
	uint64_t get_seed() const __attribute__(( warn_unused_result )) { return seed; } // get local seed
	PRNG64() { set_seed( uRdtsc() ); }					// random seed
	PRNG64( uint64_t seed ) { set_seed( seed ); }		// fixed seed
	uint64_t operator()() __attribute__(( warn_unused_result )) { callcnt += 1; return PRNG_NAME_64( state ); } // [0,UINT_MAX]
	uint64_t operator()( uint64_t u ) __attribute__(( warn_unused_result )) { return operator()() % u; } // [0,u)
	uint64_t operator()( uint64_t l, uint64_t u ) __attribute__(( warn_unused_result )) { return operator()( u - l + 1 ) + l; } // [l,u]
	uint64_t calls() const __attribute__(( warn_unused_result )) { return callcnt; }
	void copy( PRNG64 & src ) { *this = src; }			// checkpoint PRNG state
}; // PRNG64

#if defined( __x86_64__ ) || defined( __arm_64__ )		// 64-bit architecture
typedef PRNG64 PRNG;
#else													// 32-bit architecture
typedef PRNG32 PRNG;
#endif // __x86_64__


// Concurrent Pseudo Random-Number Generator : generate repeatable sequence of values that appear random.
//
// Interface :
//   set_seed( 1009 ) - fixed seed for all kernel threads versus random seed
//   get_seed() - read seed
//   prng() - generate random value in range [0,UINT_MAX]
//   prng( u ) - generate random value in range [0,u)
//   prng( l, u ) - generate random value in range [l,u]
//
// Examples : generate random number between 5-21
//   prng() % 17 + 5;	values 0-16 + 5 = 5-21
//   prng( 16 + 1 ) + 5;
//   prng( 5, 21 );

// Harmonize with uC++.h.
void set_seed( size_t seed_ );							// set global seed
size_t get_seed() __attribute__(( warn_unused_result )); // get global seed
// SLOWER, global routines
size_t prng() __attribute__(( warn_unused_result ));	// [0,UINT_MAX]
static inline size_t prng( size_t u ) __attribute__(( warn_unused_result ));
static inline size_t prng( size_t u ) { return prng() % u; } // [0,u)
static inline size_t prng( size_t l, size_t u ) __attribute__(( warn_unused_result ));
static inline size_t prng( size_t l, size_t u ) { return prng( u - l + 1 ) + l; } // [l,u]
// FASTER, task members, see uBaseTask in uC++.h 

// Local Variables: //
// compile-command: "make install" //
// End: //
