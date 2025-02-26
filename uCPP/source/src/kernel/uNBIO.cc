//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
//
// uNBIO.cc -- non-blocking IO
//
// Author           : Peter A. Buhr
// Created On       : Mon Mar  7 13:56:53 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Aug 22 22:53:53 2024
// Update Count     : 1515
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

// ***************************************************************************
// WARNING: The poller task uses uYieldNoPoll so that nonlocal exceptions and
// cancellation cannot occur while it is conceptually blocked on an I/O
// operation.
// ***************************************************************************


#define __U_KERNEL__


#include <uC++.h>
#include <uIOcntl.h>
//#include <uDebug.h>

#include <algorithm>
using std::max;
using std::min;

#include <cstring>										// strerror
#include <climits>										// CHAR_BIT
#include <cerrno>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/param.h>									// howmany


namespace UPP {
	uDEBUGPRT(
		static void printFDset( uNBIO *nbio, const char *title, int nmasks, fd_set *fds ) {
			uDebugPrt2( "(uNBIO &)%p.printFDset, masks %d, %s:", nbio, nmasks, title );
			if ( fds != nullptr ) {
				for ( int i = 0; i < nmasks; i += 1 ) {
					uDebugPrt2( "%lx ", fds->fds_bits[i] );
				} // for
			} else {
				uDebugPrt2( "nullptr" );
			} // if
			uDebugPrt2( "\n" );
		} // printFDset
	);

	/****************** countBits ********************
	Purpose: Count number of bits with value 1
	Parameter: unsigned long int (up to 128 bits)
	Reference: Bit Twiddling Hacks: http://graphics.stanford.edu/%7Eseander/bithacks.html#CountBitsSetNaive
	**************************************************/
	static inline int countBits( unsigned long int v ) {
		if ( v == 0 ) return 0;
		typedef __typeof__(v) T;
		v = v - ((v >> 1) & (T)~(T)0/3);				// temp
		v = (v & (T)~(T)0/15*3) + ((v >> 2) & (T)~(T)0/15*3); // temp
		v = (v + (v >> 4)) & (T)~(T)0/255*15;			// temp
		return (T)(v * ((T)~(T)0/255)) >> (sizeof(v) - 1) * CHAR_BIT; // count
	} // countBits

	/****************** msbpos ********************
	Purpose: Find the most significant bit's position (log2 N)
	Parameter: unsigned int
	**************************************************/
#if defined( __GNUC__ )									// GNU gcc compiler ?
// O(1) polymorphic integer log2, using clz, which returns the number of leading 0-bits, starting at the most
// significant bit (single instruction on x86). UNDEFINED FOR 0.
#   define msbpos( n ) ( sizeof(n) * __CHAR_BIT__ - 1 - (				\
							 ( sizeof(n) == sizeof(int) ) ? __builtin_clz( n ) : \
							 ( sizeof(n) == sizeof(long int) ) ? __builtin_clzl( n ) : \
							 ( sizeof(n) == sizeof(long long int) ) ? __builtin_clzll( n ) : \
							 -1 ) )
#else
	int msbpos( int n ) {								// fallback integer log2( n )
		return n > 1 ? 1 + msbpos( n / 2 ) : n == 1 ? 0 : -1;
	}
#endif // __GNUC__

#if 0
	static const int msbpostab[] = {
		-64, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, // -64 allows check for zero mask
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
	}; // msbpostab

	static inline int msbpos( size_t v ) {
		int l;						// l is lg(v)
		register size_t t, tt;
#if __U_WORDSIZE__ == 64
		register size_t ttt;

		if ( (ttt = v >> 32) != 0 ) {					// 64 bit version => dead-code removal
			if ( (tt = ttt >> 16) != 0 ) {
				l = (t = tt >> 8) != 0 ? 56 + msbpostab[t] : 48 + msbpostab[tt];
			} else {
				l = (t = ttt >> 8) != 0 ? 40 + msbpostab[t] : 32 + msbpostab[ttt];
			} // if
		} else
#endif // __U_WORDSIZE__
			if ( (tt = v >> 16) != 0 ) {
				l = (t = tt >> 8) != 0 ? 24 + msbpostab[t] : 16 + msbpostab[tt];
			} else {
				l = (t = v >> 8) != 0 ? 8 + msbpostab[t] : msbpostab[v];
			} // if
		assert( l >= 0 );
		return l;
	} // msbpos
