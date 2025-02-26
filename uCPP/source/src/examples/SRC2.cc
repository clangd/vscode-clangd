//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2000
// 
// SRC2.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sun Dec 10 08:44:06 2000
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Dec 19 23:02:49 2016
// Update Count     : 262
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

#include <uSequence.h>
#include <iostream>
using std::cout;
using std::osacquire;
using std::endl;

_Monitor SRC {
	const unsigned int MaxItems;						// maximum items in the resource pool
	int Free, Taken;
	bool Locked;										// allocates blocked at this time

	struct Node : public uSeqable {
		unsigned int N;									// allocation request
		uCondition c;									// place to wait until request can be serviced
		Node( unsigned int N ) : N( N ) {}
	} *p;												// index for searching list of pending requests
	uSequence<Node> list;								// list of pending requests
	uSeqIter<Node> iter;								// iterator to search list of pending requests

	bool CheckPending();
  public:
	SRC( unsigned int MaxItems );
	~SRC();
	_Nomutex unsigned int Max();
	void Hold();
	void Resume();
	void Deallocate();
	void Allocate( unsigned int N );
}; // SRC

bool SRC::CheckPending() {
	// While locked, multiple deallocates can occur, so multiple pending requests may be able to be processed. A single
	// pass over the FIFO list is sufficient as Free is strictly decreasing.

	bool flag = false;
	int temp = Free;									// temporary count of free resources
	for ( iter.over(list); iter >> p && temp > 0; ) {	// O(N) search
		osacquire( cout ) << &uThisTask() << " CheckPending, temp:" << temp << " p->N:" << p->N << endl;
		if ( p->N <= temp ) {							// sufficient resources ?
			flag = true;
			temp -= p->N;								// reduce temporary number of free resource
			p->c.signal();								// wake up task waiting in Allocate
		} // if
	} // for
	return flag;
} // SRC::CheckPending

SRC::SRC( unsigned int MaxItems = 5 ) : MaxItems( MaxItems ) {
	Free = MaxItems;
	Taken = 0;
	Locked = false;
	uAcceptReturn( Hold, Allocate, ~SRC );				// only initial calls allowed
} // SRC::SRC

SRC::~SRC() {
	if ( ! list.empty() ) abort( "problem 4" );
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
	if ( ! Locked ) abort( "problem 1" );				// Resume call before Hold disallowed
	osacquire( cout ) << &uThisTask() << " Resume,     Free:" << Free << " Taken:" << Taken << " Waiting:" << list.empty() << endl;
	Locked = false;
	if ( ! CheckPending() ) uAcceptReturn( Hold, Deallocate, Allocate, ~SRC ); // check for any pending requests
} // SRC::Resume

void SRC::Deallocate() {
	if ( Taken <= 0 ) abort( "problem 2" );
	Free += 1;
	Taken -= 1;
	assert( Free >= 0 && Taken <= MaxItems );
	osacquire( cout ) << &uThisTask() << " Deallocate, Free:" << Free << " Taken:" << Taken << " Locked:" << Locked << " Waiting:" << list.empty() << endl;
	if ( Locked ) uAcceptReturn( Resume, Deallocate );
	if ( ! CheckPending() ) uAcceptReturn( Hold, Deallocate, Allocate, ~SRC ); // check for any pending requests
} // SRC::Deallocate

void SRC::Allocate( unsigned int N = 1 ) {
	if ( N > MaxItems ) abort( "problem 3" );
	osacquire( cout ) << &uThisTask() << " Allocate(" << N << "), enter, Free:" << Free << " Taken:" << Taken << " Waiting:" << list.empty() << endl;
	if ( N > Free ) {									// insufficient resources ?
		Node n( N );									// storage on stack of blocked task => no dynamic allocation
		list.add( &n );									// FIFO order, O(1) operation
		uAcceptWait( Hold, Deallocate, Allocate ) n.c;	// block until sufficient resources
//		n.c.wait();										// block until sufficient resources
		list.remove( &n );								// O(1) operation
		osacquire( cout ) << &uThisTask() << " Allocate(" << N << "), after, Free:" << Free << " Taken:" << Taken << " Waiting:" << list.empty() << endl;
	} // if
	Free -= N;
	Taken += N;
	assert( ! Locked && Free >= 0 && Taken <= MaxItems );
	osacquire( cout ) << &uThisTask() << " Allocate(" << N << "), exit, Free:" << Free << " Taken:" << Taken << endl;
	uAcceptReturn( Hold, Deallocate, Allocate );		// block until sufficient resources
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
// compile-command: "u++-work SRC2.cc" //
// End: //
