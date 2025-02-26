//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// attribute.h --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 16:02:53 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Dec 24 17:17:47 2021
// Update Count     : 69
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


// Mutex qualifiers are NOT part of declaration qualifier because mutual exclusion attributes cannot be accumulated
// acrossed declarations like declarations qualifiers.

union declmutex {
	int value;
	struct {
		bool MUTEX : 1;									// for mutex type: default mutex attribute for public members
		// for mutex member: mutex qualifier of member
		bool mutex : 1;									// only indicates the presence of the keyword qualifier in a specifier
		bool NOMUTEX : 1;
		bool nomutex : 1;								// only indicates the presence of the keyword qualifier in a specifier
	} qual;
};

union declqualifier {
	int value;
	struct {
		bool REGISTER : 1;
		bool STATIC : 1;
		bool EXTERN : 1;
		bool MUTABLE : 1;
		bool THREAD : 1;
		bool EXTENSION : 1;
		bool INLINE : 1;
		bool VIRTUAL : 1;
		bool EXPLICIT : 1;
		bool CONST : 1;
		bool VOLATILE : 1;
		bool RESTRICT : 1;
		bool ATOMIC : 1;
	} qual;
};

union declkind {
	int value;
	struct {
		bool TYPEDEF : 1;
		bool FRIEND : 1;
		bool CONSTEXPR : 1;
	} kind;
};

union rttaskkind {
	int value;
	struct {
		bool PERIODIC : 1;
		bool APERIODIC : 1;
		bool SPORADIC : 1;
	} kind;
};

struct table_t;											// forward declaration
struct symbol_t;										// forward declaration
struct token_t;											// forward declaration

struct attribute_t {
	bool Mutex;											// for mutex type: true => mutex type (not redundant with dclqual qualifier)
	// for mutex member: true => mutex member (redundant with dclqual qualifier)
	declmutex dclmutex;									// declaration mutex
	declqualifier dclqual;								// declaration qualifiers
	declkind dclkind;									// declaration kind
	rttaskkind rttskkind;								// kind of realtime task
	symbol_t * typedef_base;							// typedef base type
	bool emptyparms;									// indicates routine parameter list is empty
	bool nestedqual;									// indicates nest qualification of routine name

	table_t * focus;									// current focus at time template is found
	table_t * plate;									// template attributes

	token_t * startT, * endT;							// start/end of template
	token_t * startRet, * endRet;						// start/end of routine return-type
	token_t * startParms, * endParms;					// start/end of routine parameter list
	token_t * startCR;									// start of class/routine implementation (not forward)
	token_t * startI;									// start of identifier
	token_t * startE;									// start of entry list type
	token_t * startM;									// start of mutex list type
	token_t * endM;										// end of mutex type attribute
	token_t * startP;									// start of PIQ list type
	token_t * endP;										// end of PIQ type attribute

	token_t * fileCR;									// last file name seen before start of object declaration
	unsigned int lineCR;								// last line number seen before start of object declaration

	attribute_t();
	~attribute_t();
};


// Local Variables: //
// compile-command: "make install" //
// End: //
