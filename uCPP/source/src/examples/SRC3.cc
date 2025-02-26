//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Ashif S. Harji 2000
// 
// SRC3.cc -- 
// 
// Author           : Ashif S. Harji
// Created On       : Fri Dec 22 14:58:38 2000
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Dec 19 23:02:55 2016
// Update Count     : 28
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
} // SRC::SRC

SRC::~SRC() {
} // SRC::SRC

unsigned int SRC::Max() {
	return MaxItems;
} // SRC::Max

void SRC::Hold() {
	while ( Locked ) {
		_Accept( Resume, Deallocate );
	} // while
	osacquire( cout ) << &uThisTask() << " Hold,       Free:" << Free << " Taken:" << Taken << endl;
	Locked = true;
} // SRC::Hold

void SRC::Resume() {
	while ( ! Locked ) {								// assume Resume never accepted if outstanding allocate
		_Accept( Hold ) {
		} or _When( Free > 0 ) _Accept( Allocate ) {
		} or _Accept( Deallocate ) {
		} // _Accept
	} // while
	osacquire( cout ) << &uThisTask() << " Resume,     Free:" << Free << " Taken:" << Taken << endl;
	Locked = false;
} // SRC::Resume

void SRC::Deallocate() {
	if ( Taken <= 0 ) abort( "problem 2" );
	Free += 1;
	Taken -= 1;
	assert( Free >= 0 && Taken <= MaxItems );
	osacquire( cout ) << &uThisTask() << " Deallocate, Free:" << Free << " Taken:" << Taken << " Locked:" << Locked << " Need:" << Need << endl;
} // SRC::Deallocate

void SRC::Allocate( unsigned int N = 1 ) {
	if ( N > MaxItems ) abort( "problem 3" );
	osacquire( cout ) << &uThisTask() << " Allocate(" << N << "), enter, Free:" << Free << " Taken:" << Taken << endl;
	while ( Locked || ! Free > 0 ) {
		_When ( ! Locked ) _Accept( Hold ) {
		} or _When ( Locked ) _Accept( Resume ) {
		} or _Accept( Deallocate ) {
		} // _Accept
	} // while	
	Free -= N;
	Taken += N;
	assert( ! Locked && Free >= 0 && Taken <= MaxItems );
	osacquire( cout ) << &uThisTask() << " Allocate(" << N << "), exit, Free:" << Free << " Taken:" << Taken << endl;
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
				src.Allocate();
				yield( 3 );								// pretend to do something
				src.Deallocate();
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
// compile-command: "u++-work SRC3.cc" //
// End: //
