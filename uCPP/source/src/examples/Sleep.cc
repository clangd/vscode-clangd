//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Philipp E. Lim 1996
// 
// Sleep.cc -- 
// 
// Author           : Philipp E. Lim
// Created On       : Wed Jan 10 17:02:39 1996
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 26 13:05:01 2022
// Update Count     : 26
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

volatile int x = 0, y = 1;

_Task fred {
	void main() {
		for ( ;; ) {
		  if ( x == 20 ) break;
			if ( x < y ) x += 1;
			uTime start = uClock::currTime();
			_Timeout( uDuration( 1 ) );
			osacquire( cout ) << "fred slept for " << uClock::currTime() - start << " seconds" << endl;
		} // for
		osacquire( cout ) << "fred finished" << endl;
	} // fred::main
}; // fred

_Task mary {
	void main() {
		for ( ;; ) {
		  if ( y == 20 ) break;
			if ( y == x ) y += 1;
			uTime start = uClock::currTime();
			_Timeout( uDuration( 2 ) );
			osacquire( cout ) << "mary slept for " << uClock::currTime() - start << " seconds" << endl;
		} // for
		osacquire( cout ) << "mary finished" << endl;
	} // mary::main
}; // mary

int main() {
	fred f;
	mary m;
} // main

// Local Variables: //
// compile-command: "u++ Sleep.cc" //
// End: //
