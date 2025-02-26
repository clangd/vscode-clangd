//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1998
// 
// EHM2.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue Oct 27 21:24:48 1998
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Oct  7 08:06:53 2023
// Update Count     : 44
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


_Exception xxx {
  public:
	uBaseTask *tid;
	xxx( uBaseTask *tid ) : tid(tid) {}
};

_Exception yyy {
  public:
	uBaseTask *tid;
	yyy( uBaseTask *tid ) : tid(tid) {}
};


_Task fred {
	void r( int i ) {
		if ( i == 0 ) {
			_Throw xxx( &uThisTask() );
		} else {
			r( i - 1 );
		} // if
	} // fred::r

	void s( int i ) {
		if ( i == 0 ) {
			try {
				r( 5 );
			} catch( xxx & e ) {
				assert( e.tid == &uThisTask() );
				_Throw;
			}
		} else {
			s( i - 1 );
		} // if
	} // fred::s

	void t( int i ) {
		if ( i == 0 ) {
			try {
				s( 5 );
			} catch( xxx & e ) {
				assert( e.tid == &uThisTask() );
				_Throw yyy( &uThisTask() );
			}
		} else {
			t( i - 1 );
		} // if
	} // fred::t
	
	void main() {
		for ( int i = 0; i < 5000; i += 1 ) {
//	for ( int i = 0; i < 5; i += 1 ) {
			try {
				t( 5 );
			} catch( yyy & e ) {
				assert( e.tid == &uThisTask() );
			} // try
		} // for
	} // fred::main
}; // fred

int main() {
	uProcessor processors[3] __attribute__(( unused ));	// more than one processor
	fred f[4] __attribute__(( unused ));
} // main
