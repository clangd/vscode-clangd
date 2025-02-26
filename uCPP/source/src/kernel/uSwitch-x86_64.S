// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uSwitch-x64.S -- context switch for x86_64
// 
// Author           : Richard C. Bilson
// Created On       : Mon Feb 28 13:36:42 2005
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Aug 20 07:37:54 2024
// Update Count     : 31
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

// The context switch routine requires the initial the stack of a thread to
// look like the thread has saved its context in the normal manner.

// Offsets must synchronized with the uMachContext::uContext_t in uC++.h.

#define PTR_BYTE	8
#define SP_OFFSET	( 0 * PTR_BYTE )
#define FP_OFFSET	( 1 * PTR_BYTE )
//#define PC_OFFSET	( 2 * PTR_BYTE ) debugger

// Context switch between coroutines/tasks.
//   void uSwitch( uMachContext::uContext_t * from, uMachContext::uContext_t * to );
// Arguments "from" in register rdi, "to" in register rsi.

	// Prevent warning message from GNU bin-utils V2.39 about executable stack.
	.section .note.GNU-stack

	.file "uSwitch-x64.S"
	.text
	.align 2
	.global uSwitch
	.type uSwitch, @function
uSwitch:

	// Save volatile registers on the stack.

	subq $8,%rsp
	stmxcsr 0(%rsp)					// 4 bytes
	fnstcw 4(%rsp)					// 2 bytes
	pushq %r15
	pushq %r14
	pushq %r13
	pushq %r12
	pushq %rbx

	// Save old context in the "from" area.

	movq %rsp,SP_OFFSET(%rdi)
	movq %rbp,FP_OFFSET(%rdi)

	// Load new context from the "to" area.

	movq SP_OFFSET(%rsi),%rsp
	movq FP_OFFSET(%rsi),%rbp

	// Load volatile registers from the stack.

	popq %rbx
	popq %r12
	popq %r13
	popq %r14
	popq %r15
	fldcw 4(%rsp)
	ldmxcsr 0(%rsp)
	addq $8,%rsp

	// Return to thread.

	ret
	.size uSwitch, .-uSwitch

// Stub to create new stacks which can be context switched to
//   void uInvokeStub( uMachContext * );

	.text
	.align 2
	.global uInvokeStub
	.type uInvokeStub, @function
uInvokeStub:
	movq %rbx, %rdi					// move this to first argument
	jmp *%r12					// jmp to invoke

// Local Variables: //
// mode: asm //
// comment-column: 56 //
// compile-command: "make install" //
// End: //
