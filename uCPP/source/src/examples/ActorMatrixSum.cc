// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2016
//
// ActorMatrixSum.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sat Dec 29 16:36:02 2018
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Apr 24 18:28:21 2022
// Update Count     : 11
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
#include <uActor.h>


_Actor Adder {
	int * row, cols, & subtotal;						// communication

	Allocation receive( Message & ) {
		subtotal = 0;
		for ( int c = 0; c < cols; c += 1 ) {
			subtotal += row[c];
		} // for
		return Delete;
	} // Adder::receive
  public:
	Adder( int row[], int cols, int & subtotal ) :
	row( row ), cols( cols ), subtotal( subtotal ) {}
}; // Adder

int main() {
	enum { rows = 10, cols = 10 };
	int matrix[rows][cols], subtotals[rows], total = 0;

	for ( unsigned int r = 0; r < rows; r += 1 ) {		// initialize matrix
		for ( unsigned int c = 0; c < cols; c += 1 ) {
			matrix[r][c] = 1;
		} // for
	} // for
	uActor::start();									// start actor system
	for ( unsigned int r = 0; r < rows; r += 1 ) {		// actor per row
		*(new Adder( matrix[r], cols, subtotals[r] )) | uActor::startMsg;
	} // for
	uActor::stop();										// wait for all actors to terminate
	for ( int r = 0; r < rows; r += 1 ) {				// wait for threads to finish
		total += subtotals[r];							// total subtotals
	}
	cout << total << endl;
} // main

// Local Variables: //
// compile-command: "u++-work -g -O2 -multi ActorMatrixSum.cc" //
// End: //
