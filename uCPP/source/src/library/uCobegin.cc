//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Thierry Delisle 2020
// 
// uCobegin.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sun Oct 11 15:53:22 2020
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Jan 17 09:29:33 2023
// Update Count     : 4
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

#define __U_KERNEL__
#include <uC++.h>
#include <uCobegin.h>

void uCobegin( std::initializer_list< std::function< void ( unsigned int ) >> funcs ) {
	unsigned int uLid = 0;
	_Task Runner {
		typedef std::function<void ( unsigned int )> Func; // function type
		unsigned int parm;								// local thread lid
		Func f;											// function to run for each lid

		void main() { f( parm ); }
	  public:
		Runner( unsigned int parm, Func f ) : parm( parm ), f( f ) {}
	}; // Runner

	const unsigned int size = funcs.size();
	uNoCtor< Runner > * runners = new uNoCtor< Runner >[size]; // do not use up task stack

	for ( auto f : funcs ) { runners[uLid].ctor( uLid, f ); uLid += 1; }
	delete [] runners;
} // uCobegin


// Local Variables: //
// compile-command: "make install" //
// End: //
