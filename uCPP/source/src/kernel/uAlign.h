//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uAlign.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sat Dec 18 22:08:40 1993
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Aug 28 20:47:56 2022
// Update Count     : 57
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


// Minimum size used to align memory boundaries for memory allocations. Minimum size must be >=
// sizeof(uHeapManager::uStorage).

#define uAlign() (__BIGGEST_ALIGNMENT__)

// Check for power of 2

static inline __attribute__((always_inline)) bool uPow2( unsigned long int value ) {
	// clears all bits below value, rounding value down to the next lower multiple of value
	return (value & (value - 1)) == 0;
} // uPow2


// Returns value aligned at the floor of align.

static inline __attribute__((always_inline)) unsigned long int uFloor( unsigned long int value, unsigned long int align ) {
	assert( uPow2( align ) );
	// clears all bits above or equal to align, getting (value % align), the phase of value with regards to align
	return value & -align;
} // uFloor


// Returns value aligned at the ceiling of align.

static inline __attribute__((always_inline)) unsigned long int uCeiling( unsigned long int value, unsigned long int align ) {
	assert( uPow2( align ) );
	// "negate, round down, negate" is the same as round up
	return -uFloor( -value, align );
} // uCeiling


// Local Variables: //
// compile-command: "make install" //
// End: //