#endif // 0


	/****************** findMaxFD *******************
	Purpose: Find the most significant digit in the mask
	Parameter: previousMaxFD, readMask, writeMask, exceptionMask
	Return: maxFD
	**************************************************/
	static inline int findMaxFD( unsigned int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds ) {
		unsigned int prevMax = nfds;
		nfds = 0;

		fd_mask tmask = howmany( prevMax, NFDBITS ) - 1; // number of "chunks" (words) in previous mask
		fd_mask combined = 0;

		// This code makes assumptions about the implementation of fd_set.  Namely that the size of each chunk is the
		// same as the size of "combined" and the most significant bit contains the highest numbered fd.
		static_assert( (sizeof(combined) * 8) == NFDBITS, "(sizeof(combined) * 8) != NFDBITS" );

		for ( int i = tmask; 0 <= i; i -= 1 ) {			// search backwards from previous max
			if ( rfds != nullptr ) combined |= rfds->fds_bits[i];
			if ( wfds != nullptr ) combined |= wfds->fds_bits[i];
			if ( efds != nullptr ) combined |= efds->fds_bits[i];
		  if ( combined != 0 ) {						// at least one bit set ?
				nfds = msbpos( combined ) + i * NFDBITS + 1;
				break;
			} // if
		} // for
		assert( nfds <= FD_SETSIZE );
		return nfds;
	} // findMaxFD


	//######################### uSelectTimeoutHndlr #########################


	/******************** handler ********************
	Purpose: Handle timeout event
	Effect: For each cluster, if there is an IOPoller, wake up IOPoller to check bit mask
	**************************************************/
	void uNBIO::uSelectTimeoutHndlr::handler() {
		*node.nbioTimeout = node.timedout = true;
		uPid_t temp = cluster.NBIO.IOPollerPid;			// race: IOPollerPid can change to -1 if poller wakes before wakeup
		if ( temp != (uPid_t)-1 ) cluster.wakeProcessor( temp ); // IOPoller set ? => wakeup
	} // uNBIO::uSelectTimeoutHndlr::handler


	//######################### uNBIO #########################


	/***************** checkIOStart ******************
	Purpose: Initialize mask before checking
	Effect: Update master read/write/exception mask from both singleFD mask and multipleFD mask
	**************************************************/
	void uNBIO::checkIOStart() {
		// Combine the single and multiple master masks to form the master mask.

		// get maxFD and minFD from singleFD and multipleFD
		maxFD = max( smaxFD, mmaxFD );
		assert( maxFD != 0 );
#ifdef __U_STATISTICS__
		if ( maxFD > Statistics::select_maxFD ) Statistics::select_maxFD = maxFD;
#endif // __U_STATISTICS__
		unsigned int minFD = min( smaxFD, mmaxFD );

		unsigned int tmasks = howmany( minFD, NFDBITS ); // number of chunks in current mask
		unsigned int i;

		// merge masks up to equal lengths
		// scan each mask individually to prevent random accesses across multile pages
		for ( i = 0; i < tmasks; i += 1 ) mRFDs.fds_bits[i] = srfds.fds_bits[i] | mrfds.fds_bits[i];
		for ( i = 0; i < tmasks; i += 1 ) mWFDs.fds_bits[i] = swfds.fds_bits[i] | mwfds.fds_bits[i];
		if ( efdsUsed )
			for ( i = 0; i < tmasks; i += 1 ) mEFDs.fds_bits[i] = sefds.fds_bits[i] | mefds.fds_bits[i];

		// complete merge by finishing longer mask
		if ( smaxFD > mmaxFD ) {
			unsigned int stmasks = howmany( smaxFD, NFDBITS );
			for ( i = tmasks; i < stmasks; i += 1 ) mRFDs.fds_bits[i] = srfds.fds_bits[i];
			for ( i = tmasks; i < stmasks; i += 1 ) mWFDs.fds_bits[i] = swfds.fds_bits[i];
			if ( efdsUsed )
				for ( i = tmasks; i < stmasks; i += 1 ) mEFDs.fds_bits[i] = sefds.fds_bits[i];
		} else {
			unsigned int mtmasks = howmany( mmaxFD, NFDBITS );
			for ( i = tmasks; i < mtmasks; i += 1 ) mRFDs.fds_bits[i] = mrfds.fds_bits[i];
			for ( i = tmasks; i < mtmasks; i += 1 ) mWFDs.fds_bits[i] = mwfds.fds_bits[i];
			if ( efdsUsed )
				for ( i = tmasks; i < mtmasks; i += 1 ) mEFDs.fds_bits[i] = mefds.fds_bits[i];
		} // if
	} // uNBIO::checkIOStart


	/******************* select **********************
	Purpose: Wait until either one of the file dsecriptors becomes ready, or a signal is delivered
	Effect: Call syscall "pselect" and pass master signal mask
	**************************************************/
	int uNBIO::select( sigset_t *orig_mask ) {
		static timespec timeout_ = { 0, 0 };

#ifdef __U_STATISTICS__
		uFetchAdd( Statistics::select_syscalls, 1 );
		Statistics::select_pending = pending;
#endif // __U_STATISTICS__

		// Safe to make direct accesses through TLS pointer because only called from uNBIO::pollIO, where interrupts are
		// disabled.
		assert( uKernelModule::uKernelModuleBoot.disableInt );
		// maxFD : most significant file descriptor in master mask
		// mRFDs, mWFDs : read/write masks
		// mEFDs : optional exception mask, i.e., do not pass through system call unless necessary (very rare)
		// selectBlock == true  => no ready tasks available so block processor and wait for I/O event(s) to unblock
		//             == false => other tasks to execute so poll for descriptors that occurred while executing,
		//                         polling is specified with a 0 time value
		// orig_mask => original mask before masking SIGALRM/SIGURS1 to provide mutual exclusion, installing this mask
		//              exits mutual exclusion
		descriptors = RealRtn::pselect( maxFD, &mRFDs, &mWFDs, // use library verion
										! efdsUsed ? nullptr : &mEFDs, // no exceptions ?
										selectBlock ? nullptr : &timeout_, orig_mask ); // poll or block ?
		IOPollerPid = (uPid_t)-1;			// reset IOPoller
		return errno;
	} // uNBIO::select


	/******************* pollIO *********************
	Purpose: Perform pollIO actions
	Effect:
	**************************************************/
	bool uNBIO::pollIO( NBIOnode &node ) {
		int terrno;					// temporary errno

		// Note, select occurs outside of the mutex members of NBIO monitor, so that other tasks can enter the
		// monitor and register their interest in other IO events.

		checkIOStart();									// acquires mutual exclusion

		// This processor is about to become idle. First, interrupts are disabled because the following operations
		// affect some kernel data structures.  Second, preemption is turned off because it is now controlled by the
		// "select" timeout.

		uKernelModule::uKernelModuleData::disableInterrupts();

		// disable context switch
		if ( uThisProcessor().getPreemption() != 0 ) {	// optimize out UNIX call if possible
			uThisProcessor().setContextSwitchEvent( 0 ); // turn off preemption for virtual processor to prevent waking UNIX processor
		} // if

		// prevent external tasks (other clusters) from scheduling onto this ready queue
		uThisCluster().readyIdleTaskLock.acquire();

		selectBlock = false;							// default is polling

		// Check processor private and public ready queues, as well as the uNBIO monitor entry-queue to make sure no
		// task managed to slip onto the queues since starting the mutual exclusion.
		if ( ! uThisCluster().readyQueueEmpty() || ! uThisProcessor().external.empty() || ! uEntryList.empty() ) {
			// tasks slipped through, so release mutual exclusion and allow tasks to execute after I/O polling.
			uThisCluster().readyIdleTaskLock.release();
			terrno = select( nullptr );					// poll for descriptors
		} else {
			// Block any SIGALRM/SIGUSR1 signals from external sources, i.e., the system processor performing
			// time-slicing.
			sigset_t new_mask, old_mask;
			sigemptyset( &new_mask );					// clear mask and block SIGALRM/SIGUSR1 interrupts
			sigemptyset( &old_mask );
			sigaddset( &new_mask, SIGALRM );
			sigaddset( &new_mask, SIGUSR1 );
			// race to disable interupt and deliver interrupt
			if ( sigprocmask( SIG_BLOCK, &new_mask, &old_mask ) == -1 ) {
				abort( "internal error, sigprocmask" );
			} // if

			// check if any interrupt occured before interrupts disabled (i.e., interrupt won race)

			// Safe to make direct accesses through TLS pointer because interrupts disabled (see above).
			if ( ! uKernelModule::uKernelModuleBoot.RFinprogress && uKernelModule::uKernelModuleBoot.RFpending ) { // need to start roll forward ?
				// interrupt slipped through, so release mutual exclusion, reset the signal mask, and allow tasks to
				// execute after I/O polling.
				uThisCluster().readyIdleTaskLock.release();
				if ( sigprocmask( SIG_SETMASK, &old_mask, nullptr ) == -1 ) {
					abort( "internal error, sigprocmask" );
				} // if
				terrno = select( nullptr );				// poll for descriptors
			} else {
				// Tasks migrating to a cluster wake a processor. When there is more than one processor on a cluster, do
				// not signal the one blocked on select (by not putting it on the idle list), otherwise there can be a
				// large number of unnecessary EINTR restarts for the select.

				if ( uThisCluster().getProcessors() == 1 ) // must go on idle queue if only process
					uThisCluster().makeProcessorIdle( uThisProcessor() );
				uThisCluster().readyIdleTaskLock.release();

#if ! defined( __U_MULTI__ )
				// okToSelect is set in the uniprocessor kernel when *all* clusters have empty ready queues.
				if ( okToSelect ) {
					okToSelect = false;					// reset select flag
#endif // ! __U_MULTI__

					// common uni/multi code
					selectBlock = true;					// going to block
					// set IOPollerPid so this processor is woken up by arriving I/O requests or timed-out I/O requests
					IOPollerPid = uThisProcessor().getPid();
#ifdef __U_STATISTICS__
					uFetchAdd( Statistics::select_blocking, 1 );
#endif // __U_STATISTICS__

#if ! defined( __U_MULTI__ )
				} // if
#endif // ! __U_MULTI__

				// select unblocked because of SIGALRM
				if ( timeoutOccurred ) selectBlock = false;

				terrno = select( &old_mask );

				if ( sigprocmask( SIG_SETMASK, &old_mask, nullptr ) == -1 ) { // new mask restored so install old signal mask over new one
					abort( "internal error, sigprocmask" );
				} // if

				if ( uThisProcessor().idle() )
					uThisCluster().makeProcessorActive( uThisProcessor() );
			} // if
		} // if

		if ( uThisProcessor().getPreemption() != 0 ) {	// optimize out UNIX call if possible
			uThisProcessor().setContextSwitchEvent( uThisProcessor().getPreemption() ); // reset processor preemption time
		} // if

		// rollForward is called before exit by enableInterrupts

		uKernelModule::uKernelModuleData::enableInterrupts(); // does necessary roll forward

		return checkIOEnd( node, terrno );				// acquires mutual exclusion
	} // uNBIO::pollIO


	/******************* performIO **********************
	Purpose: Perform IO select operation
	Effect:
	**************************************************/
	void uNBIO::performIO( int fd, NBIOnode *p, uSequence<NBIOnode> &pendingIO, int cnt ) {
		// The IOPoller tries the IO operations for each thread waiting for an event that has occurred rather than
		// restart the thread and let it recheck the IO operation. This should result in a lower cost. If the IO
		// operation completes, the corresponding thread is unblocked. A IO operation can fail when multiple threads
		// wait on the same FD, and earlier checked threads steal the IO from later checked threads. For failing IO
		// operations, the FD completion bit in the master mask is cleared, and the corresponding IO event is reset in
		// the appropriate mask.

		p->smfd.sfd.closure->wrapper();
		if ( p->smfd.sfd.closure->retcode == -1 && p->smfd.sfd.closure->errno_ == U_EWOULDBLOCK ) {
			if ( *p->smfd.sfd.uRWE & uCluster::ReadSelect ) {
				FD_CLR( fd, &mRFDs );					// remove bit from master mask so no other task is woken
				FD_SET( fd, &srfds );					// reset single master for pending tasks on next select
			} // if
			if ( *p->smfd.sfd.uRWE & uCluster::WriteSelect ) {
				FD_CLR( fd, &mWFDs );					// remove bit from master mask so no other task is woken
				FD_SET( fd, &swfds );					// reset single master for pending tasks on next select
			} // if
			if ( efdsUsed )
				if ( *p->smfd.sfd.uRWE & uCluster::ExceptSelect ) {
					FD_CLR( fd, &mEFDs );				// remove bit from master mask so no other task is woken
					FD_SET( fd, &sefds );				// reset single master for pending tasks on next select
				} // if
		} else {
			uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.performIO, removing node %p, cnt:%d, timedout:%d\n", this, p, cnt, p->timedout ); );
			pendingIO.remove( p );						// remove node from list of waiting tasks
			p->nfds = cnt;								// set return value
			p->pending.V();								// wake up waiting task (empty for IOPoller)
			pending -= 1;
		} // if
	} // uNBIO::performIO


	void uNBIO::checkSfds( int fd, NBIOnode *p, uSequence<NBIOnode> &pendingIO ) {
		int temp = 0, cnt = 0;

		uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.checkSfds, found task %.256s (%p) waiting on single fd %d with mask 0x%x\n",
							  this, p->pendingTask->getName(), p->pendingTask, fd, *p->smfd.sfd.uRWE ); );

		// Determine all IO events registered by a task.
		if ( (*p->smfd.sfd.uRWE & uCluster::ReadSelect) && FD_ISSET( fd, &mRFDs ) ) {
			temp |= uCluster::ReadSelect;
			cnt += 1;
		} // if
		if ( (*p->smfd.sfd.uRWE & uCluster::WriteSelect ) && FD_ISSET( fd, &mWFDs ) ) {
			temp |= uCluster::WriteSelect;
			cnt += 1;
		} // if
		if ( efdsUsed )
			if ( (*p->smfd.sfd.uRWE & uCluster::ExceptSelect) && FD_ISSET( fd, &mEFDs ) ) {
				temp |= uCluster::ExceptSelect;
				cnt += 1;
			} // if

		// cnt == 0 => master mask-bit turned off after executing the wrapper for a prior task
		if ( cnt != 0 ) {								// I/O possible for task so perform operation on behalf of waiting task
			*p->smfd.sfd.uRWE = temp;
			performIO( fd, p, pendingIO, cnt );
		} else if ( p->timedout ) {						// timed out (set by event handler) ? not needed for single fds without timeout
			uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.checkSfds, removing node %p, cnt:%d, timedout:%d\n", this, p, cnt, p->timedout ); );
			pendingIO.remove( p );						// remove node from list of waiting tasks
			p->nfds = cnt;								// set return value
			p->pending.V();								// wake up waiting task (empty for IOPoller)
			pending -= 1;
		} // if
	} // uNBIO::checkSfds


	/******************* unblockFD **********************
	Purpose: unblock the pending IO from the waiting queue
	Effect: next pending task becomes IOPoller and will be waked up
	**************************************************/
	void uNBIO::unblockFD( uSequence<NBIOnode> &pendingIO ) {
#ifdef __U_STATISTICS__
		uFetchAdd( Statistics::iopoller_exchange, 1 );
#endif // __U_STATISTICS__
		NBIOnode *p = pendingIO.head();
		IOPoller = p->pendingTask;						// next poller task
		p->pending.V();									// wake up waiting task
		uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.unblockFD, poller %.256s (%p) nominating task %.256s (%p) to be next poller\n",
							  this, uThisTask().getName(), &uThisTask(), (IOPoller != nullptr ? IOPoller->getName() : "nullptr"), IOPoller ); );
	} // uNBIO::unblockFD


	bool uNBIO::checkIOEnd( NBIOnode &node, int terrno ) {
		unsigned int i, tcnt, cnt;
		unsigned int tmasks;
		NBIOnode *p;

		uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.checkIOEnd, select returns: found %d\n", this, descriptors ); );

		if ( descriptors > 0 ) {						// I/O has occurred (from pselect) ?
#ifdef __U_STATISTICS__
			uFetchAdd( Statistics::select_events, descriptors );
#endif // __U_STATISTICS__

			uDEBUGPRT( tmasks = howmany( maxFD, NFDBITS ); // total number of masks in fd set
					   uDebugAcquire();
					   uDebugPrt2( "(uNBIO &)%p.checkIOEnd, found %d pending IO operations\n", this, descriptors );
					   printFDset( this, "mRFDs", tmasks, &mRFDs ); printFDset( this, "mWFDs", tmasks, &mWFDs ); printFDset( this, "mEFDs", tmasks, &mEFDs );
					   uDebugRelease(); );

			// Check to see which tasks are waiting for ready I/O operations on multiple mask and wake them.

			bool multiples = false;
			for ( uSeqIter<NBIOnode> iter( pendingIOMfds ); iter >> p; ) { // multiple fds & single fds with timeout
				if ( p->fdType == NBIOnode::singleFd ) { // single fd
					checkSfds( p->smfd.sfd.closure->access.fd, p, pendingIOMfds );
				} else {								// multiple fds
					tcnt = 0;
					if ( ! multiples ) {				// only clear if there were some in the list
						FD_ZERO( &mrfds );				// clear the read set
						FD_ZERO( &mwfds );				// clear the write set
						if ( efdsUsed )
							FD_ZERO( &mefds );			// clear the exceptional set
						multiples = true;
					} // if
					uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.checkIOEnd, found task %.256s (%p) waiting for multiple fd, nfd:%d\n",
										  this, p->pendingTask->getName(), p->pendingTask, p->smfd.mfd.tnfds ); );
					// "min" is necessary because new tasks can enter after a select occurs, so maxFD does not reflect
					// the current max.
					tmasks = howmany( min( p->smfd.mfd.tnfds, maxFD ), NFDBITS ); // total number of masks in fd set

					// this mask prevents bits in the user fdset from being changed when the returned fdset is shorter
					int shift = p->smfd.mfd.tnfds % NFDBITS;
					if ( shift == 0 ) shift = NFDBITS;
					fd_mask mask = (~0ul) << shift;		// (all 1s) mask for interest bits

					uDEBUGPRT(
						uDebugAcquire();
						uDebugPrt2( "(uNBIO &)%p.checkIOEnd Multi before, tmasks %d\n", this, tmasks );
						printFDset( this, "trfds", tmasks, p->smfd.mfd.trfds );
						printFDset( this, "twfds", tmasks, p->smfd.mfd.twfds );
						printFDset( this, "tefds", tmasks, p->smfd.mfd.tefds );
						uDebugRelease();
					);

					fd_mask temp;
					tcnt = cnt = 0;

					if ( p->smfd.mfd.trfds != nullptr ) { // non-null user mask ?
						for ( i = 0; i < tmasks - 1; i += 1 ) {
							temp = p->smfd.mfd.trfds->fds_bits[i] & mRFDs.fds_bits[i];
							if ( temp != 0 ) cnt += countBits( temp ); // some bits on ?
						} // for
						// special case for last mask in the bit set
						temp = p->smfd.mfd.trfds->fds_bits[i] & ( ~mask & mRFDs.fds_bits[i] );
						if ( temp != 0 ) {
							cnt += countBits( temp );	// some bits on ?
						} // if
						if ( cnt != 0 ) {
							for ( i = 0; i < tmasks - 1; i += 1 ) p->smfd.mfd.trfds->fds_bits[i] &= mRFDs.fds_bits[i];
							p->smfd.mfd.trfds->fds_bits[i] = temp | ( mask & p->smfd.mfd.trfds->fds_bits[i] );
						} // if
					} // if
					tcnt += cnt;

					cnt = 0;
					if ( p->smfd.mfd.twfds != nullptr ) { // non-null user mask ?
						for ( i = 0; i < tmasks - 1; i += 1 ) {
							temp = p->smfd.mfd.twfds->fds_bits[i] & mWFDs.fds_bits[i];
							if ( temp != 0 ) cnt += countBits( temp ); // some bits on ?
						} // for
						// special case for last mask in the bit set
						temp = p->smfd.mfd.twfds->fds_bits[i] & ( ~mask & mWFDs.fds_bits[i] );
						if ( temp != 0 ) {
							cnt += countBits( temp );	// some bits on ?
						} // if
						if ( cnt != 0 ) {
							for ( i = 0; i < tmasks - 1; i += 1 ) p->smfd.mfd.twfds->fds_bits[i] &= mWFDs.fds_bits[i];
							p->smfd.mfd.twfds->fds_bits[i] = temp | ( mask & p->smfd.mfd.twfds->fds_bits[i] );
						} // if
					} // if
					tcnt += cnt;

					cnt = 0;
					if ( p->smfd.mfd.tefds != nullptr ) { // non-null user mask ?
						for ( i = 0; i < tmasks - 1; i += 1 ) {
							temp = p->smfd.mfd.tefds->fds_bits[i] & mEFDs.fds_bits[i];
							if ( temp != 0 ) cnt += countBits( temp ); // some bits on ?
						} // for
						// special case for last mask in the bit set
						temp = p->smfd.mfd.tefds->fds_bits[i] & ( ~mask & mEFDs.fds_bits[i] );
						if ( temp != 0 ) {
							cnt += countBits( temp );	// some bits on ?
						} // if
						if ( cnt != 0 ) {
							for ( i = 0; i < tmasks - 1; i += 1 ) p->smfd.mfd.tefds->fds_bits[i] &= mEFDs.fds_bits[i];
							p->smfd.mfd.tefds->fds_bits[i] = temp | ( mask & p->smfd.mfd.tefds->fds_bits[i] );
						} // if
					} // if
					tcnt += cnt;

					uDEBUGPRT(
						uDebugAcquire();
						uDebugPrt2( "(uNBIO &)%p.checkIOEnd Multi after, tcnt:%d\n", this, tcnt );
						printFDset( this, "trfds", tmasks, p->smfd.mfd.trfds );
						printFDset( this, "twfds", tmasks, p->smfd.mfd.twfds );
						printFDset( this, "tefds", tmasks, p->smfd.mfd.tefds );
						uDebugRelease();
					);

					if ( tcnt != 0 || p->timedout ) { // I/O completed for this task or timed out (set by event handler) ?
						uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.checkIOEnd, removing node %p for task %s (%p), tcnt:%d, timedout:%d\n",
											  this, p, p->pendingTask->getName(), p->pendingTask, tcnt, p->timedout ); )
							pendingIOMfds.remove( p ); // remove node from list of waiting tasks
						p->nfds = tcnt;					// set return value
						p->pending.V();					// wake up waiting task (empty for IOPoller)
						pending -= 1;
					} else {							// task is not waking up
						tmasks = howmany( p->smfd.mfd.tnfds, NFDBITS );
						if ( p->smfd.mfd.trfds != nullptr )
							for ( i = 0; i < tmasks; i += 1 ) mrfds.fds_bits[i] |= p->smfd.mfd.trfds->fds_bits[i];
						if ( p->smfd.mfd.twfds != nullptr )
							for ( i = 0; i < tmasks; i += 1 ) mwfds.fds_bits[i] |= p->smfd.mfd.twfds->fds_bits[i];
						if ( p->smfd.mfd.tefds != nullptr )
							for ( i = 0; i < tmasks; i += 1 ) mefds.fds_bits[i] |= p->smfd.mfd.tefds->fds_bits[i];

						uDEBUGPRT(
							uDebugAcquire();
							uDebugPrt2( "(uNBIO &)%p.checkIOEnd multiple set\n", this );
							printFDset( this, "mrfds", tmasks, &mrfds ); printFDset( this, "mwfds", tmasks, &mwfds ); printFDset( this, "mefds", tmasks, &mefds );
							uDebugRelease();
						);
					} // if
				} // if
			} // for

			if ( multiples ) {							// check if multiple maxFD needs to be reset
				unsigned int last = mmaxFD - 1;
				if ( ! ( FD_ISSET( last, &mrfds ) || FD_ISSET( last, &mwfds ) || (
							 efdsUsed &&
							 FD_ISSET( last, &mefds ) ) ) ) {
					mmaxFD = findMaxFD( mmaxFD, &mrfds, &mwfds,
										! efdsUsed ? nullptr :
										&mefds );
				} // if
			} // if
			uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.checkIOEnd multiple mmaxFD:%d\n", this, mmaxFD ); );

			// Check to see which tasks are waiting for ready I/O operations on single mask and wake them.

			tmasks = howmany( smaxFD, NFDBITS );		// total number of masks in fd set

			uDEBUGPRT(
				uDebugAcquire();
				uDebugPrt2( "(uNBIO &)%p.checkIOEnd single set before smaxFD:%d\n", this, smaxFD );
				printFDset( this, "srfds", tmasks, &srfds ); printFDset( this, "swfds", tmasks, &swfds ); printFDset( this, "sefds", tmasks, &sefds );
				uDebugRelease();
			);

			unsigned long int combined;
			for ( i = 0; i < tmasks; i += 1 ) {			// single fds with no timeout
				// assumes the bits after the maxFD bit are unchanged by the OS when the mask is returned
				combined = mRFDs.fds_bits[i] | mWFDs.fds_bits[i];
				if ( efdsUsed )
					combined |= mEFDs.fds_bits[i];

				if ( combined == 0 ) continue;			// no bits set in chunk ?

				srfds.fds_bits[i] &= ~mRFDs.fds_bits[i];
				swfds.fds_bits[i] &= ~mWFDs.fds_bits[i];
				if ( efdsUsed )
					sefds.fds_bits[i] &= ~mEFDs.fds_bits[i];

				uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.checkIOEnd %d  combined:%lx  single %lx %lx %lx  master %lx %lx %lx \n",
									  this, i, combined, srfds.fds_bits[i], swfds.fds_bits[i], sefds.fds_bits[i], mRFDs.fds_bits[i], mWFDs.fds_bits[i], mEFDs.fds_bits[i] ); );

				// process each bit in the combined chunks
				for ( int fd = i * NFDBITS - 1; combined != 0; ) { // fd is origin 0 so substract 1
#if __U_WORDSIZE__ == 64								// 64 bit architecture with ffsl
					int posn = ffsl( combined );
#elif __U_WORDSIZE__ == 32								// 32 bit architecture with ffs
					int posn = ffs( combined );
#else													// 64 bit architecture without ffsl
					unsigned long int temp = combined & 0xffffffff; // extract least significant 32 bits
					int posn = temp != 0 ? ffs( temp ) : ffs( combined >> 32 ) + 32;
#endif // ffs
					if ( posn == NFDBITS ) combined = 0; // shift of word size is nop
					else combined >>= posn;
					fd += posn;

					// process each task waiting for this fd's events, list can be empty due to timeout
					for ( uSeqIter<NBIOnode> iter( pendingIOSfds[fd] ); iter >> p; ) {
						checkSfds( fd, p, pendingIOSfds[fd] );
					} // for
				} // for
			} // for

			smaxFD = findMaxFD( smaxFD, &srfds, &swfds,
								! efdsUsed ? nullptr :
								&sefds );

			uDEBUGPRT( tmasks = howmany( smaxFD, NFDBITS ); // total number of masks in fd set
					   uDebugAcquire();
					   uDebugPrt2( "(uNBIO &)%p.checkIOEnd single set after smaxFD:%d\n", this, smaxFD );
					   printFDset( this, "srfds", tmasks, &srfds ); printFDset( this, "swfds", tmasks, &swfds ); printFDset( this, "sefds", tmasks, &sefds );
					   uDebugRelease();
			);

		} else if ( descriptors == 0 ) {	// time limit expired, no IO is ready
			uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.checkIOEnd, time limit expired\n", this ); );
#ifdef __U_STATISTICS__
			uFetchAdd( Statistics::select_nothing, 1 );
#endif // __U_STATISTICS__

			if ( timeoutOccurred ) {					// non-polling timeout ?
				timeoutOccurred = false;

				// Check for timed-out IO

				NBIOnode *p;
				for ( uSeqIter<NBIOnode> iter( pendingIOMfds ); iter >> p; ) {
					if ( p->timedout ) {				// timed out waiting for I/O for this task ?
						uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.checkIOEnd, removing node %p for task %s (%p)\n", this, p, p->pendingTask->getName(), p->pendingTask ); );
						pendingIOMfds.remove( p );		// remove node from list of waiting tasks
						p->nfds = 0;					// set return value
						p->pending.V();					// wake up waiting task (empty for IOPoller)
						pending -= 1;
					} // if
				} // for
			} // if
		} else {
			uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.checkIOEnd, error, errno:%d %s\n", this, terrno, strerror( terrno ) ); );
#ifdef __U_STATISTICS__
			uFetchAdd( Statistics::select_errors, 1 );
#endif // __U_STATISTICS__
			// Either an EINTR occurred or one of the clients specified a bad file number, and a EBADF was received.
			// This is handled by waking up all the clients, telling them that IO has occured so that they will retry
			// their IO operation and get the error message at that point.  They can recover from their errors at that
			// point more gracefully than can be done here.

			if ( terrno == EINTR ) {
				// probably sigalrm from migrate, do nothing
#ifdef __U_STATISTICS__
				uFetchAdd( Statistics::select_eintr, 1 );
#endif // __U_STATISTICS__
			} else if ( terrno == EBADF ) {
				// Received an unexpected error, chances are that one of the tasks has fouled up a call to some IO
				// routine. Wake up all the tasks that were waiting for IO, allow them to retry their IO call and hope
				// they catch the error this time.

				for ( unsigned int fd = 0; fd < smaxFD; fd += 1 ) { // single fd with no timeout
					// process each task waiting for this fd's events, list can be empty due to timeout
					for ( uSeqIter<NBIOnode> iter( pendingIOSfds[fd] ); iter >> p; ) {
						performIO( fd, p, pendingIOSfds[fd], -1 );
					} // for
				} // for
				smaxFD = 0;

				bool multiples = false;
				NBIOnode *p;
				for ( uSeqIter<NBIOnode> iter( pendingIOMfds ); iter >> p; ) { // multiple fds & single fd with timeout
					if ( p->fdType == NBIOnode::singleFd ) { // single fd ?
						int fd = p->smfd.sfd.closure->access.fd;
						performIO( fd, p, pendingIOMfds, -1 );
					} else {
						multiples = true;
						pendingIOMfds.remove( p );		// remove node from list of waiting tasks
						p->nfds = -1;					// mark the fact that something is wrong
						p->pending.V();					// wake up waiting task (empty for IOPoller)
						pending -= 1;
					} // if
				} // for
				if ( multiples ) {						// only clear if there were some in the list
					FD_ZERO( &mrfds );					// clear the read set
					FD_ZERO( &mwfds );					// clear the write set
					if ( efdsUsed )
						FD_ZERO( &mefds );				// clear the exceptional set
					mmaxFD = 0;
				} // if
			} else {
				abort( "(uNBIO &)%p.checkIOEnd() : internal error, maxFD:%d error(%d) %s.", this, maxFD, terrno, strerror( terrno ) );
			} // if
		} // if

		// If the IOPoller's I/O completed, attempt to nominate another waiting
		// task to be the IOPoller.

		if ( ! node.listed() ) {						// IOPoller's node removed ?
			if ( ! pendingIOMfds.empty() ) {			// any other tasks waiting for I/O event on a general FD mask?
				unblockFD( pendingIOMfds );
			} else {
				if ( smaxFD == 0 || pendingIOSfds[smaxFD - 1].empty() ) {
					IOPoller = nullptr;
				} else {
					unblockFD( pendingIOSfds[smaxFD - 1] );
				} // if
			} // if
			return false;
		} else {
#ifdef __U_STATISTICS__
			uFetchAdd( Statistics::iopoller_spin, 1 );
#endif // __U_STATISTICS__
			uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.checkIOEnd, poller %.256s (%p) continuing to poll\n", this, uThisTask().getName(), &uThisTask() ); );
			return true;
		} // if
	} // uNBIO::checkIOEnd


	/******************* checkPoller **********************
	Purpose: Check if first task to register interest
	Effect: If first task to register interest, task becomes IOPoller
	Return: return true if first task, false otherwise.
	**************************************************/
	bool uNBIO::checkPoller() {
		if ( IOPoller == nullptr ) {
			IOPoller = &uThisTask();					// make this task the poller

			uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.checkPoller, set poller task %.256s (%p)\n", this, IOPoller->getName(), IOPoller ); );
			return true;
		} // if
		uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.checkPoller, blocking non-poller I/O task %.256s (%p))\n", this, uThisTask().getName(), &uThisTask() ); );

		return false;
	} // uNBIO::checkPoller


	void uNBIO::waitOrPoll( NBIOnode &node, uEventNode *timeoutEvent ) {
		if ( ! initSfd( node, timeoutEvent ) ) {		// single bit ?
			node.pending.P();
			if ( ! node.listed() ) return;				// not poller task ?
		} // if
		while ( pollIO( node ) ) uThisTask().uYieldNoPoll(); // busy wait
	} // uNBIO::waitOrPoll


	void uNBIO::waitOrPoll( unsigned int nfds, NBIOnode &node, uEventNode *timeoutEvent ) {
		if ( ! initMfds( nfds, node, timeoutEvent ) ) {	// multiple bits ?
			node.pending.P();
			if ( ! node.listed() ) return;				// not poller task ?
		} // if
		while ( pollIO( node ) ) uThisTask().uYieldNoPoll(); // busy wait
	} // uNBIO::waitOrPoll


	bool uNBIO::initSfd( NBIOnode &node, uEventNode *timeoutEvent ) {
		unsigned int fd = node.smfd.sfd.closure->access.fd; // optimization

		if ( fd >= smaxFD ) {							// increase maxFD if necessary
			smaxFD = fd + 1;
		} // if

		// set the appropriate fd bit in the master fd mask
		if ( *node.smfd.sfd.uRWE & uCluster::ReadSelect ) FD_SET( fd, &srfds );
		if ( *node.smfd.sfd.uRWE & uCluster::WriteSelect ) FD_SET( fd, &swfds );
		if ( *node.smfd.sfd.uRWE & uCluster::ExceptSelect ) {
			efdsUsed = true;
			FD_SET( fd, &sefds );
		} // if

		uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.initSfd, adding node %p for fd %d\n", this, &node, fd ); )

			if ( timeoutEvent != nullptr ) {
				timeoutEvent->add();
				pendingIOMfds.addTail( &node );			// node is removed by IOPoller
			} else {
				pendingIOSfds[fd].addTail( &node );		// node is removed by IOPoller
			} // if

		uPid_t temp = IOPollerPid;						// race: IOPollerPid can change to -1 if poller wakes before wakeup
		if ( temp != (uPid_t)-1 ) uThisCluster().wakeProcessor( temp );
		pending += 1;
		return checkPoller();
	} // uNBIO::initSfd


	bool uNBIO::initMfds( unsigned int nfds, NBIOnode &node, uEventNode *timeoutEvent ) {
		if ( nfds > mmaxFD ) {							// increase maxFD if necessary
			mmaxFD = nfds;
		} // if

		// set the appropriate fd bits in the master fd mask; mask pointers can be null => nothing in that mask
		unsigned int tmask = howmany( nfds, NFDBITS );
		if ( node.smfd.mfd.trfds != nullptr ) {
			for ( unsigned int i = 0; i < tmask; i += 1 ) {
				mrfds.fds_bits[i] |= node.smfd.mfd.trfds->fds_bits[i];
			} // for
		} // if
		if ( node.smfd.mfd.twfds != nullptr ) {
			for ( unsigned int i = 0; i < tmask; i += 1 ) {
				mwfds.fds_bits[i] |= node.smfd.mfd.twfds->fds_bits[i];
			} // for
		} // if
		if ( node.smfd.mfd.tefds != nullptr ) {
			efdsUsed = true;
			for ( unsigned int i = 0; i < tmask; i += 1 ) {
				mefds.fds_bits[i] |= node.smfd.mfd.tefds->fds_bits[i];
			} // for
		} // if

		uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.initMfds, adding node %p\n", this, &node ); );

		if ( timeoutEvent != nullptr ) {
			timeoutEvent->add();
		} // if

		pendingIOMfds.addTail( &node );					// node is removed by IOPoller

		uPid_t temp = IOPollerPid;						// race: IOPollerPid can change to -1 if poller wakes before wakeup
		if ( temp != (uPid_t)-1 ) uThisCluster().wakeProcessor( temp );
		pending += 1;
		return checkPoller();
	} // uNBIO::initMfds


	uNBIO::uNBIO() {
		uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.uNBIO\n", this ); );
		FD_ZERO( &srfds );								// clear the read set
		FD_ZERO( &swfds );								// clear the write set
		FD_ZERO( &sefds );								// clear the exceptional set
		FD_ZERO( &mrfds );								// clear the read set
		FD_ZERO( &mwfds );								// clear the write set
		FD_ZERO( &mefds );								// clear the exceptional set
		efdsUsed = false;								// efds set not used
		smaxFD = 0;										// all masks are clear
		mmaxFD = 0;										// all masks are clear
		pending = 0;
		IOPoller = nullptr;								// no poller task
		IOPollerPid = (uPid_t)-1;						// IOPoller not blocked on a processor
		timeoutOccurred = false;
#if ! defined( __U_MULTI__ )
		okToSelect = false;
#endif // ! __U_MULTI__
	} // uNBIO::uNBIO


	int uNBIO::select( uIOClosure &closure, int &rwe, timeval *timeout ) {
		uDEBUGPRT(
			uDebugAcquire();
			uDebugPrt2( "(uNBIO &)%p.select1, fd:%d, rwe:0x%x", this, closure.access.fd, rwe );
			if ( timeout != nullptr ) uDebugPrt2( ", timeout:%ld.%ld", timeout->tv_sec, timeout->tv_usec );
			uDebugPrt2( "\n" );
			uDebugRelease();
		);

		if ( closure.access.fd < 0 || FD_SETSIZE <= closure.access.fd ) {
			abort( "Attempt to select on file descriptor %d that exceeds range 0-%d.",
				   closure.access.fd, FD_SETSIZE - 1 );
		} // if

		NBIOnode node;
		node.pending.P();
		node.nfds = 0;
		node.pendingTask = &uThisTask();
		node.fdType = NBIOnode::singleFd;
		node.timedout = false;
		node.nbioTimeout = &timeoutOccurred;
		node.smfd.sfd.closure = &closure;
		node.smfd.sfd.uRWE = &rwe;

		if ( timeout != nullptr ) {			// timeout ?
			if ( timeout->tv_sec == 0 && timeout->tv_usec == 0 ) { // optimization
				// It is unnecessary to create a timeout event for this trivial case. Just mark the timeout as having
				// already occurred.

				node.timedout = true;
				waitOrPoll( node );
			} else {
				uDuration delay( timeout->tv_sec, timeout->tv_usec * 1000 );
				uTime time = uClock::currTime() + delay;
				uSelectTimeoutHndlr handler( uThisTask(), node );

				uEventNode timeoutEvent( uThisTask(), handler, time ); // event node for event list
				timeoutEvent.executeLocked = true;

				waitOrPoll( node, &timeoutEvent );

				// always doing remove guarantees the node is not deallocated prematurely
				timeoutEvent.remove();
			} // if
		} else {
			waitOrPoll( node );
		} // if

		uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.select1, exits, cnt:%d\n", this, node.nfds ); );
		return node.nfds;
	} // uNBIO::select


	int uNBIO::select( int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, timeval *timeout ) {
		uDEBUGPRT(
			unsigned int tmasks = howmany( FD_SETSIZE, NFDBITS ); // total number of masks in fd set
			uDebugAcquire();
			uDebugPrt2( "(uNBIO &)%p.select2 enter, nfds:%d\n", this, nfds );
			printFDset( this, "rfds", tmasks, rfds );
			printFDset( this, "wfds", tmasks, wfds );
			printFDset( this, "efds", tmasks, efds );
			if ( timeout != nullptr ) uDebugPrt2( "(uNBIO &)%p.select2, timeout:%ld.%ld\n", this, timeout->tv_sec, timeout->tv_usec );
			uDebugRelease();
		);

#ifdef __U_DEBUG__
		if ( nfds < 1 || FD_SETSIZE < nfds ) {
			abort( "Attempt to select with a file descriptor set size of %ld that exceeds range 0-%d.",
				   (long int)nfds, FD_SETSIZE );
		} // if
#endif // __U_DEBUG__

		NBIOnode node;
		node.pending.P();
		node.nfds = 0;
		node.pendingTask = &uThisTask();
		node.fdType = NBIOnode::multipleFds;
		node.timedout = false;
		node.nbioTimeout = &timeoutOccurred;
		node.smfd.mfd.tnfds = nfds;
		node.smfd.mfd.trfds = rfds;
		node.smfd.mfd.twfds = wfds;
		node.smfd.mfd.tefds = efds;

		if ( timeout != nullptr ) {						// timeout ?
			if ( timeout->tv_sec == 0 && timeout->tv_usec == 0 ) { // optimization
				// It is unnecessary to create a timeout event for this trivial case. Just mark the timeout as having
				// already occurred.

				node.timedout = true;
				waitOrPoll( nfds, node );
			} else {
				uDuration delay( timeout->tv_sec, timeout->tv_usec * 1000 );
				uTime time = uClock::currTime() + delay;
				uSelectTimeoutHndlr handler( uThisTask(), node );

				uEventNode timeoutEvent( uThisTask(), handler, time ); // event node for event list
				timeoutEvent.executeLocked = true;

				waitOrPoll( nfds, node, &timeoutEvent );

				// always doing remove guarantees the node is not deallocated prematurely
				timeoutEvent.remove();
			} // if
		} else {
			waitOrPoll( nfds, node );
		} // if

		uDEBUGPRT( uDebugPrt( "(uNBIO &)%p.select2, exits, cnt:%d\n", this, node.nfds ); );
		return node.nfds;
	} // uNBIO::select
} // UPP


