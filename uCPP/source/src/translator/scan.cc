//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// scan.c --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 15:11:49 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Dec 24 18:11:08 2021
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

#include "hash.h"
#include "token.h"
#include "table.h"
#include "key.h"
#include "scan.h"

#include <cstdio>										// EOF

//#define __U_DEBUG_H__
#include "debug.h"

#ifdef __U_DEBUG_H__
#include <iostream>
using std::cerr;
using std::endl;
#endif // __U_DEBUG_H__


void scan() {
	ahead = ahead->next_parse_token();

  if ( ahead->value == EOF ) return;

	if ( ahead->hash->value != 0 ) {
		// if the value of the hash associated with the look ahead token is non zero, it must be a keyword.  simply make
		// the value of the token the value of the keyword.

		ahead->value = ahead->hash->value;
	} else if ( ahead->symbol != nullptr ) {
		// symbol has already been looked up and the parser has backtracked to it again.
	} else if ( ahead->value == IDENTIFIER ) {
		// use the symbol to determine whether the identifier is a type or a variable.

		uDEBUGPRT( cerr << "scan: token " << ahead << " (" << ahead->hash->text << ") focus " << focus << endl; );
		if ( focus != nullptr ) {						// scanning mode
			ahead->symbol = focus->search_table( ahead->hash );
			if ( ahead->symbol != nullptr ) {
				uDEBUGPRT( cerr << "scan: setting token " << ahead << " (" << ahead->hash->text << ") to value " << ahead->symbol->value << endl; )
					ahead->value = ahead->symbol->value;
			} // if
		} // if
	} // if
} // scan


void unscan( token_t * back ) {
	for ( token_t * t = back->next_parse_token(); t != ahead->next_parse_token(); t = t->next_parse_token() ) {
		uDEBUGPRT( if ( t->value == TYPE || t->symbol != nullptr ) cerr << "unscan: clearing token " << ahead << " (" << ahead->hash->text << ")" << endl; )
			if ( t->value == TYPE ) t->value = IDENTIFIER;
		t->symbol = nullptr;
	} // for
	ahead = back;
} // unscan


// Local Variables: //
// compile-command: "make install" //
// End: //
