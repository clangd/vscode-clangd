//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// table.c --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:32:43 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Jan  2 10:09:33 2025
// Update Count     : 436
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
#include "key.h"
#include "hash.h"
#include "token.h"
#include "table.h"

#include <iostream>
using std::cerr;
using std::endl;

extern void context();

//#define __U_DEBUG_H__
//#define __U_DEBUG_CONTEXT_H__
#include "debug.h"

lexical_t * top;										// pointer to current top table
table_t * root;											// root table for global definitions
table_t * focus;										// pointer to current lookup table

table_t::table_t( symbol_t * sym ) {
	// initialize the fields of the table

	local = nullptr;
	useing = false;
	lexical = nullptr;
	symbol = sym;
	access = 0;
	defined = false;
	hascopy = false;
	hasassnop = false;
	hasdefault = false;
	private_area = protected_area = public_area = nullptr;
	startT = endT = nullptr;

	// remember which table is associated with this symbol

	if ( symbol != nullptr ) {
		symbol->data->table = this;
	} // if
} // table_t::table_t


table_t::~table_t() {
	uDEBUGPRT(
		if ( symbol != nullptr ) {
			cerr << "delete table for " << symbol->hash->text << " containing:" << endl;
		} else {
			cerr << "delete table for anonymous template containing:" << endl;
		} // if
	);
	for ( local_t * l = local; l != nullptr; ) {
		local_t * curr = l;
		l = l->link;
		if ( ! curr->useing ) {							// ignore contents of using entry ?
			symbol_t * sym = curr->kind.sym;
			uDEBUGPRT( cerr << "\t" << sym->hash->text << endl; );

			// remember the hash node pointed to by this symbol table is not in this symbol table any more

			if ( sym->hash != nullptr ) {
				sym->hash->InSymbolTable -= 1;
			} // if

			delete sym;
		} // if
		delete curr;
	} // for
	local = nullptr;
	uDEBUGPRT(
#ifdef __U_DEBUG_CONTEXT_H__
		context();
#endif // __U_DEBUG_CONTEXT_H__
	);
} // table_t::~table_t


void table_t::push_table() {
	uDEBUGPRT(
		cerr << "PUSH FOCUS:" << ::focus << " (" << (::focus->symbol != nullptr ? ::focus->symbol->hash->text : (::focus == root) ? "root" : "template/compound") << ")" << endl;
#ifdef __U_DEBUG_CONTEXT_H__
		context();
#endif // __U_DEBUG_CONTEXT_H__
	);
	lexical_t * temp = new lexical_t( this );
	temp->link = top;
	top = temp;
	focus = this;
	uDEBUGPRT( cerr << "NEW FOCUS:" << ::focus << " (" << (::focus->symbol != nullptr ? ::focus->symbol->hash->text : (::focus == root) ? "root" : "template/compound") << ")" << endl; );
} // table_t::push_table


static void print_blanks( int blank ) {
	for ( int i = 0; i < blank; i += 1 ) {
		cerr << " ";
	} // for
} // print_blanks


