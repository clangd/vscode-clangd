//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2003
// 
// EHM9.cc -- recursive resumption
// 
// Author           : Peter A. Buhr
// Created On       : Sun Dec  7 12:15:30 2003
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Oct  7 08:05:45 2023
// Update Count     : 43
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


_Exception Up { public: int & up; Up( int & up ) : up(up) {}; };
_Exception Down { public: int & down; Down( int & down ) : down(down) {}; };

void foo() {
	int up = 0;
	try {
		for ( ; up < 5; ) _ResumeTop Up( up );
	} _CatchResume( Down down ) {
		cout << "Down " << up << " " << down.down << endl;
		down.down += 1;									// change main::down
	} // tru
	cout << "foo ends" << endl;
} // foo

void bar( int i ) {
	if ( i == 0 ) foo();								// base case
	else bar( i - 1 );
	cout << "bar ends" << endl;
} // bar

int main() {
	// Checks marking of handlers is performed correctly during propagation.
	// Marking prevents recursive resumption.

	_Exception R1 {};
	_Exception R2 {};

	try {
		try {
			try {
				cout << "before raise" << endl;
				_Resume R1();
				cout << "after raise" << endl;
			} _CatchResume( R2 ) {
				cout << "enter H3" << endl;
				_Resume R1();
				cout << "exit  H3" << endl;
			} // try
		} _CatchResume( R1 ) {
			cout << "enter H2" << endl;
			_Resume R2();
			cout << "exit  H2" << endl;
		} // try
	} _CatchResume( R2 ) {
		cout << "enter H1" << endl;
		cout << "exit  H1" << endl;
	} // try

	cout << endl;

	// Checking non-marking of handlers to create algebraic effects with bidirectional
	// resumption control-flow.

	int down = 0;
	try {
		bar( 2 );
		foo();
	} _CatchResume( Up up ) {
		cout << "Up " << down << " " << up.up << endl;
		up.up += 1;					// change foo::up
		_ResumeTop Down( down );
	} // try

	cout << "finished" << endl;
} // main


// Local Variables: //
// tab-width: 4 //
// End: //
