//                               -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1997
// 
// uDefault.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Thu Mar 20 18:12:31 1997
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Jan  3 09:49:26 2025
// Update Count     : 86
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


// Define the default scheduling pre-emption time in milliseconds.  A scheduling pre-emption is attempted every default
// pre-emption milliseconds.  A pre-emption does not occur if the executing task is not in user code or the task is
// currently in a critical section.  A critical section begins when a task acquires a lock and ends when a user releases
// a lock.

enum : unsigned int { __U_DEFAULT_PREEMPTION__ = 10 };


// Define the default spin time in units of checks and context switches. The idle task checks the ready queue and
// context switches this many times before the UNIX process executing the idle task goes to sleep.

enum : unsigned int { __U_DEFAULT_SPIN__ = 1000 };


// Define the default stack size in bytes.  Change the implicit default stack size for a task or coroutine created on a
// particular cluster.

//enum { __U_DEFAULT_STACK_SIZE__ = 30000 };
enum  : unsigned int{ __U_DEFAULT_STACK_SIZE__ = 250 * 1000 };

// often large automatic arrays for setting up the program
enum : unsigned int { __U_DEFAULT_MAIN_STACK_SIZE__ = 500 * 1000 };


// Define the default number of processors created on the user cluster. Must be greater than 0.

enum : unsigned int { __U_DEFAULT_USER_PROCESSORS__ = 1 };


extern unsigned int uDefaultStackSize();				// cluster coroutine/task stack size (bytes)
extern unsigned int uMainStackSize();					// uMain task stack size (bytes)
extern unsigned int uDefaultSpin();						// processor spin time for idle task (context switches)
extern unsigned int uDefaultPreemption();				// processor scheduling pre-emption durations (milliseconds)
extern unsigned int uDefaultProcessors();				// number of processors created on the user cluster
extern unsigned int uDefaultBlockingIOProcessors();		// number of blocking I/O processors created on the blocking I/O cluster

extern void uStatistics();								// print user defined statistics on interrupt


// Local Variables: //
// compile-command: "make install" //
// End: //
