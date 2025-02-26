//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1998
// 
// EHM6.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue Oct 27 21:24:48 1998
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Oct  7 08:08:34 2023
// Update Count     : 30
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

#include <iostream>
using std::cout;
using std::osacquire;
using std::endl;

_Exception R1 {
  public:
	int &i; char &c;
	R1( int &i, char &c ) : i( i ), c( c ) {}
};

_Exception R2 {};


class fred {
  public:
	void f( int x, char y ) {
		osacquire( cout ) << "enter f, x:" << x << " y:" << y << endl;
		_Resume R2();
		osacquire( cout ) << "exit f, x:" << x << " y:" << y << endl;
	}
	void g( int &x, char &y ) {
		osacquire( cout ) << "enter g, x:" << x << " y:" << y << endl;
		_Resume R1( x, y );
		osacquire( cout ) << "exit g, x:" << x << " y:" << y << endl;
	}
};


int main() {
	fred ff, gg;

	try {
		int x = 0;
		char y = 'a';

		ff.g( x, y );
		osacquire( cout ) << "try<R1,rtn1> x:" << x << " y:" << y << endl;

		// Check multiple handlers, only one is used.
		try {
			gg.f( x, y );
			osacquire( cout ) << "try<R2,rtn2>, x:" << x << " y:" << y << endl;
		} _CatchResume( R1 ) {
		} _CatchResume( gg.R2 ) {
			osacquire( cout ) << "rtn2, i:" << x << " c:" << y << endl;
			x = 2; y = 'c';									// change x, y
		} // try
		try {								// R1 empty handler
			ff.g( x, y );
			osacquire( cout ) << "try<R1>, x:" << x << " y:" << y << endl;
		} _CatchResume( R1 ) {
		} _CatchResume( gg.R2 ) {
			osacquire( cout ) << "rtn2, i:" << x << " c:" << y << endl;
			x = 2; y = 'c';									// change x, y
		} // try
	} _CatchResume( R1 &r ) {
		osacquire( cout ) << "rtn1" << endl;
		r.i = 1; r.c = 'b';
	} // try
}

// Local Variables: //
// tab-width: 4 //
// End: //
