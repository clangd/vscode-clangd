// 
// uC++ Version 7.0.0, Copyright (C) Richard C. Bilson 2004
// 
// uAtomic.h -- atomic routines for various processors
// 
// Author           : Richard C. Bilson
// Created On       : Thu Sep 16 13:57:26 2004
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Aug 22 23:08:47 2024
// Update Count     : 171
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

static inline __attribute__((always_inline)) unsigned long long int uRdtsc() {
	unsigned long long int result;
#if defined( __x86_64__ )
	asm volatile ( "rdtsc\n"
		   "salq $32, %%rdx\n"
		   "orq %%rdx, %%rax"
		   : "=a" (result) : : "rdx"
	);
#elif defined( __i386__ )
	asm volatile ( "rdtsc" : "=A" (result) );
#elif defined( __ARM_ARCH )
	// https://github.com/google/benchmark/blob/v1.1.0/src/cycleclock.h#L116
	asm volatile ( "mrs %0, cntvct_el0" : "=r" (result) );
	return result;
#else
	#error unsupported architecture
#endif
	return result;
} // uRdtsc


static inline __attribute__((always_inline)) void uFence() { // fence to prevent code movement
#if defined(__x86_64)
	//#define Fence() __asm__ __volatile__ ( "mfence" )
	__asm__ __volatile__ ( "lock; addq $0,(%%rsp);" ::: "cc" );
#elif defined(__i386)
	__asm__ __volatile__ ( "lock; addl $0,(%%esp);" ::: "cc" );
#elif defined( __ARM_ARCH )
	__asm__ __volatile__ ( "DMB ISH" ::: );
#else
	#error unsupported architecture
#endif
} // uFence


static inline __attribute__((always_inline)) void uPause() { // pause to prevent excess processor bus usage
#if defined( __i386 ) || defined( __x86_64 )
	__asm__ __volatile__ ( "pause" ::: );
#elif defined( __ARM_ARCH )
	__asm__ __volatile__ ( "YIELD" ::: );
#else
	#error unsupported architecture
#endif
} // uPause


template< typename T > static inline __attribute__((always_inline)) bool uAtomicLoad( volatile T & val, int memorder = __ATOMIC_SEQ_CST ) {
	return __atomic_load_n( &val, memorder );
} // uAtomicLoad

template< typename T > static inline __attribute__((always_inline)) bool uAtomicStore( volatile T & val, T replace, int memorder = __ATOMIC_SEQ_CST ) {
	return __atomic_store_n( &val, replace, memorder );
} // uAtomicStore


template< typename T > static inline __attribute__((always_inline)) bool uTestSet( volatile T & lock, int memorder = __ATOMIC_ACQUIRE ) {
	//return __sync_lock_test_and_set( &lock, 1 );
	return __atomic_test_and_set( &lock, memorder );
} // uTestSet

template< typename T > static inline __attribute__((always_inline)) void uTestReset( volatile T & lock, int memorder = __ATOMIC_RELEASE ) {
	//__sync_lock_release( &lock );
	__atomic_clear( &lock, memorder );
} // uTestReset


template< typename T > static inline __attribute__((always_inline)) T uFetchAssign( volatile T & assn, T replace, int memorder = __ATOMIC_ACQUIRE ) {
	//return __sync_lock_test_and_set( &assn, replace );
	return __atomic_exchange_n( &assn, replace, memorder );
} // uFetchAssign


template< typename T > static inline __attribute__((always_inline)) T uFetchAdd( volatile T & counter, int increment, int memorder = __ATOMIC_SEQ_CST ) {
	//return __sync_fetch_and_add( &counter, increment );
	return __atomic_fetch_add( &counter, increment, memorder );
} // uFetchAdd


template< typename T > static inline __attribute__((always_inline)) bool uCompareAssign( volatile T & assn, T comp, T replace, int memorder1, int memorder2 ) {
	return __atomic_compare_exchange_n( &assn, &comp, replace, false, memorder1, memorder2 );
} // uCompareAssign

template< typename T > static inline __attribute__((always_inline)) bool uCompareAssign( volatile T & assn, T comp, T replace ) {
	return uCompareAssign( assn, comp, replace, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST );
} // uCompareAssign

#ifdef __SIZEOF_INT128__
// Use __sync because __atomic with 128-bit CAA can result in calls to pthread_mutex_lock.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80878
template<> inline __attribute__((always_inline)) bool uCompareAssign( volatile __int128 & assn, __int128 comp, __int128 replace ) {
	return __sync_bool_compare_and_swap( &assn, comp, replace );
	// return __atomic_compare_exchange_n( &assn, &comp, replace, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST );
} // uCompareAssign
#endif // __SIZEOF_INT128__


template< typename T > static inline __attribute__((always_inline)) bool uCompareAssignValue( volatile T & assn, T & comp, T replace, int memorder1, int memorder2 ) {
	return __atomic_compare_exchange_n( &assn, &comp, replace, false, memorder1, memorder2 );
} // uCompareAssignValue

template< typename T > static inline __attribute__((always_inline)) bool uCompareAssignValue( volatile T & assn, T & comp, T replace ) {
	return uCompareAssignValue( assn, comp, replace, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST );
} // uCompareAssignValue

#ifdef __SIZEOF_INT128__
// Use __sync because __atomic with 128-bit CAA can result in calls to pthread_mutex_lock.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80878
template<> inline __attribute__((always_inline)) bool uCompareAssignValue( volatile __int128 & assn, __int128 & comp, __int128 replace ) {
	__int128 temp = comp;
	__int128 old = __sync_val_compare_and_swap( &assn, comp, replace );
	return old == temp ? true : ( comp = old, false );
	// return __atomic_compare_exchange_n( &assn, &comp, replace, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST );
} // uCompareAssignValue
#endif // __SIZEOF_INT128__


// Local Variables: //
// compile-command: "make install" //
// End: //
