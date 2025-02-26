//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2009
// 
// assert.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Thu Dec 10 20:40:07 2009
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 25 13:57:39 2023
// Update Count     : 54
// 

// This include file is not idempotent, so there is no guard.

extern "C" void __assert_fail( const char *__assertion, const char *__file, unsigned int __line, const char *__function )
	 __THROW __attribute__ ((__noreturn__));
extern "C" void __assert_perror_fail( int __errnum, const char *__file, unsigned int __line, const char *__function )
	 __THROW __attribute__ ((__noreturn__));
extern "C" void __assert( const char *__assertion, const char *__file, int __line )
	 __THROW __attribute__ ((__noreturn__));

#ifdef NDEBUG
	#define assert( expr ) ((void)0)
#else
	#include <stdlib.h>									// abort
	#include <unistd.h>									// STDERR_FILENO
	#include <uDebug.h>

	#define __STRINGIFY__(str) #str
	#define __VSTRINGIFY__(str) __STRINGIFY__(str)

	#define assert( expr ) \
	if ( ! ( expr ) ) { \
		int retcode __attribute__(( unused )); \
		uDebugWrite( STDERR_FILENO, __FILE__ ":" __VSTRINGIFY__(__LINE__) ": Assertion \"" __VSTRINGIFY__(expr) "\" failed.\n", \
			sizeof( __FILE__ ":" __VSTRINGIFY__(__LINE__) ": Assertion \"" __VSTRINGIFY__(expr) "\" failed.\n" ) - 1 );	\
		abort(); \
	}
#endif // NDEBUG


// Local Variables: //
// compile-command: "make install" //
// End: //
