//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2012
// 
// Errno.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Wed Sep 12 08:45:31 2012
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Apr 24 18:28:41 2022
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

_Task Worker {
	void main() {
		for ( int i = 0; i < 1000000; i += 1 ) {
			errno = i;
			yield();
			if ( i != errno ) abort( "Error: interference on errno %p %p %d %d\n", &uThisTask(), &uThisProcessor(), i, errno );
		} // for
	} // Worker::main
}; // Worker

int main() {
	uProcessor p[3];
	Worker w[10];
}
