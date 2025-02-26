//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// hash.h --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:37:36 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Dec 24 17:25:54 2021
// Update Count     : 33
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


// should be a prime number
#define HASH_TABLE_SIZE 1009


class hash_t {
	friend class hash_table_t;
  public:
	char * text;
	hash_t * link;
	int value;
	int InSymbolTable;
  public:
	hash_t( const char *, hash_t *, int value );
	~hash_t();
};


class hash_table_t {
  private:
	hash_t * table[HASH_TABLE_SIZE];
  public:
	hash_table_t();
	~hash_table_t();
	hash_t * lookup( const char *, int value = 0 );
};

extern hash_table_t * hash_table;



// Local Variables: //
// compile-command: "make install" //
// End: //
