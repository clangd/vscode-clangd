//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// TimeSlice.cc -- test time slice
// 
// Author           : Peter A. Buhr
// Created On       : Mon Apr 26 11:04:37 1993
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Feb  4 14:33:47 2022
// Update Count     : 37
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


unsigned int uDefaultPreemption() {
	return 1;
} // uDefaultPreemption

enum { NoOfTimes = 1000 };
volatile int x = 0, y = 1;

_Task T1 {
	void main() {
		for ( ;; ) {
			if ( x == NoOfTimes ) break;
			if ( x < y ) x += 1;
		} // for
	} // T1::main
}; // T1

_Task T2 {
	void main() {
		for ( ;; ) {
			if ( y == NoOfTimes ) break;
			if ( y == x ) y += 1;
		} // for
	} // T2::main
}; // T2

int main() {
	T1 t1;
	T2 t2;
} // main

// Local Variables: //
// compile-command: "u++ TimeSlice.cc" //
// End: //
