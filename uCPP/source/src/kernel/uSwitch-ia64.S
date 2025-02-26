//                               -*- Mode: Asm -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2002
// 
// uSwitch-ia64.s -- context switch routine for Itanium processors
// 
// Author           : Peter A. Buhr
// Created On       : Sat Jul  6 12:50:40 2002
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu May  5 14:55:12 2005
// Update Count     : 29
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

// This routine is adapted from the GNU C Library implementations of getcontext
// and setcontext, written by David Mosberger-Tang.

// Itanium Processor Architecture Reference Vol. 2, section 4.5.1.2, describes
// the rationale.

// This context switch routine depends on the fact that the stack of a new
// thread has been set up to look like the thread has saved its context in
// the normal manner.
//
// void uSwitch( uMachContext *from, uMachContext *to );

#define __ASSEMBLY__

#include <asm/unistd.h>
#include <asm/signal.h>					// SIG_BLOCK, SIG_SETMASK

#define rTMP	r16
#define rPOS	r16
#define rCPOS	r17
#define rNAT	r18

#define rB5	r18
#define rB4	r19
#define rB3	r20
#define rB2	r21
#define rB1	r22
#define rB0	r23
#define rRSC	r24
#define rBSP	r25
#define rRNAT	r26
#define rUNAT	r27
#define rFPSR	r28
#define rPFS	r29
#define rLC	r30
#define rPR	r31

#define CONTEXT_SIZE	144

	.file	"uSwitch-ia64.S"
	.pred.safe_across_calls p1-p5,p16-p63
.text
	.align 16
	.global uSwitch#
	.proc uSwitch#
uSwitch:
	.prologue
	alloc r16 = ar.pfs, 2, 0, 4, 0

	invala

	// sigprocmask (SIG_BLOCK, NULL, &sc->sc_mask):

	mov r15 = __NR_rt_sigprocmask
	;;
	mov out0 = SIG_SETMASK
	add out1 = 16, in1
	add out2 = 16, in0
	mov out3 = 8					// sizeof kernel sigset_t

	break __BREAK_SYSCALL
	flushrs						// save dirty partition on rbs
	add r2 = -8, sp

	mov.m rFPSR = ar.fpsr
	mov.m rRSC = ar.rsc
	add r3 = -16, sp
	;;
	mov.m rBSP = ar.bsp
	.save ar.unat, rUNAT
	mov.m rUNAT = ar.unat
	.body
	add sp = - CONTEXT_SIZE - 16, sp
	;;

.mem.offset 8,0; st8.spill [r32] = sp, 8
.mem.offset 0,0; st8.spill [r2] = r1, -16
	mov.i rPFS = ar.pfs
	;;
.mem.offset 8,0; st8.spill [r3] = r4, -16
.mem.offset 0,0; st8.spill [r2] = r5, -16
	and rTMP = ~0x3, rRSC
	;;
.mem.offset 8,0; st8.spill [r3] = r6, -16
.mem.offset 0,0; st8.spill [r2] = r7, -16
	;;
	mov.m ar.rsc = rTMP				// put RSE into enforced lazy mode
	mov.m rNAT = ar.unat
	mov.i rLC = ar.lc
	;;
	ld8 sp = [r33], 8
	mov.m rRNAT = ar.rnat
	mov rPR = pr
	;;
	st8 [r32] = rBSP
	st8 [r3] = rRNAT, -16
	mov rB0 = b0
	;;
	st8 [r2] = rNAT, -16
	st8 [r3] = rUNAT, -16
	mov rB1 = b1
	;;
	st8 [r2] = rFPSR, -16
	st8 [r3] = rPFS, -16
	mov rB2 = b2
	;;
	st8 [r2] = rLC, -16
	st8 [r3] = rPR, -16
	mov rB3 = b3
	;;
	st8 [r2] = rB0, -16
	st8 [r3] = rB1, -16
	mov rB4 = b4
	;;
	st8 [r2] = rB2, -16
	st8 [r3] = rB3, -16
	mov rB5 = b5
	;;
	st8 [r2] = rB4
	st8 [r3] = rB5
	;;
	ld8 rBSP = [r33]
	add r2 = 24, sp
	add r3 = 16, sp
	;;
	ld8 rB5 = [r3], 16
	ld8 rB4 = [r2], 16
	;;
	ld8 rB3 = [r3], 16
	ld8 rB2 = [r2], 16
	mov b5 = rB5
	;;
	ld8 rB1 = [r3], 16
	ld8 rB0 = [r2], 16
	mov b4 = rB4
	;;
	ld8 rPR = [r3], 16
	ld8 rLC = [r2], 16
	mov b3 = rB3
	;;
	ld8 rPFS = [r3], 16
	ld8 rFPSR = [r2], 16
	mov b2 = rB2
	;;
	ld8 rUNAT = [r3], 16
	ld8 rNAT = [r2], 16
	mov b1 = rB1
	;;
	ld8 rRNAT = [r3], 16
	mov ar.unat = rNAT
	mov b0 = rB0
	;;
	ld8.fill r7 = [r2], 16
	ld8.fill r6 = [r3], 16
	;;
	ld8.fill r5 = [r2], 16
	ld8.fill r4 = [r3]
	mov pr = rPR, -1
	;;
	ld8.fill r1 = [r2]
	mov.m ar.fpsr = rFPSR
	add sp = CONTEXT_SIZE + 16, sp
	;;
	loadrs				// drop dirty partition
	mov.i ar.pfs = rPFS
	;;
	mov.m ar.bspstore = rBSP
	mov.m ar.unat = rUNAT
	mov.i ar.lc = rLC
	;;
	mov.m ar.rnat = rRNAT
	mov.m ar.rsc = rRSC
	
	br.ret.sptk.few b0;;

	.endp uSwitch#

