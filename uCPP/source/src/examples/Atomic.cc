//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Richard C. Bilson 2007
// 
// Atomic.cc -- 
// 
// Author           : Richard C. Bilson
// Created On       : Mon Sep 10 16:47:22 2007
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 26 08:18:52 2022
// Update Count     : 17
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

#define NPROCS 8
#define NTASKS 8
#define NITER 1000000

volatile int locn1 = 0, locn2 = 0;

_Task IncTester {
	void main() {
		int cur;
		for ( int i = 0; i < NITER; i += 1 ) {
			uFetchAdd( locn1, 1 );
			uFetchAdd( locn1, -1 );

			do {
				cur = locn2;
			} while( ! uCompareAssign( locn2, cur, cur + 1 ) );
			do {
				cur = locn2;
			} while( ! uCompareAssign( locn2, cur, cur - 1 ) );
		} // for
	} // IncTester::main
}; // IncTester

int main() {
	uProcessor p[ NPROCS - 1 ] __attribute__(( unused ));
	{
		IncTester testers[ NTASKS ] __attribute__(( unused ));
	}
	if ( locn1 == 0 && locn2 == 0 ) {
		cout << "successful completion" << endl;
	} else {
		cout << "error: expected values 0, 0 but got values " << locn1 << ", " << locn2 << endl;
	} // if
} // main
