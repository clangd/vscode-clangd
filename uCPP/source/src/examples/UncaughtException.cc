//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2010
// 
// UncaughtException.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sun Jul 18 11:14:42 2010
// Last Modified By : Peter A. Buhr
// Last Modified On : Wed Apr 20 23:01:58 2022
// Update Count     : 8
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
	void main() {
		for( ;; ) {
			_Accept( ~T1 ) break;
			try {
				throw 1;
			} catch( int ) {
			} // try
		} // for
	} // T1::main
}; // T1

_Task T2 {
	void main() {
		for( ;; ) {
			_Accept( ~T2 ) break;
			assert( ! std::__U_UNCAUGHT_EXCEPTION__() );
			yield();
		} // for
	} // T2::main
}; // T2

int main() {
	T1 t1;
	T2 t2;
	sleep( 5 );
	cout << "Successful completion" << endl;
} // main