.text
	.align 16
	.global uInvokeStub#
	.proc uInvokeStub#
uInvokeStub:
	.prologue
	.altrp b2
	alloc r16 = ar.pfs, 0, 0, 1, 0
	mov r32 = r4
	br.cond.sptk.many b1;;

	.endp uInvokeStub#

// void uIA64FPsave( double cxt[] );

	.file	"uFloat-ia64.S"
	.pred.safe_across_calls p1-p5,p16-p63
.text
	.align 16
	.global uIA64FPsave#
	.proc uIA64FPsave#
uIA64FPsave:
	.prologue
	alloc r16 = ar.pfs, 1, 0, 0, 0
	mov r2 = r32
	add r3 = 16, r32
	;;
	stf.spill [r2] = f2, 32
	stf.spill [r3] = f3, 32
	;;
	stf.spill [r2] = f4, 32
	stf.spill [r3] = f5, 32
	;;
	stf.spill [r2] = f16, 32
	stf.spill [r3] = f17, 32
	;;
	stf.spill [r2] = f18, 32
	stf.spill [r3] = f19, 32
	;;
	stf.spill [r2] = f20, 32
	stf.spill [r3] = f21, 32
	;;
	stf.spill [r2] = f22, 32
	stf.spill [r3] = f23, 32
	;;
	stf.spill [r2] = f24, 32
	stf.spill [r3] = f25, 32
	;;
	stf.spill [r2] = f26, 32
	stf.spill [r3] = f27, 32
	;;
	stf.spill [r2] = f28, 32
	stf.spill [r3] = f29, 32
	;;
	stf.spill [r2] = f30, 32
	stf.spill [r3] = f31, 32
	;;
	br.ret.sptk.few b0
	;;
	.endp uIA64FPsave#
	
// void uIA64FPrestore( double cxt[] );

.text
	.align 16
	.global uIA64FPrestore#
	.proc uIA64FPrestore#
uIA64FPrestore:
	.prologue
	alloc r16 = ar.pfs, 1, 0, 0, 0
	mov r2 = r32
	add r3 = 16, r32
	;;
	ldf.fill f2 = [r2], 32
	ldf.fill f3 = [r3], 32
	;;
	ldf.fill f4 = [r2], 32
	ldf.fill f5 = [r3], 32
	;;
	ldf.fill f16 = [r2], 32
	ldf.fill f17 = [r3], 32
	;;
	ldf.fill f18 = [r2], 32
	ldf.fill f19 = [r3], 32
	;;
	ldf.fill f20 = [r2], 32
	ldf.fill f21 = [r3], 32
	;;
	ldf.fill f22 = [r2], 32
	ldf.fill f23 = [r3], 32
	;;
	ldf.fill f24 = [r2], 32
	ldf.fill f25 = [r3], 32
	;;
	ldf.fill f26 = [r2], 32
	ldf.fill f27 = [r3], 32
	;;
	ldf.fill f28 = [r2], 32
	ldf.fill f29 = [r3], 32
	;;
	ldf.fill f30 = [r2], 32
	ldf.fill f31 = [r3], 32
	;;
	br.ret.sptk.few b0
	;;
	.endp uIA64FPrestore#

// Local Variables: //
// compile-command: "make install" //
// End: //