void table_t::display_table( int blank ) {
	print_blanks( blank );
	cerr << "** table " << this << " focus " << ::focus << " lexical " << lexical;
	if ( symbol != nullptr && symbol->hash != nullptr ) {
		cerr << " \"" << symbol->hash->text << "\"" << endl;
	} else {
		cerr << endl;
	} // if

	for ( local_t * cur = local; cur != nullptr; cur = cur->link ) {
		print_blanks( blank );

		symbol_t * sym = cur->kind.sym;

		cerr << sym << " \"" << sym->hash->text << "\" table " << (sym->data ? sym->data->table : nullptr);
		switch ( sym->value ) {
		  case TYPE:
			cerr << " TYPE";
			switch( sym->data->key ) {
			  case ENUM:
				cerr << " ENUM";
				break;
			  case STRUCT:
				cerr << " STRUCT";
				break;
			  case UNION:
				cerr << " UNION";
				break;
			  case CLASS:
				cerr << " CLASS";
				break;
			  case NAMESPACE:
				cerr << " NAMESPACE";
				break;
			  case COROUTINE:
				cerr << " COROUTINE";
				break;
			  case PTASK:
				cerr << " PTASK";
				break;
			  case RTASK:
				cerr << " RTASK";
				break;
			  case STASK:
				cerr << " STASK";
				break;
			  case TASK:
				cerr << " TASK";
				break;
			  case EXCEPTION:
				cerr << " EXCEPTION";
				break;
			  case ACTOR:
				cerr << " ACTOR";
				break;
			  case CORACTOR:
				cerr << " CORACTOR";
				break;
			  default:
				break;
			} // switch
			if ( sym->copied ) cerr << " COPIED";
			if ( sym->typname ) { cerr << " TYPENAME" << endl; break; }
			cerr << endl;
			if ( sym->data->table != nullptr ) {
				if ( ! sym->data->attribute.dclkind.kind.TYPEDEF ) {
					sym->data->table->display_table( blank + 2 );
				} else {
					print_blanks( blank + 2 );
					cerr << " TYPEDEF for \"" << sym->data->table->symbol->hash->text << "\"" << endl;
				} // if
			} // if
			break;
		  case IDENTIFIER:
			cerr << " IDENTIFIER" << endl;
			break;
		  case OPERATOR:
			cerr << " OPERATOR" << endl;
			break;
		  case LABEL:
			cerr << " LABEL" << endl;
			break;
		  default:
			cerr << " UNKNOWN (" << sym->value << ")" << endl;
		} // switch
	} // for
} // table_t::display_table


static symbol_t * search_list( hash_t * hash, local_t * locals ) {
	for ( local_t * list = locals; list != nullptr; list = list->link ) {
		if ( list->tblsym ) {
			uDEBUGPRT( cerr << "\tbegin using table: " << list->kind.tbl->symbol->hash->text << endl; );
			symbol_t * temp = search_list( hash, list->kind.tbl->local );
			uDEBUGPRT( cerr << "\tend using table: " << list->kind.tbl->symbol->hash->text << endl; );
			if ( temp != nullptr ) return temp;
		} else {
			uDEBUGPRT( cerr << "\t\t" << list->kind.sym->hash->text << endl; );
			if ( list->kind.sym->hash == hash ) return list->kind.sym;
		} // if
	} // for

	return nullptr;
} // search_list


symbol_t * table_t::search_table( hash_t * hash ) {
	uDEBUGPRT(
		cerr << "LOOKUP:" << hash->text << endl;
		cerr << "SYMBOL TABLE:" << endl;
		root->display_table( 0 );
#ifdef __U_DEBUG_CONTEXT_H__
		context();
#endif // __U_DEBUG_CONTEXT_H__
	);

	// simple check determines if the hash is in any table

	if ( hash->InSymbolTable == 0 ) {
		uDEBUGPRT( cerr << "NOT FOUND:" << endl; );
		return nullptr;
	} // if

	// otherwise search the ST tree

	symbol_t * st = search_table2( hash );
	uDEBUGPRT(
		if ( st != nullptr ) {
			table_t * parent = st->data->found;
			cerr << "FOUND:" << hash->text << " in " << parent << " (" << (parent->symbol != nullptr ? parent->symbol->hash->text : parent == root ? "root" : "template/compound") << ")" << endl;
//		cerr << "AFTER:" << endl;
//		root->display_table( 0 );
		} // if
	);
	return st;
} // table_t::search_table


// Parser does not distinguish structure templates so cycles can form. Ignore them.
static symbol_t * inheritCycle = nullptr;

