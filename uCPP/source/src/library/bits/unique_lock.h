//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2023
// 
// unique_lock.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Wed Jan  4 11:37:42 2023
// Last Modified By : Peter A. Buhr
// Last Modified On : Wed Jan  4 15:58:32 2023
// Update Count     : 2
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

// This include file uses the uC++ keyword _Mutex as a template parameter name.
// The name is changed only for the include file.

#if ! defined( _Mutex )									// nesting ?
#define _Mutex Mutex_									// make keyword an identifier
#define __U_UNIQUE_LOCK_H__
#endif

#include_next <bits/unique_lock.h>

#if defined( _Mutex ) && defined( __U_UNIQUE_LOCK_H__ )	// reset only if set
#undef __U_UNIQUE_LOCK_H__
#undef _Mutex
#endif

// Local Variables: //
// compile-command: "make install" //
// End: //
