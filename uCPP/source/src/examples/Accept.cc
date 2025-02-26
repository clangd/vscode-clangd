//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2011
// 
// Accept.cc -- Check dynamically nested accepting returns to the correct code after the accept clause that accepted the
//              call.
// 
// Author           : Peter A. Buhr
// Created On       : Mon Mar 21 18:16:14 2011
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Feb 25 06:58:44 2022
// Update Count     : 7
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
using namespace std;


_Task T1 {
	int check;
	void main();
  public:
	void X();
	void Y();
	void Z();
};

_Task T0 {
	T1 &t1;
	void main() {
		t1.Y();
	}
  public:
	T0( T1 &t1 ) : t1( t1 ) {}
};

void T1::X() {
	check = 1;
}
void T1::Y() {
	check = 2;
	X();
	check = 2;
}
void T1::Z() {
	check = 3;
	T0 to( *this );
	_Accept( X ) {
		assert( check == 1 );
	} or _Accept( Y ) {
		assert( check == 2 );
	} or _Accept( Z ) {
		assert( check == 3 );
	}
	check = 3;
}
void T1::main() {
	for ( ;; ) {
		_Accept( ~T1 ) {
			break;
		} or _Accept( X ) {
			assert( check == 1 );
		} or _Accept( Y ) {
			assert( check == 2 );
		} or _Accept( Z ) {
			assert( check == 3 );
		}
	}
}

_Task T2 {
	T1 t1;
	void main() {
		t1.X();
		t1.Y();
		t1.Z();
	}
};

int main() {
	T1 t1;
	t1.X();
	uBaseTask::yield( 5 );
	t1.Y();
	t1.Z();
	T2 t2;
	cout << "successful completion" << endl;
}
