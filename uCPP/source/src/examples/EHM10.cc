//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2004
// 
// EHM10.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Mar 22 21:17:24 2004
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Oct  7 08:06:27 2023
// Update Count     : 15
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
using std::endl;

_Exception R1 {
  public:
	int &i;
	R1( int &i ) : i(i) {}
};

_Exception R2 {
  public:
	int &i;
	R2( int &i ) : i(i) {}
};

void f( int &i );
void g( int &i );

void f( int &i ) {
	i -= 1;
	cout << "f, i:" << i << endl;
	if ( i > 5 ) {
		try {
			if ( i < 10 ) _Resume R2(i);
			try {
				g( i );
			} _CatchResume( R2 &r ) {
				r.i -= 1;
				cout << "h2, i:" << r.i << endl;
				g( r.i );
			} // if
		} _CatchResume( R1 &r ) {
			r.i -= 1;
			cout << "h1, i:" << r.i << endl;
			f( r.i );
		} // try
	} else {
		if ( i > -5 ) _Resume R2(i);
	} // if
}

void g( int &i ) {
	i -= 1;
	cout << "g, i:" << i << endl;
	if ( i > 10 ) {
		try {
			f( i );
		} _CatchResume( R2 &r ) {
			r.i -= 1;
			cout << "h2, i:" << r.i << endl;
			g( r.i );
		} // try
		try {
			f( i );
		} _CatchResume( R1 &r ) {
			r.i -= 1;
			cout << "h3, i:" << r.i << endl;
		} // try
		_Resume R1(i);
	}
}
int main() {
	int i = 20;
	f( i );
}

// Local Variables: //
// tab-width: 4 //
// End: //
