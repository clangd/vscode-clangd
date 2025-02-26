//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2024
// 
// Array.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Thu Jun 13 14:39:12 2024
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Jun 13 14:45:49 2024
// Update Count     : 2
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

struct Obj {
	int i, j;
	Obj( int i ) : i{i}, j{0} {}
	Obj( int i, int j ) : i{i}, j{j} {}
};
ostream & operator<<( ostream & os, const Obj & obj ) {
	return os << '(' << obj.i << ',' << obj.j << ')';
}

int main() {
//	int size;
//	cin >> size;
	enum { size = 20 };
	uArray( Obj, arr, size );						// VLA, no construction

	// ALL SUBSCRIPTING IS CHECKED
	for ( int i = 0; i < size; i += 1 ) {
		arr[i]( i, i );									// call 2 value constructor
		cout << *arr[i] << ' ';
	} // for
	cout << endl;

	arr[3] = arr[7];
	Obj fred( 3 );
	arr[12] = fred;
	arr[14] = 3;										// call 1 value constructor
	arr[15] = (Obj){ 3, 4 };

	for ( int i = 0; i < size; i += 1 ) {
		cout << *arr[i] << ' ';
	} // for
	cout << endl;
	cout << endl;

//	arr[100];											// subscript error

	uArrayPtr( Obj, arrp, size );
	for ( int i = 0; i < size; i += 1 ) {
		arrp[i]( i, i );								// call 2 value constructor
		cout << *arrp[i] << ' ';
	} // for
	cout << endl;

	arrp[3] = arrp[7];
	arrp[12] = fred;
	arrp[14] = 3;										// call 1 value constructor
	arrp[15] = (Obj){ 3, 4 };

	for ( int i = 0; i < size; i += 1 ) {
		cout << *arrp[i] << ' ';
	} // for
	cout << endl;

//	arrp[100];											// subscript error
}
