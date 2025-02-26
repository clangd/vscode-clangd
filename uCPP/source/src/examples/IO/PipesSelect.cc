//                              -*- Mode: C++ -*- 
// 
// Copyright (C) Peter A. Buhr 2016
// 
// PipesSelect.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Wed Dec 21 22:17:34 2016
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Apr 22 17:07:30 2022
// Update Count     : 7
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

#include <uFile.h>
#include <iostream>
#include <iomanip>
using namespace std;

enum { PIPE_NUM = 510 };
static uPipe pipes[PIPE_NUM];

static unsigned int do_reader_work( uPipe::End &end ) {
	static char buf[8192];

	int nread = end.read( buf, 8192 );					// read from the pipe

	for ( int i = 0; i < nread; i += 1 ) {				// check transfer
		if ( buf[i] != 'a' ) abort();
	} // for

	// Do some arbitrary work on the buffer.
	// Change the outer loop limit to control the amount of work done per read call.
	for ( int j = 0; j < 1; j += 1 ) {
		for ( int i = 0; i < nread - 3; i += 1 ) {
			buf[i + 1] ^= buf[i];
			buf[i + 2] |= buf[i + 1];
			buf[i + 3] &= buf[i + 2];
		} // for
	} // for

	return nread;
} // do_reader_work


_Task Reader {
	void main() {
		uFloatingPointContext context;					// save/restore floating point registers during context switch
		fd_set rfds;
		int maxfd = 0, nfds;
		int i;
		double read_mbps;
		unsigned int select_calls = 0, select_fds = 0;
		unsigned int do_reader_bytes = 0;
		uTime start, end;

		FD_ZERO( &rfds );
		for ( i = 0; i < PIPE_NUM; i += 1 ) {
			FD_SET( pipes[i].left().fd(), &rfds );
			if ( pipes[i].left().fd() > maxfd ) maxfd = pipes[i].left().fd();
		} // for

		osacquire( cout ) << setprecision(2) << fixed;
		start = uClock::currTime();

		// Run the select loop.

		uDuration pollFreq( 1 );
		timeval t = { 5, 0 };
		osacquire( cout ) << "   Time"
						  << "\tselects"
						  << "\tFDs/Cal"
						  << "\tTP/Mbs"
			//<< "\t VP/swt"
#ifdef __U_STATISTICS__
						  << "\t  maxFD"
						  << "\tSpin(K)"
						  << "\tSpn Sch"
						  << "\t ReadyQ"
						  << "\t MutexQ"
						  << "\tPthredQ"
						  << "\tI/OlokQ"
						  << "\tSel/Evt"
						  << "\tSel/Nul"
						  << "\tSel/tou"
						  << "\tpending"
						  << "\tSel/Cal"
#endif // __U_STATISTICS__
						  << endl;

#ifdef __U_STATISTICS__
		unsigned int tselect_syscalls = 0;
#endif // __U_STATISTICS__
		for ( ;; ) {
		  if ( (nfds = select( maxfd + 1, &rfds, nullptr, nullptr, &t )) <= 0 ) break;

			for ( i = 0; i < PIPE_NUM; i += 1 ) {
				if ( FD_ISSET( pipes[i].left().fd(), &rfds ) ) {
					do_reader_bytes += do_reader_work( pipes[i].left() );
				} else {
					FD_SET( pipes[i].left().fd(), &rfds );
				} // if
			} // for

			// Statistics.
			select_calls += 1;
			select_fds += nfds;
			end = uClock::currTime();
			uDuration diff = end - start;
#ifdef __U_STATISTICS__
			tselect_syscalls = UPP::Statistics::select_syscalls - tselect_syscalls;
#endif // __U_STATISTICS__

			//osacquire( cout ) << start << " " << end << " " << diff << endl;
	    
			if ( diff >= pollFreq ) {
				read_mbps = do_reader_bytes / diff / 1000000.0;
				osacquire( cout ) << setw(7) << diff.nanoseconds() / 1000000000.0
								  << "\t" << setw(7) << select_calls
								  << "\t" << setw(7) << (select_calls != 0 ? select_fds / select_calls : 0)
								  << "\t" << setw(7) << read_mbps
#ifdef __U_STATISTICS__
								  << "\t" << setw(7) << UPP::Statistics::select_maxFD
								  << "\t" << setw(7) << UPP::Statistics::spins / 1000
								  << "\t" << setw(7) << UPP::Statistics::spin_sched
								  << "\t" << setw(7) << UPP::Statistics::ready_queue
								  << "\t" << setw(7) << UPP::Statistics::mutex_queue
								  << "\t" << setw(5) << UPP::Statistics::owner_lock_queue << "/" << UPP::Statistics::adaptive_lock_queue
								  << "\t" << setw(7) << UPP::Statistics::io_lock_queue
								  << "\t" << setw(7) << UPP::Statistics::select_events
								  << "\t" << setw(7) << UPP::Statistics::select_nothing
								  << "\t" << setw(7) << UPP::Statistics::select_blocking
								  << "\t" << setw(7) << (UPP::Statistics::select_syscalls != 0 ? UPP::Statistics::select_pending / UPP::Statistics::select_syscalls : 0)
								  << "\t" << setw(7) << UPP::Statistics::select_syscalls
#endif // __U_STATISTICS__
								  << endl;
				select_calls = select_fds = 0;
				do_reader_bytes = 0;
				start = end;
#ifdef __U_STATISTICS__
				tselect_syscalls = UPP::Statistics::select_syscalls;
#endif // __U_STATISTICS__
			} // if
		} // for

		if ( nfds == -1 ) {
			cerr << "select failure " << errno << " " << strerror( errno ) << endl;
		} // if
		osacquire( cout ) << "Reader:" << &uThisTask() << " terminates" << endl;
	} // Reader::main
  public:
	Reader() {}
	Reader( uCluster &rt ) : uBaseTask( rt ) {}
}; // Reader


