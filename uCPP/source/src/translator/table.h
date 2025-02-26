//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// table.h --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:59:11 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Dec 24 18:20:17 2021
// Update Count     : 70
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


struct table_t;

#include "symbol.h"
#include "structor.h"

struct local_t {
	bool useing;										// true => entry is for a "using" statment
	bool tblsym;										// true => table_t, false => symbol_t
	union {
		table_t * tbl;
		symbol_t * sym;
	} kind;
	local_t * link;										// next stack element
};

struct lexical_t {
	table_t * tbl;
	lexical_t * link;									// next stack element

	lexical_t( table_t * tbl ) {
		lexical_t::tbl = tbl;
		link = nullptr;
	}
};

struct table_t {
	local_t * local;									// list of local tables/symbols defined at this scope level
	table_t * lexical;									// nested lexical scopes
	symbol_t * symbol;									// symbol (back pointer) that owns this nested scope

	bool useing;										// true => "using" entries in local table/symbol list
	unsigned int access;								// current member access (PRIVATE,PROTECTED,PUBLIC); changes as access clauses are parsed
	structor_list_t constructor;						// list of constructors for class
	structor_list_t destructor;							// list of destructors for class
	bool defined;										// class body defined (i.e., not just prototype)
	bool hascopy;										// has copy constructor
	bool hasassnop;										// has assignment operator
	bool hasdefault;									// has a default constructor
	token_t * private_area;
	token_t * protected_area;
	token_t * public_area;

	// template parameters AND types are combined into a single list for name lookup in stack order
	local_t * startT;									// first template parameter (stack base)
	local_t * endT;										// last template parameter

	table_t( symbol_t * symbol );
	~table_t();

	void push_table();
	void display_table( int blank );
	symbol_t * search_table( hash_t * hash );
	symbol_t * search_table2( hash_t * hash );
	void insert_table( symbol_t * symbol );
};

table_t * pop_table();

extern lexical_t * top;									// pointer to current top table
extern table_t * root;									// root table for global definitions
extern table_t * focus;									// pointer to current lookup table


// Local Variables: //
// compile-command: "make install" //
// End: //
