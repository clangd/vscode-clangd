//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2002
// 
// EHM7.cc -- 
// 
// Author           : Roy Krischer
// Created On       : Sun Nov 24 12:42:34 2002
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Jun 11 08:52:23 2024
// Update Count     : 36
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

_Exception Fred {
  public:
	int k;
	Fred ( int k ) : k(k) {}
};

_Task Mary {
	void main();
}; // Mary

_Task John {
	void main();
}; // John

Mary mary;
John john;

void Mary::main() {
	_Resume Fred( 42 ) _At john;
	try {
		_Enable {
			for ( int i = 0; i < 200; i+= 1 ) yield();
		} // _Enable
	} catch ( john.Fred & fred ) {
		assert( &john == fred.getRaiseObject() );
		assert( &john == &fred.source() );
		assert( fred.k == 84 ); 
		osacquire( cout ) << "Mary catches exception from John: " << fred.k << endl;
		for ( int i = 0; i < 200; i+= 1 ) yield();
	} // try
} // Mary::main

void John::main() {
	_Resume Fred( 84 ) _At mary;
	try {
		_Enable {
			for ( int i = 0; i < 200; i+= 1 ) yield();
		} // _Enable
	} catch ( mary.Fred & fred ) {
		assert( &mary == fred.getRaiseObject() );
		assert( &mary == &fred.source() );
		assert( fred.k == 42 ); 
		osacquire( cout ) << "John catches exception from Mary: " << fred.k << endl
						  << "Mary's address: " << (void *)&mary << " exception binding: " << fred.getRaiseObject()
						  << " exception Src: " << (void *)&fred.source() << endl;
		for ( int i = 0; i < 200; i+= 1 ) yield();
	} // try
} // John::main

int main() {
	for ( int i = 0; i < 200; i+= 1 ) uBaseTask::yield();
} // main

// Local Variables: //
// tab-width: 4 //
// End: //
