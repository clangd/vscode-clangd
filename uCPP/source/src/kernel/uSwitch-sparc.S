!                               -*- Mode: Asm -*- 
! 
! uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
! 
! uSwitch-sparc.s -- 
! 
! Author           : Peter A. Buhr
! Created On       : Tue Mar  1 14:42:19 1994
! Last Modified By : Peter A. Buhr
! Last Modified On : Sun Apr  3 10:50:06 2022
! Update Count     : 44
!
! This  library is free  software; you  can redistribute  it and/or  modify it
! under the terms of the GNU Lesser General Public License as published by the
! Free Software  Foundation; either  version 2.1 of  the License, or  (at your
! option) any later version.
! 
! This library is distributed in the  hope that it will be useful, but WITHOUT
! ANY  WARRANTY;  without even  the  implied  warranty  of MERCHANTABILITY  or
! FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
! for more details.
! 
! You should  have received a  copy of the  GNU Lesser General  Public License
! along  with this library.
! 

! include some machine dependent values

#include <sys/asm_linkage.h>
#include <sys/isa_defs.h>

! Offsets in the context structure. This needs to be synchronized with the
! high level code a little better.

#if __U_WORDSIZE__ == 32
#   define PTR_BYTE	4
#   define STORE_OP	stw
#   define LOAD_OP	lduw
#elif __U_WORDSIZE__ == 64
#   define PTR_BYTE	8
#   define STORE_OP	stx
#   define LOAD_OP	ldx
#else
	#error uC++ : internal error, unsupported ABI data model
#endif

#define SP_OFFSET	( 0 * PTR_BYTE )
#define FP_OFFSET	( 1 * PTR_BYTE )
#define PC_OFFSET	( 2 * PTR_BYTE )

! This context switch routine depends on the fact that the stack of a new
! thread has been set up to look like the thread has saved its context in
! the normal manner.
!
! void uSwitch( void *from, void *to );

	.file	"uSwitch-sparc.s"
	.section ".text"
	.align	4
	.global	uSwitch
	.type	uSwitch,#function
	.proc	4
uSwitch:
	! Establish a minimal frame.

	save %sp,-SA(MINFRAME),%sp

	! Save all stack window registers.

	!ta ST_FLUSH_WINDOWS
	flushw

	! Save old context in the "from" area.

	STORE_OP %sp,[%i0+SP_OFFSET]
	STORE_OP %fp,[%i0+FP_OFFSET]
	STORE_OP %i7,[%i0+PC_OFFSET]

	! Load new context from the "to" area.

	LOAD_OP [%i1+PC_OFFSET],%i7
	LOAD_OP [%i1+FP_OFFSET],%fp
	LOAD_OP [%i1+SP_OFFSET],%sp

	! Return to thread.

	ret

	! Pop back one register frame, causing the in and local registers
	! to be loaded from memory.

	restore
	.size	uSwitch,(.-uSwitch)


! Routine stub used for first call to uInvoke
!
! void uInvokeStub( uMachContext * );

	.file	"uInvokeStub-sparc.s"
	.section ".text"
	.align	4
	.global	uInvokeStub
	.type	uInvokeStub,#function
	.proc	4
uInvokeStub:
	jmpl	%i1,%r0
	mov	%i0,%o0		! delay slot
	.size	uInvokeStub,(.-uInvokeStub)

!! Local Variables: !!
!! compile-command: "make install" !!
!! End: !!
