//                               -*- Mode: C -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2022
// 
// uRandom.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sun Dec 11 18:15:01 2022
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 25 15:27:57 2023
// Update Count     : 42
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

#pragma once

#include <cstdint>										// uintXX_t

#define GLUE2( x, y ) x##y
#define GLUE( x, y ) GLUE2( x, y )

// Set default PRNG for architecture size.
#if defined( __x86_64__ ) || defined( __arm_64__ )		// 64-bit architecture
	// 64-bit generators
	//#define LEHMER64
	//#define XORSHIFT_12_25_27
	#define XOSHIRO256PP
	//#define KISS_64
    // #define SPLITMIX_64

	// 32-bit generators
	//#define XORSHIFT_6_21_7
	#define XOSHIRO128PP
    // #define SPLITMIX_32
#else													// 32-bit architecture
	// 64-bit generators
	//#define XORSHIFT_13_7_17
	#define XOSHIRO256PP
    // #define SPLITMIX_64

	// 32-bit generators
	//#define XORSHIFT_6_21_7
	#define XOSHIRO128PP
    // #define SPLITMIX_32
#endif

// Define C/uC++ PRNG name and random-state.

#ifdef XOSHIRO256PP
#define PRNG_NAME_64 xoshiro256pp
#define PRNG_STATE_64_T GLUE(PRNG_NAME_64,_t)
#endif // XOSHIRO256PP

#ifdef XOSHIRO128PP
#define PRNG_NAME_32 xoshiro128pp
#define PRNG_STATE_32_T GLUE(PRNG_NAME_32,_t)
#endif // XOSHIRO128PP

#ifdef LEHMER64
#define PRNG_NAME_64 lehmer64
#define PRNG_STATE_64_T __uint128_t
#endif // LEHMER64

#ifdef WYHASH64
#define PRNG_NAME_64 wyhash64
#define PRNG_STATE_64_T uint64_t
#endif // LEHMER64

#ifdef XORSHIFT_13_7_17
#define PRNG_NAME_64 xorshift_13_7_17
#define PRNG_STATE_64_T uint64_t
#endif // XORSHIFT_13_7_17

#ifdef XORSHIFT_6_21_7
#define PRNG_NAME_32 xorshift_6_21_7
#define PRNG_STATE_32_T uint32_t
#endif // XORSHIFT_6_21_7

#ifdef XORSHIFT_12_25_27
#define PRNG_NAME_64 xorshift_12_25_27
#define PRNG_STATE_64_T uint64_t
#endif // XORSHIFT_12_25_27

#ifdef SPLITMIX_64
#define PRNG_NAME_64 splitmix64
#define PRNG_STATE_64_T uint64_t
#endif // SPLITMIX32

#ifdef SPLITMIX_32
#define PRNG_NAME_32 splitmix32
#define PRNG_STATE_32_T uint32_t
#endif // SPLITMIX32

#ifdef KISS_64
#define PRNG_NAME_64 kiss_64
#define PRNG_STATE_64_T GLUE(PRNG_NAME_64,_t)
#endif // KISS_^64

#ifdef XORWOW
#define PRNG_NAME_32 xorwow
#define PRNG_STATE_32_T GLUE(PRNG_NAME_32,_t)
#endif // XOSHIRO128PP

#define PRNG_SET_SEED_64 GLUE(PRNG_NAME_64,_set_seed)
#define PRNG_SET_SEED_32 GLUE(PRNG_NAME_32,_set_seed)


// Default PRNG used by runtime.
#if defined( __x86_64__ ) || defined( __arm_64__ )		// 64-bit architecture
#define PRNG_NAME PRNG_NAME_64
#define PRNG_STATE_T PRNG_STATE_64_T
#else													// 32-bit architecture
#define PRNG_NAME PRNG_NAME_32
#define PRNG_STATE_T PRNG_STATE_32_T
#endif

#define PRNG_SET_SEED GLUE(PRNG_NAME,_set_seed)


// ALL PRNG ALGORITHMS ARE OPTIMIZED SO THAT THE PRNG LOGIC CAN HAPPEN IN PARALLEL WITH THE USE OF THE RESULT.
// Specifically, the current random state is copied for returning, before computing the next value.  As a consequence,
// the set_seed routine primes the PRNG by calling it with the state so the seed is not return as the first random
// value.


// https://rosettacode.org/wiki/Pseudo-random_numbers/Splitmix64
//
// Splitmix64 is not recommended for demanding random number requirements, but is often used to calculate initial states
// for other more complex pseudo-random number generators (see https://prng.di.unimi.it).
// Also https://rosettacode.org/wiki/Pseudo-random_numbers/Splitmix64.
static inline uint64_t splitmix64( uint64_t & state ) {
    state += 0x9e3779b97f4a7c15;
    uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
} // splitmix64

static inline void splitmix64_set_seed( uint64_t & state , uint64_t seed ) {
    state = seed;
    splitmix64( state );								// prime
} // splitmix64_set_seed

// https://github.com/bryc/code/blob/master/jshash/PRNGs.md#splitmix32
//
// Splitmix32 is not recommended for demanding random number requirements, but is often used to calculate initial states
// for other more complex pseudo-random number generators (see https://prng.di.unimi.it).

