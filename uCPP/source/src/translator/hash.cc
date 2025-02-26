//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// hash.c --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:04:17 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Dec 24 17:25:23 2021
// Update Count     : 64
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

#include "hash.h"
#include <cstring>

// should be a prime number
#define HASH_MAGIC_VALUE 1048583


hash_t::hash_t( const char *t, hash_t *l, int v ) {
	text = new char[strlen( t ) + 1];
	strcpy( text, t );
	link = l;
	value = v;
	InSymbolTable = 0;
} // hash_t::hash_t


hash_t::~hash_t() {
	delete [] text;
} // hash_t::~hash_t


hash_table_t::hash_table_t() {
	for ( int i = 0; i < HASH_TABLE_SIZE; i += 1 ) {
		table[i] = nullptr;
	} // for
} // hash_table_t::hash_table_t


hash_table_t::~hash_table_t() {
	for ( int i = 0; i < HASH_TABLE_SIZE; i += 1 ) {
		hash_t *hash = table[i];
		while ( hash != nullptr ) {
			hash_t *temp = hash;
			hash = hash->link;
			delete temp;
		} // while
	} // for
} // hash_table_t::~hash_table_t


hash_t *hash_table_t::lookup( const char *text, int value ) {
	unsigned int key;
	const char *cp;

	for ( key = 0, cp = text; *cp != '\0'; cp += 1 ) {
		key += (int)*cp;
	} // for
	key *= HASH_MAGIC_VALUE;
	key = key % HASH_TABLE_SIZE;

	// if matching entry found in the hash table, return pointer to entry

	hash_t *hash;
	for ( hash = table[ key ]; hash != nullptr; hash = hash->link ) {
		if ( strcmp( hash->text, text ) == 0 ) return hash;
	} // for

	// if matching entry not found, create a new hash table entry, and insert it into hash table

	hash = new hash_t( text, table[key], value );
	table[key] = hash;

	return hash;
} // hash_table::lookup


// declare an instance of a hash table

hash_table_t *hash_table;

// Local Variables: //
// compile-command: "make install" //
// End: //
