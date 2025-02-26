//                              -*- Mode: C++ -*-
// 
// uC++ Version 7.0.0, Copyright (C) Martin Karsten 1995
// 
// ArchBasics.h -- 
// 
// Author           : Martin Karsten
// Created On       : Sat May 13 14:54:59 1995
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Jan 20 11:30:58 2019
// Update Count     : 17
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

#ifndef _ArchBasics_h_
#define _ArchBasics_h_ 1

#if defined( __i386__ )

#include <sys/ptrace.h>
#define NUM_REGS 16			// from gdb/config/i386/tm-i386v.h
typedef int				prgregset_t[NUM_REGS+1]; // to hold ORIG_EAX
typedef int				prgreg_t;
#define R_SP  UESP				/* Contains address of top of stack */
#define R_PC  EIP
#define R_FP  EBP				/* Contains address of executing stack frame */

typedef long*		CodeAddress;
typedef char		Instruction;
typedef long*		DataAddress;
typedef long		DataField;
typedef int			Register;
typedef char*		InternalAddress;

struct MinimalRegisterSet {
	Register	sp;
	Register	fp;
	Register	pc;
}; // struct MinimalRegisterSet

#define CREATE_MINIMAL_REGISTER_SET(regs)\
	asm(" movl %%esp,%0" : "=m" (regs.sp) : );\
	asm(" movl %%ebp,%0" : "=m" (regs.fp) : );\
	asm(" movl 4(%ebp),%eax");\
	asm(" movl %%eax,%0" : "=m" (regs.pc) : );

#else /* any not supported architecture */

typedef long*		CodeAddress;
typedef long		Instruction;
typedef long*		DataAddress;
typedef long		DataField;
typedef int			Register;
typedef char*		InternalAddress;

struct MinimalRegisterSet {
	Register	sp;
	Register	fp;
	Register	pc;
}; // struct MinimalRegisterSet

#define CREATE_MINIMAL_REGISTER_SET(regs)\
	regs.sp = 0;\
	regs.fp = 0;\
	regs.pc = 0;

#endif // __i386__

// once for all
#include <string.h>

// same for all UNIXs ?
typedef	uPid_t			OSKernelThreadId;

#endif // _ArchBasics_h_


// Local Variables: //
// tab-width: 4 //
// End: //