static inline uint32_t splitmix32( uint32_t & state ) {
    state += 0x9e3779b9;
    uint64_t z = state;
    z = (z ^ (z >> 15)) * 0x85ebca6b;
    z = (z ^ (z >> 13)) * 0xc2b2ae35;
    return z ^ (z >> 16);
} // splitmix32

static inline void splitmix32_set_seed( uint32_t & state, uint64_t seed ) {
    state = seed;
    splitmix32( state );								// prime
} // splitmix32_set_seed

#ifdef __SIZEOF_INT128__
//--------------------------------------------------
static inline uint64_t lehmer64( __uint128_t & state ) {
	__uint128_t ret = state;
	state *= 0xda94'2042'e4dd'58b5;
	return ret >> 64;
} // lehmer64

static inline void lehmer64_set_seed( __uint128_t & state, uint64_t seed ) {
	// The seed needs to be coprime with the 2^64 modulus to get the largest period, so no factors of 2 in the seed.
	state = splitmix64( seed );							// prime
} // lehmer64_set_seed

//--------------------------------------------------
static inline uint64_t wyhash64( uint64_t & state ) {
	uint64_t ret = state;
	state += 0x60be'e2be'e120'fc15;
	__uint128_t tmp;
	tmp = (__uint128_t) ret * 0xa3b1'9535'4a39'b70d;
	uint64_t m1 = (tmp >> 64) ^ tmp;
	tmp = (__uint128_t)m1 * 0x1b03'7387'12fa'd5c9;
	uint64_t m2 = (tmp >> 64) ^ tmp;
	return m2;
} // wyhash64

static inline void wyhash64_set_seed( uint64_t & state, uint64_t seed ) {
	state = splitmix64( seed );							// prime
} // wyhash64_set_seed
#endif // __SIZEOF_INT128__

// https://prng.di.unimi.it/xoshiro256starstar.c
//
// This is xoshiro256++ 1.0, one of our all-purpose, rock-solid generators.  It has excellent (sub-ns) speed, a state
// (256 bits) that is large enough for any parallel application, and it passes all tests we are aware of.
//
// For generating just floating-point numbers, xoshiro256+ is even faster.
//
// The state must be seeded so that it is not everywhere zero. If you have a 64-bit seed, we suggest to seed a
// splitmix64 generator and use its output to fill s.

typedef struct { uint64_t s0, s1, s2, s3; } xoshiro256pp_t;

static inline uint64_t xoshiro256pp_rotl( const uint64_t x, int k ) {
	return (x << k) | (x >> (64 - k));
} // xoshiro256pp_rotl

static inline uint64_t xoshiro256pp( xoshiro256pp_t & rs ) {
	const uint64_t result = xoshiro256pp_rotl( rs.s0 + rs.s3, 23 ) + rs.s0;
	const uint64_t t = rs.s1 << 17;

	rs.s2 ^= rs.s0;
	rs.s3 ^= rs.s1;
	rs.s1 ^= rs.s2;
	rs.s0 ^= rs.s3;
	rs.s2 ^= t;
	rs.s3 = xoshiro256pp_rotl( rs.s3, 45 );
	return result;
} // xoshiro256pp

static inline void xoshiro256pp_set_seed( xoshiro256pp_t & state, uint64_t seed ) {
    // To attain repeatable seeding, compute seeds separately because the order of argument evaluation is undefined.
    uint64_t seed1 = splitmix64( seed );				// prime
    uint64_t seed2 = splitmix64( seed );
    uint64_t seed3 = splitmix64( seed );
    uint64_t seed4 = splitmix64( seed );
	state = (xoshiro256pp_t){ seed1, seed2, seed3, seed4 };
} // xoshiro256pp_set_seed

// https://prng.di.unimi.it/xoshiro128plusplus.c
//
// This is xoshiro128++ 1.0, one of our 32-bit all-purpose, rock-solid generators. It has excellent speed, a state size
// (128 bits) that is large enough for mild parallelism, and it passes all tests we are aware of.
//
// For generating just single-precision (i.e., 32-bit) floating-point numbers, xoshiro128+ is even faster.
//
// The state must be seeded so that it is not everywhere zero.

typedef struct { uint32_t s0, s1, s2, s3; } xoshiro128pp_t;

static inline uint32_t xoshiro128pp_rotl( const uint32_t x, int k ) {
	return (x << k) | (x >> (32 - k));
} // xoshiro128pp_rotl

static inline uint32_t xoshiro128pp( xoshiro128pp_t & rs ) {
	const uint32_t result = xoshiro128pp_rotl( rs.s0 + rs.s3, 7 ) + rs.s0;
	const uint32_t t = rs.s1 << 9;

	rs.s2 ^= rs.s0;
	rs.s3 ^= rs.s1;
	rs.s1 ^= rs.s2;
	rs.s0 ^= rs.s3;
	rs.s2 ^= t;
	rs.s3 = xoshiro128pp_rotl( rs.s3, 11 );
	return result;
} // xoshiro128pp

