// 
// Copyright (C) Peter A. Buhr 2016
// 
// RWLock.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Aug 29 21:39:45 2016
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Jun 10 18:34:28 2023
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

#include <uRWLock.h>
#include <iostream>
using namespace std;

const unsigned int NoOfTimes =
#ifdef __U_STATISTICS__
	100'000;
#else
	1'000'000;
#endif // __U_STATISTICS__
const unsigned int Work = 100;

uRWLock rwlock;

_Task Reader {
	void main() {
		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			rwlock.rdacquire();
			// if ( rwlock.wrcnt() != 0 )
			// 	abort( "reader interference: wcnt %d, rcnt %d", rwlock.wrcnt(), rwlock.rdcnt() );
			for ( volatile unsigned int b = 0; b < Work; b += 1 );
			// if ( rwlock.wrcnt() != 0 )
			//     abort( "reader interference: wcnt %d, rcnt %d", rwlock.wrcnt(), rwlock.rdcnt() );
			rwlock.rdrelease();
		} // for
	} // main
  public:
};

_Task Writer {
	void main() {
		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			for ( volatile unsigned int b = 0; b < Work; b += 1 );
			rwlock.wracquire();
			// if ( rwlock.wrcnt() != 1 || rwlock.rdcnt() != 0 )
			// 	abort( "writer interference: wcnt %d, rcnt %d", rwlock.wrcnt(), rwlock.rdcnt() );
			for ( volatile unsigned int b = 0; b < Work; b += 1 );
			// if ( rwlock.wrcnt() != 1 || rwlock.rdcnt() != 0 )
			//     abort( "writer interference: wcnt %d, rcnt %d", rwlock.wrcnt(), rwlock.rdcnt() );
			rwlock.wrrelease();
			for ( volatile unsigned int b = 0; b < Work; b += 1 );
		} // for
	} // main
  public:
}; // Writer


int main() {
	enum { NoOfReaders = 6, NoOfWriters = 2 };
	uProcessor p[NoOfReaders + NoOfWriters];
	{
		Writer writers[NoOfWriters];
		Reader readers[NoOfReaders];
	}
	cout << "successful completion" << endl;
} // main

// Local Variables: //
// compile-command: "../../bin/u++ -Wall -Wextra -O3 -multi -nodebug -g RWLock.cc" //
// End: //