extern "C" int select( int nfds, fd_set *rfd, fd_set *wfd, fd_set *efd, timeval *timeout ) { // replace
	return uThisCluster().select( nfds, rfd, wfd, efd, timeout );
} // select


extern "C" int pselect( int nfds, fd_set *rfd, fd_set *wfd, fd_set *efd, const timespec *timeout, const sigset_t *sigmask ) { // interpose (need original)
	sigset_t old_mask;
	sigemptyset( &old_mask );
	if ( sigprocmask( SIG_BLOCK, sigmask, &old_mask ) == -1 ) {
	    abort( "internal error, sigprocmask" );
	} // if

	int ready;
	if ( timeout == nullptr ) {
	    ready = select( nfds, rfd, wfd, efd, nullptr );
	} else {
		timeval ttime = { timeout->tv_sec, timeout->tv_nsec / 1000 };
		ready = select( nfds, rfd, wfd, efd, &ttime );
	} // if

	if ( sigprocmask( SIG_SETMASK, &old_mask, nullptr ) == -1 ) {
	    abort( "internal error, sigprocmask" );
	} // if

	return ready;
} // pselect


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
// Problem with __fortified_attr_access causing warning in g++-14
//   extern int poll (struct pollfd *__fds, nfds_t __nfds, int __timeout)
//            __fortified_attr_access (__write_only__, 1, 2);
extern "C" int poll( struct pollfd *fds, nfds_t nfds, int timeout ) { // replace
	fd_set rfd, wfd, efd;
	int maxfd = -1;

	FD_ZERO( &rfd );
	FD_ZERO( &wfd );
	FD_ZERO( &efd );

	for ( unsigned int i = 0; i < nfds; i += 1 ) {
		if ( fds[i].fd < 0 ) continue;
		if ( ( fds[i].events & ~( POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM | POLLPRI |
								  POLLERR | POLLHUP | POLLNVAL ) ) != 0 ) { // output only so ignore
			abort( "poll: unknown event requested %x", fds[i].events );
		} // if
		if ( fds[i].fd > maxfd ) maxfd = fds[i].fd;

		uDEBUGPRT( uDebugPrt( "poll( %p, %lu, %d ): fd %d ask ", fds, nfds, timeout, fds[i].fd ); );
		if ( fds[i].events & POLLIN || fds[i].events & POLLRDNORM ) {
			FD_SET( fds[i].fd, &rfd );
		} // if
		if ( fds[i].events & POLLOUT || fds[i].events & POLLWRNORM ) {
			FD_SET( fds[i].fd, &wfd );
		} // if
		if ( fds[i].events & POLLPRI ) {
			FD_SET( fds[i].fd, &efd );
		} // if
	} // for

	if ( timeout >= 0 ) {
		timeval ttime = { timeout / 1000, timeout % 1000 * 1000 };
		if ( select( maxfd + 1, &rfd, &wfd, &efd, &ttime ) < 0 ) return -1;
	} else {
		if ( select( maxfd + 1, &rfd, &wfd, &efd, nullptr ) < 0 ) return -1;
	} // if

	int nresults = 0;
	for ( unsigned int i = 0; i < nfds; i += 1 ) {
		fds[i].revents = 0;
		if ( FD_ISSET( fds[i].fd, &rfd ) || FD_ISSET( fds[i].fd, &wfd ) || FD_ISSET( fds[i].fd, &efd ) ) {
			nresults += 1;
			if ( FD_ISSET( fds[i].fd, &rfd ) ) {
				fds[i].revents |= fds[i].events & ( POLLIN | POLLRDNORM );
			} // if
			if ( FD_ISSET( fds[i].fd, &wfd ) ) {
				fds[i].revents |= fds[i].events & ( POLLOUT | POLLWRNORM );
			} // if
			if ( FD_ISSET( fds[i].fd, &efd ) ) {
				fds[i].revents |= POLLPRI;
			} // if
		} // if
	} // for

	uDEBUGPRT( uDebugPrt( "poll( %p, %lu, %d ) returns %d\n", fds, nfds, timeout, nresults ); );
	return nresults;
} // poll
#pragma GCC diagnostic pop


extern "C" int ppoll( struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts, const sigset_t *sigmask ) { // replace
	int timeout = (timeout_ts == nullptr) ? -1 : (timeout_ts->tv_sec * 1000 + timeout_ts->tv_nsec / 1000000);

	sigset_t old_mask;
	sigemptyset( &old_mask );

	if ( sigprocmask( SIG_BLOCK, sigmask, &old_mask ) == -1 ) {
	    abort( "internal error, sigprocmask" );
	} // if

	int ready = poll( fds, nfds, timeout );

	if ( sigprocmask( SIG_SETMASK, &old_mask, nullptr ) == -1 ) {
	    abort( "internal error, sigprocmask" );
	} // if

	return ready;
} // ppoll

// Local Variables: //
// compile-command: "make install" //
// End: //
