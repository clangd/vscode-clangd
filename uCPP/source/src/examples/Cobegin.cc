//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2014
// 
// Cobegin.cc -- Check COBEGIN/COEND, COFOR, and START/WAIT concurrency control structures.
// 
// Author           : Peter A. Buhr
// Created On       : Sat Dec 27 18:20:07 2014
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Apr 24 18:33:11 2022
// Update Count     : 39
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

#include <uCobegin.h>
#include <iostream>

void p( int i, double d ) { std::osacquire( std::cout ) << "p " << i << " " << d << std::endl; }
int f( int i, double d ) { std::osacquire( std::cout ) << "f " << i << " " << d << std::endl; return 7; }

void loop( int N ) {
	if ( N != 0 ) {
		COBEGIN
			BEGIN p( N, 3.2 ); END
			BEGIN loop( N - 1 ); END
		COEND
	} // if
} // loop

int main() {
	// COBEGIN
	
	COBEGIN												// concurrent
		BEGIN std::cout << "foo " << uLid << std::endl; END
		BEGIN std::cout << "bar " << uLid << std::endl; END
		BEGIN std::cout << "xxx " << uLid << std::endl; END
		BEGIN
			std::cout << "yyy " << std::endl;
			COBEGIN										// concurrent
				BEGIN std::cout << "foo " << uLid << std::endl; END
				BEGIN std::cout << "bar " << uLid << std::endl; END
				BEGIN std::cout << "xxx " << uLid << std::endl; END
			COEND
		END
	COEND

	loop( 5 );											// create dynamic number of threads

	// COFOR

	const unsigned int rows = 10, cols = 10;			// sequential
	int matrix[rows][cols], subtotals[rows], total = 0;

	for ( unsigned int r = 0; r < rows; r += 1 ) {
		for ( unsigned int c = 0; c < cols; c += 1 ) {
			matrix[r][c] = 1;
		} // for
	} // for

	COFOR( row, 0, rows,								// for row = 0 to rows { loop body }
		int & subtotal = subtotals[row];				// concurrent
		subtotal = 0;
		for ( unsigned int c = 0; c < cols; c += 1 ) {
			subtotal += matrix[row][c];
		} // for
	); // COFOR

	// START/WAIT

	for ( unsigned int r = 0; r < rows; r += 1 ) {		// sequential
		total += subtotals[r];
	} // for

	std::cout << "total:" << total << std::endl;

	auto tp = START( p, 2, 4.1 );
	std::cout << "m1" << std::endl;						// concurrent
	WAIT( tp );
	auto tf = START( f, 3, 5.2 );						// f( 3, 5.2 )
	std::cout << "m2" << std::endl;						// concurrent
	std::cout << WAIT( tf ) << std::endl;
} // main

// Local Variables: //
// compile-command: "../../bin/u++ -std=c++1y -O2 -nodebug Cobegin.cc" //
// End: //
