//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// token.c --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:19:14 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Aug 20 10:18:48 2022
// Update Count     : 112
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

#include "uassert.h"
#include "hash.h"
#include "token.h"
#include "main.h"

#include <cstring>					// strcmp

// The token manager.
#define INITIAL_SIZE 1000

class token_manager_t {
  private:
	void * space_table;
	void * from_here;
	int nr_allocatable; // number of tokens for which space exists in the current block
  public:
	token_manager_t() : nr_allocatable( INITIAL_SIZE ) {
		space_table = malloc( nr_allocatable * sizeof( token_t ) );
		from_here = (char *)space_table + nr_allocatable * sizeof( token_t );
	} // token_manager_t::token_manager_t

	void * give_token() {
		if ( space_table == from_here ) {
			nr_allocatable *= 2;
			space_table = malloc( nr_allocatable * sizeof( token_t ) );
			from_here = (char *)space_table + nr_allocatable * sizeof( token_t );
		} // if
		from_here = (char *)from_here - sizeof( token_t );
		return from_here;
	} // token_manager_t::give_token
}; // token_manager_t

// The one and only operational token manager
static token_manager_t token_boss;

// look ahead token

token_t * ahead;

// token member functions

token_t::~token_t() {
	value = -1;
	hash = nullptr;
#if 0
	// There's problem here with copied references of this value, which means
	// the storage cannot be deleted. Needs fixing!
	symbol = nullptr;
#endif
	left = right = nullptr;
} // token_t::~token_t

void token_t::add_token_after( token_t & before ) {
	token_t * after;
	after = before.fore;
	aft = &before;
	fore = after;
	before.fore = this;
	uassert( after != nullptr );
	after->aft = this;
} // token_t::add_token_after

void token_t::add_token_before( token_t & after ) {
	token_t * before;
	before = after.aft;
	aft = before;
	fore = &after;
	uassert( before != nullptr );
	before->fore = this;
	after.aft = this;
} // token_t::add_toke_before

void token_t::remove_token() {
	uassert( fore != nullptr );
	uassert( aft != nullptr );
	fore->aft = aft;
	aft->fore = fore;
} // token_t::remove_token

// the following member routine returns the next token that is not white space or directive.

token_t * token_t::next_parse_token() {
	token_t * next = fore;
	uassert( next != nullptr );
	while ( next->value == '\n' || next->value == '\r' || next->value == '#' ) {
		if ( next->value == '#' ) {
			file_token = next;
			parse_directive( next->hash->text, file, line, flag );
		} else {
			// SKULLDUGGERY: prevent backtracking from incrementing line number multiple times by marking the "left"
			// variable, which is not used for token "#".
			if ( next->left == nullptr ) {
				line += 1;
				next->left = (token_t *)1;
			} // if
		} // if
		next = next->fore;
		uassert( next != nullptr );
	} // while
	return next;
} // next_parse_token

token_t * token_t::prev_parse_token() {
	token_t * prev = aft;
	uassert( prev != nullptr );
	while ( prev->value == '\n' || prev->value == '\r' || prev->value == '#' ) {
		prev = prev->aft;
		uassert( prev != nullptr );
		if ( prev->value == '#' ) {			// adjust line numbering during backup
			line -= 1;
			prev->left = nullptr;
		} // if
	} // while
	return prev;
} // prev_parse_token


// look ahead list

token_list_t list__base;
token_list_t * token_list = &list__base;

// token list member functions

token_list_t::token_list_t() {
	head.fore = &tail;
	head.value = 0;
	head.aft = nullptr;
	tail.fore = nullptr;
	tail.value = 0;
	tail.aft = &head;
} // token_list_t::token_list_t

token_list_t::~token_list_t() {
} // token_list_t::~token_list_t

void token_list_t::add_to_head( token_t & insert ) {
	insert.add_token_after( head );
} // token_list_t::add_to_head

void token_list_t::add_to_tail( token_t & insert ) {
	insert.add_token_before( tail );
} // token_list_t::add_to_tail

token_t * token_list_t::remove_from_head() {
	token_t * token;
	token = head.fore;
	token->remove_token();
	return token;
} // remove_from_head

token_t * token_list_t::remove_from_tail() {
	token_t * token;
	token = tail.aft;
	token->remove_token();
	return token;
} // token_list_t::remove_from_tail

token_t * token_list_t::get_head() {
	return &head;
} // token_list::head

token_t * token_list_t::get_tail() {
	return &tail;
} // token_list_t::tail

int token_list_t::empty() {
	return head.fore == &tail;
	// return tail.aft == &head;
} // token_list_t::empty

// Local Variables: //
// compile-command: "make install" //
// End: //
