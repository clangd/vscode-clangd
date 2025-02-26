#include <uFile.h>
#include <iostream>
#include <iomanip>
using namespace std;

enum { PIPE_NUM = 510 };
static uPipe pipes[PIPE_NUM];

static unsigned int do_reader_work( uPipe::End &end ) {
	static char buf[8192];
	static uDuration timeout( 5 );

	int nread = end.read( buf, 8192, &timeout );		// read from the pipe

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
		int i;
		double read_mbps;
		unsigned int select_calls = 0, select_fds = 0;
		unsigned int do_reader_bytes = 0;
		uTime start, end;

		cout << setprecision(2) << fixed;
		start = uClock::currTime();

		uDuration pollFreq( 1 );
		cout << "   Time"
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
		try {
			for ( ;; ) {
				for ( i = 0; i < PIPE_NUM; i += 1 ) {
					do_reader_bytes += do_reader_work( pipes[i].left() );
				} // for

				// Statistics.
				select_calls += 1;
				end = uClock::currTime();
				uDuration diff = end - start;
#ifdef __U_STATISTICS__
				tselect_syscalls = UPP::Statistics::select_syscalls - tselect_syscalls;
#endif // __U_STATISTICS__

				//cout << start << " " << end << " " << diff << endl;
	    
				if ( diff >= pollFreq ) {
					read_mbps = do_reader_bytes / diff / 1000000.0;
					cout << setw(7) << diff.nanoseconds() / 1000000000.0
						 << "\t" << setw(7) << select_calls
						 << "\t" << setw(7) << select_fds / select_calls
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
						 << "\t" << setw(7) << UPP::Statistics::select_pending / UPP::Statistics::select_syscalls
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
		} catch( uPipe::End::ReadTimeout & ) {
		} // try

		cout << "Reader:" << &uThisTask() << endl;
	} // Reader::main
  public:
	Reader() {}
	Reader( uCluster &rt ) : uBaseTask( rt ) {}
}; // Reader


const unsigned int NoWriters = 3;
char buffer[8192];										// common data for writers

_Task Writer {
	void main() {
		//osacquire( cerr ) << "Starting writer thread " << &uThistask() << endl;
		uTime start = uClock::currTime(), end;
		uDuration work( 30 );							// work for N seconds
		int pipe = rand() % PIPE_NUM;

		for ( ; uClock::currTime() - start < work; ) {
			// Write a page to a pipe.
			pipes[pipe].right().write( buffer, 8192 );
			pipe += 1;
			if ( pipe >= PIPE_NUM ) pipe = 0;
		} // for
		cout << "Writer:" << &uThisTask() << endl;
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
	UPP::Statistics::prtSigterm = true;
#endif // __U_STATISTICS__

	for ( i = 0; i < sizeof(buffer); i += 1 ) {			// data to written/read
		buffer[i] = 'a';
	} // for

	uCluster wclus[NoWriters] __attribute__(( unused ));
	uProcessor *wcpus[NoWriters];
	{
		Reader reader;									// create reader
		Writer *writers[NoWriters];
		for ( i = 0; i < NoWriters; i += 1 ) {			// create writers
			// uCluster &clus = wclus[i];						// different clusters
			uCluster &clus = uThisCluster();			// same cluster
			wcpus[i] = new uProcessor( clus );
			writers[i] = new Writer( clus );
		} // for
		for ( i = 0; i < NoWriters; i += 1 ) {
			delete writers[i];
			delete wcpus[i];
		} // for
	}
#ifdef __U_STATISTICS__
	UPP::Statistics::print();
#endif // __U_STATISTICS__
}  // main

// Local Variables: //
// compile-command: "u++-work -g -Wall -multi -O2 -DFD_SETSIZE=65536 -D__U_STATISTICS__ Pipes.cc" //
// End: //
