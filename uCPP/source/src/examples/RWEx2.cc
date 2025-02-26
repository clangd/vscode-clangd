//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// RWEx2.cc -- Readers and Writer Problem
// 
// Author           : Peter A. Buhr
// Created On       : Thu Aug  2 11:51:34 1990
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Dec 19 22:29:02 2016
// Update Count     : 86
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

_Monitor ReadersWriter {
	int rcnt, wcnt;
	uCondition RWers;
	enum RW { READER, WRITER };
  public:
	ReadersWriter() : rcnt(0), wcnt(0) {}

	void startRead() {
		if ( wcnt != 0 || ! RWers.empty() ) RWers.wait( READER );
		rcnt += 1;
	} // ReadersWriter::startRead

	void endRead() {
		rcnt -= 1;
		if ( rcnt == 0 && ! RWers.empty() ) {
			RWers.signal();
		} // if
	} // ReadersWriter::endRead

	void startWrite() {
		if ( wcnt != 0 || rcnt != 0 ) RWers.wait( WRITER );
		wcnt = 1;
	} // ReadersWriter::startWrite

	void endWrite() {
		wcnt = 0;
		if ( ! RWers.empty() ) {
			if ( RWers.front() == WRITER ) {			// writer at front of queue ?
				RWers.signal();
			} else {									// release readers at front of queue
				for ( ;; ) {
					RWers.signal();
				  if ( RWers.empty() ) break;
				  if ( RWers.front() == WRITER ) break; // writer at front of queue ?
				} // for
			} // if
		} // if
	} // ReadersWriter::endWrite
}; // ReadersWriter


volatile int SharedVar = 0;								// shared variable to test readers and writers

_Task Worker {
	ReadersWriter &rw;

	void main() {
		yield( rand() % 100 );							// don't all start at the same time
		if ( rand() % 100 < 70 ) {						// decide to be a reader or writer
			rw.startRead();
			osacquire( cout ) << "Reader:" << this << ", shared:" << SharedVar << endl;
			yield( 3 );
			rw.endRead();
		} else {
			rw.startWrite();
			SharedVar += 1;
			osacquire( cout ) << "Writer:" << this << ",  wrote:" << SharedVar << endl;
			yield( 1 );
			rw.endWrite();
		} // if
	} // Worker::main
  public:
	Worker( ReadersWriter &rw ) : rw( rw ) {
	} // Worker::Worker
}; // Worker


int main() {
	enum { MaxTask = 500 };
	ReadersWriter rw;
	Worker *workers[MaxTask];
	
	for ( int i = 0; i < MaxTask; i += 1 ) {
		workers[i] = new Worker( rw );
	} // for
	for ( int i = 0; i < MaxTask; i += 1 ) {
		delete workers[i];
	} // for

	osacquire( cout ) << "successful completion" << endl;
} // main


// Local Variables: //
// tab-width: 4 //
// compile-command: "u++ RWEx2.cc" //
// End: //