const unsigned int NoWriters = 3;
char buffer[8192];										// common data for writers
volatile bool stopWriters = false;

_Task Writer {
	void main() {
		//osacquire( cerr ) << "Starting writer thread " << &uThisTask() << endl;
		int pipe = rand() % PIPE_NUM;

		for ( ; ! stopWriters; ) {
			// Write a page to a pipe.
			pipes[pipe].right().write( buffer, 8192 );
			pipe += 1;
			if ( pipe >= PIPE_NUM ) pipe = 0;
		} // for
		osacquire( cout ) << "Writer:" << &uThisTask() << " terminates" << endl;
	} // Writer::main
  public:
	Writer();
	Writer( uCluster &rt ) : uBaseTask( rt ) {}
}; // Writer


//unsigned int uDefaultPreemption() {
//    return 0;
//} // uDefaultPreemption
//unsigned int uDefaultSpin() {
//    return 500;
//} // uDefaultPreemption

unsigned int uMainStackSize() {
	return 128 * 1000;
} // uMainStackSize


int main() {
	unsigned int i;
#ifdef __U_STATISTICS__
	UPP::Statistics::prtStatTermOn();					// print statistics on termination signal
#endif // __U_STATISTICS__

	for ( i = 0; i < sizeof(buffer); i += 1 ) {			// data to written/read
		buffer[i] = 'a';
	} // for

	// uCluster wclus[NoWriters] __attribute__(( unused )); // different clusters
	uProcessor *wcpus[NoWriters];
	uCluster &clus = uThisCluster();					// same cluster
	for ( i = 0; i < NoWriters; i += 1 ) {				// create processors
	// uCluster &clus = wclus[i];							// different clusters
		wcpus[i] = new uProcessor( clus );
	} // for
	{
		Reader reader;									// create reader
		Writer *writers[NoWriters];
		for ( i = 0; i < NoWriters; i += 1 ) {			// create writers
			//uCluster &clus = wclus[i];					// different clusters
			writers[i] = new Writer( clus );
		} // for

		sleep( 30 );									// writers run for N seconds
		stopWriters = true;

		for ( i = 0; i < NoWriters; i += 1 ) {
			delete writers[i];
		} // for
	}
	for ( i = 0; i < NoWriters; i += 1 ) {
		delete wcpus[i];
	} // for
#ifdef __U_STATISTICS__
//    UPP::Statistics::print();
#endif // __U_STATISTICS__
}  // main

// Local Variables: //
// compile-command: "u++-work -g -Wall -multi -O2 -DFD_SETSIZE=65536 -D__U_STATISTICS__ PipesSelect.cc" //
// End: //
