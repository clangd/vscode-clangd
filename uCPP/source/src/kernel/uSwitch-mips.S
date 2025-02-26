//                               -*- Mode: Asm -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uSwitch-mip.s -- 
// 
// Author           : Peter A. Buhr
// Created On       : Wed Nov  9 10:16:30 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Apr  3 10:49:28 2022
// Update Count     : 30
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

// This context switch routine depends on the fact that the stack of a new
// thread has been set up to look like the thread has saved its context in
// the normal manner.
//
// void uSwitch( void *from, void *to );

	.text
	.align 2
	.globl uSwitch
	.ent uSwitch
uSwitch:
	.frame	$sp,0,$31
	.mask	0x00000000,0
	.fmask	0x00000000,0

	// Save volatile registers (16-23,30,31,gp) on the stack.

	dsubu $sp,96				// multiple of 16
	sd $16,0($sp)
	sd $17,8($sp)
	sd $18,16($sp)
	sd $19,24($sp)
	sd $20,32($sp)
	sd $21,40($sp)
	sd $22,48($sp)
	sd $23,56($sp)
	sd $30,64($sp)
	sd $31,72($sp)
	sd $gp,80($sp)

#ifdef _ABIN32
	// Save stack pointer to the "from" area at offset 0.

	sw $sp,0($4)

	// Load stack pointer from the "to" area at offset 0.

	lw $sp,0($5)
#elif _ABI64
	// Save stack pointer to the "from" area at offset 0.

	sd $sp,0($4)

	// Load stack pointer from the "to" area at offset 0.

	ld $sp,0($5)
#else
	#error uC++ : internal error, unsupported architecture
#endif
	// Load volatile registers (16-23,30,31,gp) from the stack.

	ld $gp,80($sp)
	ld $31,72($sp)
	ld $30,64($sp)
	ld $23,56($sp)
	ld $22,48($sp)
	ld $21,40($sp)
	ld $20,32($sp)
	ld $19,24($sp)
	ld $18,16($sp)
	ld $17,8($sp)
	ld $16,0($sp)
	daddu $sp,96

	// Return to thread.

	j $31

// define end of routine

 	.end	uSwitch


// Routine stub used for first call to uInvoke

	.text
	.align 2
	.globl uInvokeStub
	.ent uInvokeStub
uInvokeStub:
	
	// move uInvoke argument into first argument register
	
	move $4,$16
	
	// $25 (t9) must contain uInvoke address: used by function prologue
	// to calculate gp
	
	move $25,$17
	j $25
	
 	.end	uInvokeStub


// Local Variables: //
// compile-command: "make install" //
// End: //
