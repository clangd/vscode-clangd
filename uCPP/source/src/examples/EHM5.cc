//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2002
// 
// EHM5.cc -- 
// 
// Author           : Roy Krischer
// Created On       : Sun Nov 24 12:34:43 2002
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Oct  7 08:06:15 2023
// Update Count     : 20
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

_Exception fred {
  public:
	int k;
	fred ( int k ) : k(k) {}
};

class mary {
  public:
	void foo() {
		_Throw fred( 42 );
	}
};

class john {
  public:
	void bar() {
		_Throw fred( 84 );
	}
};

class bob : public mary, public john { };

void foo() {
	_Throw fred( 666 );
}

int main() {
	mary m;
	john j;
	bob b;
	
//********* m ***********

	try {
		m.foo();
	} catch ( m.fred & f ) {
		cout << "thrower: m " << f.k << endl;
	} catch ( j.fred f ) {
		cout << "thrower: j " << f.k << endl;
		abort( "wrong binding matched, should have been m" );
	} catch ( fred & f ) {
		cout << "unbound " << f.k << endl;
		abort( "binding did not match, execution should have never reached here" );
	}

//********* j ***********

	try {
		j.bar();
	} catch ( m.fred & f ) {
		cout << "thrower: m " << f.k << endl;
		abort( "wrong binding matched, should have been j" );
	} catch ( j.fred f ) {
		cout << "thrower: j " << f.k << endl;
	} catch ( fred & f ) {
		cout << "unbound " << f.k << endl;
		abort( "binding did not match, execution should have never reached here" );
	}
	
//********* unbound ***********    

	try {
		foo();
	} catch ( m.fred & f ) {
		cout << "thrower: m " << f.k << endl;
		abort( "wrong binding matched, should have been unbound" );
	} catch ( j.fred f ) {
		cout << "thrower: j " << f.k << endl;
		abort( "wrong binding matched, should have been unbound" );
	} catch ( fred & f ) {
		cout << "unbound " << f.k << endl;
	}

// ******** multiple ***********

	try {
		b.bar();
	} catch ( j.fred & f ) {
		cout << "thrower: j  " << f.k << endl;
		abort( "wrong binding matched, should have been b" );
	} catch ( m.fred f ) {
		cout << "thrower: m  " << f.k << endl;
		abort( "wrong binding matched, should have been b" );
	} catch ( b.fred f ) {
		cout << "thrower b " << f.k << endl;
	}

#if (__U_CPLUSPLUS__ == 4) && (__U_CPLUSPLUS_MINOR__ == 9)
// ******** multiple, don't let bind to mb!! ***********

	mary &mb = b;
	try {
		b.bar();
	} catch ( mb.fred f ) {
		cout << "thrower: mb  " << f.k << endl;
		abort( "wrong binding matched, mb should have an address too high to match" );
	} catch ( m.fred f ) {
		cout << "thrower: m  " << f.k << endl;
		abort( "wrong binding matched, should have been b" );
	} catch ( b.fred f ) {
		cout << "thrower b " << f.k << endl;
	}
#endif
}

// Local Variables: //
// tab-width: 4 //
// End: //