static symbol_t * search_base( hash_t * hash, symbol_t * symbol ) { // recursively search the base class tree
	uDEBUGPRT( cerr << "\tSTART SEARCH BASE: " << hash->text << " " << symbol->hash->text << " inheritance cycle " << inheritCycle << " symbol " << symbol << endl; );
	for ( std::list<symbol_t *>::iterator sym = symbol->data->base_list.begin(); sym != symbol->data->base_list.end(); sym++ ) {
		uDEBUGPRT( cerr << "\t" << (*sym)->hash->text << endl; );
		if ( (*sym) == inheritCycle ) return nullptr;	// check for inheritance cycle
		if ( (*sym)->data->table != nullptr ) {
			// search local symbol table
			symbol_t * temp = search_list( hash, (*sym)->data->table->local );
			if ( temp != nullptr ) return temp;
			// search base symbol table
			temp = search_base( hash, *sym );
			if ( temp != nullptr ) return temp;
		} // if
	} // for
	uDEBUGPRT( cerr << "\tEND SEARCH BASE:" << endl; );

	return nullptr;
} // search_base


symbol_t * table_t::search_table2( hash_t * hash ) {	// recursively search the ST tree
	uDEBUGPRT(
		if ( this == root ) {
			cerr << "CURRENT: root" << endl;
		} else {
			if ( symbol != nullptr ) {
				cerr << "CURRENT:" << symbol->hash->text << endl;
			} // if
		} // if
	);

	// first search the current block

	uDEBUGPRT( cerr << "CURRENT BLOCK:" << endl; );
	symbol_t * temp = search_list( hash, local );
	if ( temp != nullptr ) return temp;

	// search derived chain

	if ( symbol != nullptr && ! symbol->data->base_list.empty() ) {
		uDEBUGPRT( cerr << "DERIVED:" << endl; );
		inheritCycle = symbol;							// start of search
		symbol_t * temp = search_base( hash, symbol );
		inheritCycle = nullptr;							// end of search
		if ( temp != nullptr ) return temp;
	} // if

	// search the lexical chain

	if ( lexical != nullptr ) {
		uDEBUGPRT( cerr << "NEXT LEXICAL:" << endl; )
			return lexical->search_table2( hash );
	} // if

	uDEBUGPRT( cerr << "NOT FOUND:" << endl; )
		return nullptr;
} // table_t::search_table2


void table_t::insert_table( symbol_t * symbol ) {
	uassert( symbol != nullptr );
	uDEBUGPRT( cerr << "LOCAL ADD SYMBOL:" << symbol->hash->text << " to " << this << " (" << (table_t::symbol != nullptr ? table_t::symbol->hash->text : this == root ? "root" : "template/compound" ) << ")" << endl; );

	// link this symbol into the list of symbols associated with this table

	local_t * n = new local_t;
	n->useing = false;
	n->tblsym = false;									// symbol
	n->kind.sym = symbol;
	n->link = local;
	local = n;

	// remember which table this symbol is found in

	symbol->data->found = this;

	// remember that the hash node pointed to by this symbol is in another symbol table

	symbol->hash->InSymbolTable += 1;
	uDEBUGPRT(	 cerr << "SYMBOL TABLE:" << endl;
				 root->display_table( 0 ); );
} // table_t::insert_table


table_t * pop_table() {
	uDEBUGPRT(
		cerr << "POP FOCUS:" << ::focus << " (" << (::focus->symbol != nullptr ? ::focus->symbol->hash->text : (::focus == root) ? "root" : "template/compound") << ")" << endl;
#ifdef __U_DEBUG_CONTEXT_H__
		context();
#endif // __U_DEBUG_CONTEXT_H__
	);
	lexical_t * tempt = top;
	table_t * tempf = focus;
	top = top->link;
	focus = top->tbl;
	delete tempt;
	uDEBUGPRT( cerr << "NEW FOCUS:" << ::focus << " (" << (::focus->symbol != nullptr ? ::focus->symbol->hash->text : (::focus == root) ? "root" : "template/compound") << ")" << endl; );
	return tempf;
} // pop_table

// Local Variables: //
// compile-command: "make install" //
// End: //
