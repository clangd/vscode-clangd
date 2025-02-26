// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uSwitch-i386.S -- context switch for x86
// 
// Author           : Peter A. Buhr
// Created On       : Sun Oct  9 11:46:38 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Aug 22 23:09:59 2024
// Update Count     : 39
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

#define PTR_BYTE	4
#define SP_OFFSET	( 0 * PTR_BYTE )
#define FP_OFFSET	( 1 * PTR_BYTE )
//#define PC_OFFSET	( 2 * PTR_BYTE ) debugger

// Context switch between coroutines/tasks.
//   void uSwitch( uMachContext::uContext_t * from, uMachContext::uContext_t * to );
// Arguments "from" in register 4(%esp), "to" in register 28(%esp)

	// Prevent warning message from GNU bin-utils V2.39 about executable stack.
	.section .note.GNU-stack

	.file "uSwitch-i386.S"
	.text
	.align 2
	.global uSwitch
	.type uSwitch, @function
uSwitch:

	// Copy the "from" context argument from the stack to register eax
	// Return address is at 0(%esp), with parameters following.

	movl 4(%esp),%eax

	// Save volatile registers on the stack.

	sub $8,%esp
	stmxcsr 0(%esp)		// 4 bytes
	fnstcw 4(%esp)		// 2 bytes
	pushl %ebx
	pushl %edi
	pushl %esi

	// Save old context in the "from" area.

	movl %esp,SP_OFFSET(%eax)
	movl %ebp,FP_OFFSET(%eax)
//	movl 4(%ebp),%ebx	// save previous eip for debugger
//	movl %ebx,PC_OFFSET(%eax)

	// Copy the "to" context argument from the stack to register eax. Having
	// pushed 5 words (= 20 bytes) on the stack, the argument is now at
	// 8 + 20 = 28(%esp).

	movl 28(%esp),%eax

	// Load new context from the "to" area.

	movl SP_OFFSET(%eax),%esp
	movl FP_OFFSET(%eax),%ebp

	// Load volatile registers from the stack.

	popl %esi
	popl %edi
	popl %ebx
	fldcw 4(%esp)
	ldmxcsr 0(%esp)
	add $8,%esp

	// Return to thread.

	ret
	.size uSwitch, .-uSwitch

// Local Variables: //
// compile-command: "make install" //
// End: //
