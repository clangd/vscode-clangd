//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// structor.c --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:17:01 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Dec 24 18:11:39 2021
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

#include "structor.h"

structor_t::structor_t() {
	prefix = nullptr;
	suffix = nullptr;
	rp = nullptr;
	link = nullptr;
} // structor_t::structor_t


structor_t::~structor_t() {
} // structor_t::structor_t


structor_list_t::structor_list_t() : head( nullptr ) {
} // structor_list_t:: structor_list_t


structor_list_t::~structor_list_t() {
} // structor_list_t::~structor_list_t


void structor_list_t::add_structor( structor_t * structor ) {
	structor->link = head;
	head = structor;
} // structor_list_t::add_structor


structor_t * structor_list_t::remove_structor() {
	structor_t * structor = head;
	head = structor->link;
	return structor;
} // structor_list_t::remove_structor


int structor_list_t::empty_structor_list() {
	return ( head == nullptr );
} // structor_list_t::empty_structor_list


// Local Variables: //
// compile-command: "make install" //
// End: //
