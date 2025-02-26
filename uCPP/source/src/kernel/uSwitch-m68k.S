#                               -*- Mode: Asm -*- 
# 
# uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1995
# 
# uSwitch-m68k.s -- 
# 
# Author           : Peter A. Buhr
# Created On       : Mon Jan  2 20:55:37 1995
# Last Modified By : Peter A. Buhr
# Last Modified On : Mon Jul 21 21:17:54 2003
# Update Count     : 13
#
# This  library is free  software; you  can redistribute  it and/or  modify it
# under the terms of the GNU Lesser General Public License as published by the
# Free Software  Foundation; either  version 2.1 of  the License, or  (at your
# option) any later version.
# 
# This library is distributed in the  hope that it will be useful, but WITHOUT
# ANY  WARRANTY;  without even  the  implied  warranty  of MERCHANTABILITY  or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
# for more details.
# 
# You should  have received a  copy of the  GNU Lesser General  Public License
# along  with this library.
# 

# This context switch routine depends on the fact that the stack of a new
# thread has been set up to look like the thread has saved its context in
# the normal manner.
#
# void uSwitch( uMachContext *to, uMachContext *from );

.text
	.even
.globl	uSwitch
uSwitch:

	| Copy the "from" context argument from the stack to register a0

	movel sp@(8),a0

	| Save volatile registers (d2-d7,a2-a5,fp) on the stack.

	moveml #0x3f3e,sp@-

	| Save stack pointer in the "from" area at offset 0.

	movel sp,a0@

	| Copy the "to" context argument from the stack to register a0

	movel sp@(48),a0

	| Load stack pointer from the "to" area at offset 0.

	movel a0@,sp

	| Load volatile registers (d2-d7,a2-a5,fp) from the stack.

	moveml sp@+,#0x7cfc

	| Return to thread.

	rts

## Local Variables: ##
## compile-command: "make install" ##
## End: ##
