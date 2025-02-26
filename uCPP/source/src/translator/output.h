//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// output.h --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:41:53 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Jan 21 09:10:22 2019
// Update Count     : 21
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


#include <token.h>

extern token_t * file_token;
extern char file[];
extern unsigned int line, flag;
extern const char * flags[];
void parse_directive( char * text, char file[], unsigned int & line, unsigned int & flag );

void write_all_output();



// Local Variables: //
// compile-command: "make install" //
// End: //
