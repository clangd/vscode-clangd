//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// symbol.c --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:18:22 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Dec 24 18:13:35 2021
// Update Count     : 77
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

#include "table.h"
#include "symbol.h"

#include "hash.h"
#include <iostream>
using std::cerr;
using std::endl;

//#define __U_DEBUG_H__
#include "debug.h"

symbol_data_t::symbol_data_t() {
	found = nullptr;
	table = nullptr;
	key = 0;
	index = DESTRUCTORPOSN;								// start allocating entry bits from this value
	base = nullptr;
	used = false;
	base_token = nullptr;
	left = right = nullptr;
} // symbol_data_t::symbol_data_t


symbol_t::symbol_t( int v, hash_t * h ) {
	value = v;
	hash = h;
	copied = typname = false;
	data = new symbol_data_t;
	uDEBUGPRT(cerr << "add1 symbol " << hash->text << endl; );
} // symbol_t::symbol_t


symbol_t::symbol_t( const symbol_t &other ) {
	value = other.value;
	hash = other.hash;
	copied = false;
	data = new symbol_data_t;
	uDEBUGPRT( cerr << "add2 symbol " << hash->text << endl; );
} // symbol_t::symbol_t


symbol_t::~symbol_t() {
	uDEBUGPRT( cerr << "delete symbol " << hash->text << endl; );
	if ( ! copied ) {									// data shared for typedef and using
		delete data->table;
		delete data;
		data = nullptr;
	} // if
	copied = false;
} // symbol_t::~symbol_t

// Local Variables: //
// compile-command: "make install" //
// End: //
