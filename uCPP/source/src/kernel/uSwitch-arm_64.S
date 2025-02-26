// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2023
// 
// uSwitch-arm_64.S -- context switch for arm_64
// 
// Author           : Peter A. Buhr
// Created On       : Thu Mar 30 22:40:19 2023
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Aug 22 23:20:25 2024
// Update Count     : 7
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

// Context switch between coroutines/tasks.
//   void uSwitch( uMachContext::uContext_t * from, uMachContext::uContext_t * to );
// Arguments "from" in register x0, "to" in register x1.

	// Prevent warning message from GNU bin-utils V2.39 about executable stack.
	.section .note.GNU-stack

#define SAVE		20 * 8
	.file "uSwitch-arm_64.S"
	.text
	.align 2
	.global uSwitch
	.type uSwitch, %function
uSwitch:

	sub  sp, sp, #SAVE				// push stack

	// Save volatile GP registers x19-x30 on the stack.

	stp  x19, x20, [sp, #0x00]
	stp  x21, x22, [sp, #0x10]
	stp  x23, x24, [sp, #0x20]
	stp  x25, x26, [sp, #0x30]
	stp  x27, x28, [sp, #0x40]
	stp  x29, x30, [sp, #0x50]			// x29 => fp

	// Save volatile SIMD/FPU registers d8-d15 on the stack.

	stp  d8,  d9,  [sp, #0x60]
	stp  d10, d11, [sp, #0x70]
	stp  d12, d13, [sp, #0x80]
	stp  d14, d15, [sp, #0x90]

	// Save old context in the "from" area.

	mov  x4, sp					// cannot store sp directly
	str  x4, [x0, #SP_OFFSET]
	str  fp, [x0, #FP_OFFSET]

	// Load new context from the "to" area.

	ldr  fp, [x1, #FP_OFFSET]
	ldr  x4, [x1, #SP_OFFSET]
	mov  sp, x4					// cannot store sp directly

	// Load volatile GP registers x19-x30 from the stack.

	ldp  x19, x20, [sp, #0x00]
	ldp  x21, x22, [sp, #0x10]
	ldp  x23, x24, [sp, #0x20]
	ldp  x25, x26, [sp, #0x30]
	ldp  x27, x28, [sp, #0x40]
	ldp  x29, x30, [sp, #0x50]
	
	// Load volatile SIMD/FPU registers d8-d15 from the stack.

	ldp  d8,  d9,  [sp, #0x60]
	ldp  d10, d11, [sp, #0x70]
	ldp  d12, d13, [sp, #0x80]
	ldp  d14, d15, [sp, #0x90]

	add  sp, sp, #SAVE				// pop stack
	ret						// return to new thread (mov pc, x30)

	.size uSwitch, .-uSwitch
	.section .note.GNU-stack,"",%progbits // mark no executable stack needed

// Stub to create new stacks which can be context switched to
//   void __cfactx_invoke_stub( void );

	.text
	.align 2
	.global uInvokeStub
	.type uInvokeStub, %function
uInvokeStub:
	mov x0, x19					// load main as parameter 0
	mov x1, x20					// load this as parameter 1
	mov x30, x21					// load coroutine invoke routine
	ret						//   and jmp to it (mov pc, x30)
	.size uInvokeStub, .-uInvokeStub

// Local Variables: //
// mode: asm //
// comment-column: 56 //
// compile-command: "make install" //
// End: //
