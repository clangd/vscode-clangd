//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2000
// 
// SRC2.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sun Dec 10 08:44:06 2000
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Dec 19 23:02:43 2016
// Update Count     : 153
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

_Monitor SRC {
	const unsigned int MaxItems;						// maximum items in the resource pool
	int Free, Taken, Need;
	bool Locked;										// allocates blocked at this time
  public:
	SRC( unsigned int MaxItems );
	~SRC();
	_Nomutex unsigned int Max();
	void Hold();
	void Resume();
	void Deallocate();
	void Allocate( unsigned int N );
}; // SRC

SRC::SRC( unsigned int MaxItems = 5 ) : MaxItems( MaxItems ) {
	Free = MaxItems;
	Taken = 0;
	Need = -1;
	Locked = false;
	uAcceptReturn( Hold, Allocate, ~SRC );				// only initial calls allowed
} // SRC::SRC

SRC::~SRC() {
} // SRC::SRC

unsigned int SRC::Max() {
	return MaxItems;
} // SRC::Max

void SRC::Hold() {
	osacquire( cout ) << &uThisTask() << " Hold,       Free:" << Free << " Taken:" << Taken << endl;
	Locked = true;
	uAcceptReturn( Resume, Deallocate );
} // SRC::Hold

void SRC::Resume() {
	osacquire( cout ) << &uThisTask() << " Resume,     Free:" << Free << " Taken:" << Taken << endl;
	Locked = false;

	// When allocation is locked, Deallocate cannot restart a blocked allocator when it determines sufficient resources
	// are available. This deferred responsibility is transfer to Resume when allocation is subsequently unlocked.

	if ( Need > 0 ) {									// task waiting for resources ?
		if ( Free < Need ) uAcceptReturn( Hold, Deallocate ); // insufficient available ?
		// otherwise, sufficient resources so wake up task waiting in Allocate
	} else uAcceptReturn( Hold, Deallocate, Allocate, ~SRC );
} // SRC::Resume

void SRC::Deallocate() {
	if ( Taken <= 0 ) abort( "problem 1" );
	Free += 1;
	Taken -= 1;
	assert( Free >= 0 && Taken <= MaxItems );
	osacquire( cout ) << &uThisTask() << " Deallocate, Free:" << Free << " Taken:" << Taken << " Locked:" << Locked << " Need:" << Need << endl;

	if ( Locked ) uAcceptReturn( Resume, Deallocate );

	if ( Need > 0 ) {									// task waiting for resources ?
		if ( Free < Need ) uAcceptReturn( Hold, Deallocate ); // insufficient available ?
		// otherwise, sufficient resources so wake up task waiting in Allocate
	} else uAcceptReturn( Hold, Deallocate, Allocate, ~SRC );
} // SRC::Deallocate

void SRC::Allocate( unsigned int N = 1 ) {
	if ( N > MaxItems ) abort( "problem 2" );
	osacquire( cout ) << &uThisTask() << " Allocate(" << N << "), enter, Free:" << Free << " Taken:" << Taken << endl;
	if ( N > Free ) {									// insufficient resources ?
		Need = N;										// indicate total number of resources need
		_Accept( Hold, Deallocate );					// block until sufficient resources
		Need = -1;										// reset to -1 so unequal to any Free value
	} // if
	Free -= N;
	Taken += N;
	assert( ! Locked && Free >= 0 && Taken <= MaxItems );
	osacquire( cout ) << &uThisTask() << " Allocate(" << N << "), exit, Free:" << Free << " Taken:" << Taken << endl;
	_AcceptReturn( Hold, Deallocate, Allocate );
} // SRC::Allocate


SRC src;												// global: used by all workers

_Task worker {
	void main() {
		for ( int i = 0; i < 20; i += 1 ) {
			if ( random() % 10 < 2 ) {					// M out of N calls are Hold/Resume
				src.Hold();
				yield( 50 );							// pretend to do something
				src.Resume();
			} else {
				int N = random() % src.Max() + 1;		// values between 1 and Max, inclusive
				src.Allocate( N );
				yield( 3 );								// pretend to do something
				for ( int i = 0; i < N; i += 1 ) {
					src.Deallocate();
				} //for
			} // if
		} // for
		osacquire( cout ) << &uThisTask() << " worker, exit" << endl;
	} // worker::main
}; // worker


int main() {
	{
		worker workers[10];
	} // wait for workers to complete
	osacquire( cout ) << "successful completion" << endl;
} // main


// Local Variables: //
// tab-width: 4 //
// compile-command: "u++-work SRC1.cc" //
// End: //
