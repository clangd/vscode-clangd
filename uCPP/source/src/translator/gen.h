//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// gen.h --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:36:43 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Dec 24 17:22:53 2021
// Update Count     : 85
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


#include "key.h"

struct token_t;
struct symbol_t;
class hash_t;

token_t * gen_code( token_t * before, const char text );
token_t * gen_code( token_t * before, const char * text, int value = CODE );
token_t * gen_error( token_t * before, const char * text );
token_t * gen_warning( token_t * before, const char * text );

void gen_wait_prefix( token_t * before, token_t * after );
void gen_with( token_t * before, token_t * after );
void gen_wait_suffix( token_t * before );
void gen_base_clause( token_t * before, symbol_t * symbol );
void gen_member_prefix( token_t * before, symbol_t * symbol );
void gen_member_suffix( token_t * before );
void gen_main_prefix( token_t * before, symbol_t * symbol );
void gen_main_suffix( token_t * before, symbol_t * symbol );
void gen_initializer( token_t * before, symbol_t * symbol, char prefix, bool after );
void gen_serial_initializer( token_t * rp, token_t * end, token_t * before, symbol_t * symbol );
void gen_constructor_parameter( token_t * before, symbol_t * symbol, bool defarg );
void gen_constructor_prefix( token_t * before, symbol_t * symbol );
void gen_constructor_suffix( token_t * before, symbol_t * symbol );
void gen_constructor( token_t * before, symbol_t * symbol );
void gen_destructor_prefix( token_t * before, symbol_t * symbol );
void gen_destructor_suffix( token_t * before, symbol_t * symbol );
void gen_destructor( token_t * before, symbol_t * symbol );
void gen_class_prefix( token_t * before, symbol_t * symbol );
void gen_class_suffix( symbol_t * symbol );
void gen_access( token_t * before, int access );
void gen_PIQ( token_t * before, symbol_t * symbol );
void gen_mutex( token_t * before, symbol_t * symbol );
void gen_mutex_entry( token_t * before, symbol_t * symbol );
void gen_base_specifier_name( token_t * before, symbol_t * symbol );
void gen_entry( token_t * before, unsigned int );
void gen_mask( token_t * before, unsigned int );
void gen_hash( token_t * before, hash_t * hash );



// Local Variables: //
// compile-command: "make install" //
// End: //
