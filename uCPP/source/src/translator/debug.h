//                               -*- Mode: C -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
// 
// debug.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Feb 20 06:53:20 2017
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Jan 21 09:19:02 2019
// Update Count     : 3
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


#ifdef __U_DEBUG_H__
#define uDEBUGPRT( stmt ) stmt
#else
#define uDEBUGPRT( stmt )
#endif // __U_DEBUG_H__


// Local Variables: //
// compile-command: "make install" //
// End: //
