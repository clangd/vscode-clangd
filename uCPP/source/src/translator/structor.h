//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// structor.h --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:46:34 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Dec 24 18:12:22 2021
// Update Count     : 34
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

struct token_t;

class structor_t {
  public:
	char separator;										// separator cahracter before base-class initializer
	token_t * start;									// first token of non-base class initializer
	token_t * prefix;									// '{' of constructor/destructor
	token_t * suffix;									// '}' of constructor/destructor
	token_t * rp;										// ')' of constructor parameter list
	declmutex dclmutex;									// mutex qualifier of constructor/destructor
	bool defarg;
	structor_t * link;									// next constructor
	structor_t();
	~structor_t();
};

class structor_list_t {
  public:
	structor_t * head;
	structor_list_t();
	~structor_list_t();
	void add_structor( structor_t * structor );
	structor_t * remove_structor();
	int empty_structor_list();
};



// Local Variables: //
// compile-command: "make install" //
// End: //