static inline void xoshiro128pp_set_seed( xoshiro128pp_t & state, uint32_t seed ) {
    // To attain repeatable seeding, compute seeds separately because the order of argument evaluation is undefined.
    uint32_t seed1 = splitmix32( seed );				// prime
    uint32_t seed2 = splitmix32( seed );
    uint32_t seed3 = splitmix32( seed );
    uint32_t seed4 = splitmix32( seed );
	state = (xoshiro128pp_t){ seed1, seed2, seed3, seed4 };
} // xoshiro128pp_set_seed

//--------------------------------------------------
static inline uint64_t xorshift_13_7_17( uint64_t & state ) {
	uint64_t ret = state;
	state ^= state << 13;
	state ^= state >> 7;
	state ^= state << 17;
	return ret;
} // xorshift_13_7_17

static inline void xorshift_13_7_17_set_seed( uint64_t & state, uint64_t seed ) {
	state = splitmix64( seed );							// prime
} // xorshift_13_7_17_set_seed

//--------------------------------------------------
// Marsaglia shift-XOR PRNG with thread-local state
// Period is 4G-1
// 0 is absorbing and must be avoided
// Low-order bits are not particularly random
static inline uint32_t xorshift_6_21_7( uint32_t & state ) {
	uint32_t ret = state;
	state ^= state << 6;
	state ^= state >> 21;
	state ^= state << 7;
	return ret;
} // xorshift_6_21_7

static inline void xorshift_6_21_7_set_seed( uint32_t & state, uint32_t seed ) {
    state = splitmix32( seed );							// prime
} // xorshift_6_21_7_set_seed

//--------------------------------------------------
// The state must be seeded with a nonzero value.
static inline uint64_t xorshift_12_25_27( uint64_t & state ) {
	uint64_t ret = state;
	state ^= state >> 12;
	state ^= state << 25;
	state ^= state >> 27;
	return ret * 0x2545'F491'4F6C'DD1D;
} // xorshift_12_25_27

static inline void xorshift_12_25_27_set_seed( uint64_t & state, uint64_t seed ) {
	state = splitmix64( seed );							// prime
} // xorshift_12_25_27_set_seed

//--------------------------------------------------
// The state must be seeded with a nonzero value.
typedef struct { uint64_t z, w, jsr, jcong; } kiss_64_t;

static inline uint64_t kiss_64( kiss_64_t & rs ) {
	kiss_64_t ret = rs;
	rs.z = 36969 * (rs.z & 65535) + (rs.z >> 16);
	rs.w = 18000 * (rs.w & 65535) + (rs.w >> 16);
	rs.jsr ^= (rs.jsr << 13);
	rs.jsr ^= (rs.jsr >> 17);
	rs.jsr ^= (rs.jsr << 5);
	rs.jcong = 69069 * rs.jcong + 1234567;
	return (((ret.z << 16) + ret.w) ^ ret.jcong) + ret.jsr;
} // kiss_64

static inline void kiss_64_set_seed( kiss_64_t & rs, uint64_t seed ) {
	rs.z = 1; rs.w = 1; rs.jsr = 4; rs.jcong = splitmix64( seed );	// prime
} // kiss_64_set_seed

//--------------------------------------------------
// The state array must be initialized to non-zero in the first four words.
typedef struct { uint32_t a, b, c, d, counter; } xorwow_t;

static inline uint32_t xorwow( xorwow_t & rs ) {
	// Algorithm "xorwow" from p. 5 of Marsaglia, "Xorshift RNGs".
	uint32_t ret = rs.a + rs.counter;
	uint32_t t = rs.d;

	uint32_t const s = rs.a;
	rs.d = rs.c;
	rs.c = rs.b;
	rs.b = s;

	t ^= t >> 2;
	t ^= t << 1;
	t ^= s ^ (s << 4);
	rs.a = t;
	rs.counter += 362437;
	return ret;
} // xorwow

static inline void xorwow_set_seed( xorwow_t & rs, uint32_t seed ) {
    // To attain repeatable seeding, compute seeds separately because the order of argument evaluation is undefined.
    uint32_t seed1 = splitmix32( seed );				// prime
    uint32_t seed2 = splitmix32( seed );
    uint32_t seed3 = splitmix32( seed );
    uint32_t seed4 = splitmix32( seed );
	rs = (xorwow_t){ seed1, seed2, seed3, seed4, 0 };
} // xorwow_set_seed

//--------------------------------------------------
// Used in __tls_rand_fwd
#define M  (1llu << 48llu)
#define A  (25'214'903'917llu)
#define AI (18'446'708'753'438'544'741llu)
#define C  (11llu)
#define D  (16llu)

// Bi-directional LCG random-number generator
static inline uint32_t LCGBI_fwd( uint64_t & rs ) {
	rs = (A * rs + C) & (M - 1);
	return rs >> D;
} // LCGBI_fwd

static inline uint32_t LCGBI_bck( uint64_t & rs ) {
	unsigned int r = rs >> D;
	rs = AI * (rs - C) & (M - 1);
	return r;
} // LCGBI_bck

#undef M
#undef A
#undef AI
#undef C
#undef D
