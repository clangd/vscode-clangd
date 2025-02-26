//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// symbol.h --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:47:21 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Dec 24 18:14:15 2021
// Update Count     : 75
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


#include "attribute.h"
#include <list>

class hash_t;
struct table_t;
struct token_t;

const unsigned int TIMEOUTPOSN = 0;						  // bit 0 is reserved for timeout
const unsigned int DESTRUCTORPOSN = 1;					  // bit 1 is reserved for destructor
const unsigned int MAXENTRYBITS = __U_MAXENTRYBITS__ - 1; // N mutex members including destructor

struct symbol_data_t {
	table_t * found;									// parent (back pointer) symbol table (where defined)
	table_t * table;									// nested symbol table for symbols defining scope
	symbol_t * base;									// special base class if single inheritance of mutex, coroutine, task, exception
	std::list<symbol_t *> base_list;					// list of all base classes (including special base class) if inheritance

	unsigned int key;									// kind of class, e.g., class, coroutine, task, exception, routine, member
	attribute_t attribute;								// attribute information for a declaration
	unsigned int index;									// for mutex type: number of mutex members in this type
														// for mutex member: position of mutex member in mutex bit mask
	bool used;											// indicates if explicit call to constructor in initializer list
	token_t * base_token;								// location to insert implicit base class (e.g., uBaseTask)
	token_t * left;										// start of base_specifier
	token_t * right;									// end of base_specifier

	symbol_data_t();
};

struct symbol_t {
	int value;											// symbol is an identifier or operator or type
	hash_t * hash;										// name of symbol in hash table
	symbol_data_t * data;								// shared data for typedef
	bool copied;										// mark if "data" copied for typedef
	bool typname;										// mark if typename

	symbol_t( int v, hash_t * h );
	symbol_t( const symbol_t & );
	~symbol_t();
};


// Local Variables: //
// compile-command: "make install" //
// End: //
