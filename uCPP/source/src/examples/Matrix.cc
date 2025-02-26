//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2019
// 
// Matrix.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Jul  8 11:19:32 2019
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Feb 18 08:27:11 2023
// Update Count     : 3
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
#include <uFuture.h>

struct Adder {											// routine, functor or lambda
	int * row, cols;									// communication
	int operator()() {									// functor-call operator
		int subtotal = 0;
		for ( int c = 0; c < cols; c += 1 ) {
			subtotal += row[c];
		} // for
		return subtotal;
	}
	Adder( int row[ ], int cols ) : row( row ), cols( cols ) {}
};

int main() {
	const int rows = 10, cols = 10;
	int matrix[rows][cols], total = 0;

	for ( int r = 0; r < rows; r += 1 ) {
		for ( int c = 0; c < rows; c += 1 ) {
			matrix[r][c] = 2;
		} // for
	} // for

	uExecutor executor( 4 );							// kernel threads
	Future_ISM<int> subtotals[rows];
	Adder * adders[rows];
	for ( int r = 0; r < rows; r += 1 ) {				// send off work for executor
		adders[r] = new Adder( matrix[r], cols );
		executor.sendrecv( *adders[r], subtotals[r] );
	} // for
	for ( int r = 0; r < rows; r += 1 ) {				// wait for results
		total += subtotals[r]();
		delete adders[r];
	} // for
	cout << total << endl;
}

// Local Variables: //
// tab-width: 4 //
// End: //
