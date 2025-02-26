//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2001
// 
// pthread.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sun Dec  9 21:38:53 2001
// Last Modified By : Peter A. Buhr
// Last Modified On : Wed Aug  9 20:23:38 2023
// Update Count     : 1198
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

// see http://nptl.bullopensource.org/Tests/Optimization-level-in-nptl.html

#define __U_KERNEL__
#include <uC++.h>
#include <uSystemTask.h>
#include <uSequence.h>
#include <csignal>										// access: sigset_t
#include <cerrno>										// access: EBUSY, ETIMEDOUT
#include <cstdlib>										// access: exit
#include <cstdio>										// access: fprintf
#include <unistd.h>										// access: _exit
#include <pthread.h>
#include <limits.h>										// access: PTHREAD_KEYS_MAX
#include <uStack.h>

//#include <uDebug.h>

#define NOT_A_PTHREAD ((pthread_t)-2)					// used as return from pthread_self for non-pthread tasks

namespace UPP {
	struct Pthread_values : public uSeqable {			// thread specific data
		bool in_use;
		void * value;
	}; // Pthread_values

	struct Pthread_keys {								// all of these fields are initialized with zero
		bool in_use;
		void (* destructor)( void * );
		uSequence<Pthread_values> threads;
	}; // Pthread_keys

	// Create storage separately to ensure no constructors are called.
//	char u_pthread_keys_storage[sizeof(Pthread_keys) * PTHREAD_KEYS_MAX] __attribute__((aligned (16))) = {0};
//	#define u_pthread_keys ((Pthread_keys *)u_pthread_keys_storage)
	static uNoCtor<Pthread_keys> u_pthread_keys[sizeof(Pthread_keys) * 8 * 1024 /*PTHREAD_KEYS_MAX*/];

	static pthread_mutex_t u_pthread_keys_lock = PTHREAD_MUTEX_INITIALIZER;
	static pthread_mutex_t u_pthread_once_lock = PTHREAD_MUTEX_INITIALIZER;

	struct Pthread_kernel_threads : public uColable {
		uProcessor processor;
		Pthread_kernel_threads() : processor( true ) {}	// true => detached
	}; // Pthread_kernel_threads

	uStack<Pthread_kernel_threads> u_pthreads_kernel_threads;
	static bool u_pthreads_kernel_threads_zero = false;	// set to zero ?
	static int u_pthreads_no_kernel_threads = 1;		// minimum of one processor per cluster
	static pthread_mutex_t u_pthread_concurrency_lock = PTHREAD_MUTEX_INITIALIZER;


	//######################### PthreadPid #########################


	class PthreadPid {
		friend _Task ::uPthreadable;

		template<bool selector>
		class Impl {
			// empty implementation -- one of the specializations is always used
		}; // Impl
	  public:
		static int create( pthread_t * new_thread_id, const pthread_attr_t * attr, void * (* start_func)( void * ), void * arg );
		static void join( pthread_t threadID );
		static ::uPthreadable * lookup( pthread_t threadID );
		static pthread_t rev_lookup( ::uPthreadable * addr );
		static void destroy();
	}; // PthreadPid


	template<>
	class PthreadPid::Impl<true> {
		// best implementation, when PublicPid is large enough to hold a Pid
		friend class PthreadPid;
		friend _Task ::uPthreadable;

		static pthread_t create( ::uPthreadable * p ) {
			return (pthread_t)p;
		} // PthreadPid::Impl::create

		static void recycle( pthread_t /* threadID */ ) {
			// nothing to do
		} // PthreadPid::Impl::recycle

		static ::uPthreadable * lookup( pthread_t threadID ) {
			return (::uPthreadable *)threadID;
		} // PthreadPid::lookup

		static pthread_t rev_lookup( ::uPthreadable * addr ) {
			return (pthread_t)(long)addr;
		} // PthreadPid::rev_lookup

		static void destroy() {
			// nothing to do
		} // PthreadPid::Impl::destroy
	}; // PthreadPid::Impl


	template<>
	class PthreadPid::Impl<false> {
		// fall-back implementation when pthread_t is too small

		#define U_POWER( n ) (1 << (n))
		#define U_PID_DIRECTORY_SIZE 8					// bits
		#define U_PID_NODE_SIZE 8						// bits (must be > 1)
		#define U_PID_MAX U_POWER(U_PID_DIRECTORY_SIZE + U_PID_NODE_SIZE)
		#define U_PID_NODE_NUMBER (U_POWER(U_PID_NODE_SIZE) - 2) // leave room for malloc header

		friend class PthreadPid;
		friend _Task ::uPthreadable;

		static_assert( U_PID_DIRECTORY_SIZE + U_PID_NODE_SIZE < sizeof(unsigned int) * 8, "" );

		static pthread_mutex_t create_lock;
		static ::uPthreadable ** directory[U_POWER(U_PID_DIRECTORY_SIZE)];
		static unsigned int free_elem;
		static bool first;
		static unsigned int size;

		static void init_block( unsigned int dir ) {
			::uPthreadable ** block = directory[ dir ];
			int i;

			dir <<= U_PID_NODE_SIZE;
			for ( i = 0; i < U_PID_NODE_NUMBER - 1; i += 1 ) {
				block[ i ] = (::uPthreadable *)(long)( dir | (i + 1) );
			} // for
			// insert "nullptr" value at end of free list
			block[ i ] = (::uPthreadable *)(long)(U_POWER(U_PID_DIRECTORY_SIZE + U_PID_NODE_SIZE) | dir);
		} // PthreadPid::Impl::init_block

		static unsigned int alloc( ::uPthreadable * tid ) {
			unsigned int block = free_elem & (U_POWER(U_PID_NODE_SIZE) - 1),
				dir = free_elem >> U_PID_NODE_SIZE;

			if ( free_elem >= U_POWER(U_PID_DIRECTORY_SIZE + U_PID_NODE_SIZE) ) {
				dir = (U_POWER(U_PID_DIRECTORY_SIZE) - 1) & dir; // remove high-order end-marker
				if ( dir == (U_POWER(U_PID_DIRECTORY_SIZE) - 1) ) {
					return UINT_MAX;
				} else {
					if ( first ) {
						first = false;
					} else {
						dir += 1;
					} // if
					directory[ dir ] = new ::uPthreadable *[U_PID_NODE_NUMBER];
					init_block( dir );
					free_elem = dir << U_PID_NODE_SIZE;
				} // if
			} // if

			unsigned int rtn = free_elem;
			free_elem = (unsigned int)(long)directory[ dir ][ block ];
			directory[ dir ][ block ] = tid;

			return rtn;
		} // PthreadPid::Impl::alloc

		static void free( uintptr_t pid ) {
			unsigned int block = pid & (U_POWER(U_PID_NODE_SIZE) - 1), dir = pid >> U_PID_NODE_SIZE;

			directory[ dir ][ block ] = (::uPthreadable *)(long)free_elem;
			free_elem = pid;
		} // PthreadPid::Impl::free

		static pthread_t create( ::uPthreadable * p ) {
			pthread_mutex_lock( &create_lock );
			if ( size == U_PID_MAX ) {
				pthread_mutex_unlock( &create_lock );
				_Throw ::uPthreadable::CreationFailure();
			} // if
			unsigned int allocated = alloc( p );
			assert( allocated != UINT_MAX );
			size += 1;
			pthread_mutex_unlock( &create_lock );
			return (pthread_t)allocated;
		} // PthreadPid::Impl::create

		static void recycle( pthread_t threadID ) {
			pthread_mutex_lock( &create_lock );
			free( (uintptr_t)threadID );
			size -= 1;
			pthread_mutex_unlock( &create_lock );
		} // PthreadPid::Impl::recycle

		static ::uPthreadable * lookup( pthread_t pid ) {
			return directory[ (uintptr_t)pid >> U_PID_DIRECTORY_SIZE ][ (uintptr_t)pid & (U_POWER(U_PID_NODE_SIZE) - 1) ];
		} // PthreadPid::lookup

		static pthread_t rev_lookup( ::uPthreadable * addr ) {
			return addr->pthreadId();
		} // PthreadPid::rev_lookup

		static void destroy() {
			for ( int i = 0; i < U_POWER(U_PID_DIRECTORY_SIZE) && directory[ i ] != nullptr; i += 1 ) {
				delete [] directory[ i ];
			} // for
		} // PthreadPid::Impl::destroy
	}; // PthreadPid::Impl


	pthread_mutex_t PthreadPid::Impl<false>::create_lock = PTHREAD_MUTEX_INITIALIZER;
	::uPthreadable ** PthreadPid::Impl<false>::directory[U_POWER(U_PID_DIRECTORY_SIZE)];
	unsigned int PthreadPid::Impl<false>::free_elem = U_POWER(U_PID_DIRECTORY_SIZE + U_PID_NODE_SIZE);
	bool PthreadPid::Impl<false>::first = true;
	unsigned int PthreadPid::Impl<false>::size = 0;


	//######################### Pthread #########################


	_Task uPthread : public ::uPthreadable {			// private type
		void main() {
			CancelSafeFinish dummy( *this );
			joinval = (*start_func)( arg );
		} // uPthread::main

		void * (* start_func)( void * );				// thread starting routine
		void * arg;										// thread parameter
	  public:
		uPthread( pthread_t * threadId, void * (* start_func)( void * ), const pthread_attr_t * attr, void * arg ) : ::uPthreadable( attr ), start_func( start_func ), arg( arg ) {
			*threadId = pthreadId();					// publish thread id
		} // uPthread::uPthread

		~uPthread() {
		} // uPthread::~uPthread

		_Nomutex void finishUp() {
			int detachstate;
			pthread_attr_getdetachstate( &pthread_attr, &detachstate );
			if ( detachstate == PTHREAD_CREATE_DETACHED ) {
				uKernelModule::systemTask->pthreadDetachEnd( *this );
			} // if
		} // uPthread::finishUp

		// execute some uPthread finalizations even if cancellation/exit/exception happens    
		class CancelSafeFinish {
			uPthread &p;
		  public:
			CancelSafeFinish( uPthread &p ) : p( p ) {}
			~CancelSafeFinish() {
				p.finishUp();
			} // CancelSafeFinish::~CancelSafeFinish
		}; // CancelSafeFinish
	}; // uPthread


	//######################### PthreadPid (cont.) ##################
 

	inline int PthreadPid::create( pthread_t * new_thread_id, const pthread_attr_t * attr, void * (* start_func)( void * ), void * arg ) {
		try {
			new uPthread( new_thread_id, start_func, attr, arg );
		} catch ( ::uPthreadable::CreationFailure & ) {
			return EAGAIN;								// Pthread creation can only fail due to too many pthread instances
		} // try

		return 0;
	} // PthreadPid::create

	inline void PthreadPid::join( pthread_t threadID ) {
		delete dynamic_cast< uPthread *>( lookup( threadID ) ); // delete Pthreads but not uPthreadables
	} // PthreadPid::join

	inline ::uPthreadable * PthreadPid::lookup( pthread_t threadID ) {
		return Impl< sizeof(pthread_t) >= sizeof(::uPthreadable *) >::lookup( threadID );
	} // PthreadPid::lookup

	inline pthread_t PthreadPid::rev_lookup( ::uPthreadable * addr ) {
		return Impl< sizeof(pthread_t) >= sizeof(::uPthreadable *) >::rev_lookup( addr );
	} // PthreadPid::rev_lookup

	inline void PthreadPid::destroy() {
		return Impl< sizeof(pthread_t) >= sizeof(::uPthreadable *) >::destroy();
	} // PthreadPid::destroy


	//######################### PthreadLock #########################


	class PthreadLock {
		template<typename Lock, bool selector>
		class Impl {
			// empty implementation -- one of the specializations is always used
		}; // Impl
	  public:
		template<typename Lock, typename PublicLock>
		static void init( PublicLock * lock ) {
			Impl< Lock, sizeof( PublicLock ) >= sizeof( Lock ) >::init( lock );
		} // PthreadLock::init

		template<typename Lock, typename PublicLock>
		static void destroy( PublicLock * lock ) {
			Impl< Lock, sizeof( PublicLock ) >= sizeof( Lock ) >::destroy( lock );
		} // PthreadLock::destroy

		template<typename Lock, typename PublicLock>
		static Lock * get( PublicLock * lock ) {
			return Impl< Lock, sizeof( PublicLock ) >= sizeof( Lock ) >::get( lock );
		} // PthreadLock::get
	}; // PthreadLock


	template<typename Lock>
	class PthreadLock::Impl<Lock, true> {
		// best implementation when PublicLock is large enough to hold a Lock
		friend class PthreadLock;

		template< typename PublicLock >
		static void init( PublicLock * lock ) {
			new( lock ) Lock;							// run constructor on supplied storage
		} // PthreadLock::Impl::init

		template< typename PublicLock >
		static void destroy( PublicLock * lock ) {
			((Lock *)lock)->~Lock();					// run destructor on supplied storage
		} // PthreadLock::Impl::destroy

		template< typename PublicLock >
		static Lock * get( PublicLock * lock ) {
			return (Lock*)lock;
		} // PthreadLock::Impl::get
	}; // PthreadLock::Impl


	template<typename Lock>
	class PthreadLock::Impl<Lock, false> {
		// fall-back implementation when PublicLock is too small to hold a Lock
		friend class PthreadLock;

		struct storage {
			Lock * lock;
		}; // storage

		enum { MaxLockStorage = 256 };
		static char lockStorage[MaxLockStorage] __attribute__(( aligned (16) ));
		static char * next;
		static bool first;

		template< typename PublicLock >
		static void init( PublicLock * lock ) {
			if ( uHeapControl::initialized() ) {		// normal execution
				((storage *)lock)->lock = new Lock;
			} else {
				if ( first ) {							// first time ?
					first = false;
					uHeapControl::startup();
					((storage *)lock)->lock = new Lock;
				} else {								// squential execution
					if ( next + sizeof(Lock) >= &lockStorage[MaxLockStorage] ) {
						#define errStr "pthread_mutex_init() : internal error, exceeded maximum boot locks during initialization.\n"
						write( STDERR_FILENO, errStr, sizeof( errStr) - 1 );
						_exit( EXIT_FAILURE );
					} // if
					((storage *)lock)->lock = (Lock *)next;
					new( ((storage *)lock)->lock ) Lock; // run constructor on supplied storage

					next += sizeof(Lock);				// assume lock size is multiple of alignment size
				} // if
			} // if
		} // PthreadLock::Impl::init

		template< typename PublicLock >
		static void destroy( PublicLock * lock ) {
			if ( (char *)lock >= &lockStorage[MaxLockStorage] ) { // normal execution, assume heap after stack
				delete ((storage *)lock)->lock;
			} // if
		} // PthreadLock::Impl::destroy

		template< typename PublicLock >
		static Lock * get( PublicLock * lock ) {
			static char magic_check[sizeof(uOwnerLock)] __attribute__(( aligned (16) )); // set to zero
			// Use double check to improve performance. Check is safe on x86; volatile prevents compiler reordering
			volatile PublicLock * const lock_ = lock;
			Lock * l = ((storage *)lock_)->lock;
			// check is necessary because storage can be statically initialized
			if ( l == nullptr ) {
				((uOwnerLock *)&magic_check)->acquire(); // race
				l = ((storage *)lock_)->lock;
				if ( l == nullptr ) {
					init( lock );
					l = ((storage *)lock)->lock;		// get the newly initialized pointer
					assert( l != nullptr );
				} // if
				((uOwnerLock *)&magic_check)->release();
			} // if
			return l;
		} // PthreadLock::Impl::get
	}; // PthreadLock::Impl

	template<typename T> char PthreadLock::Impl<T,false>::lockStorage[MaxLockStorage];
	template<typename T> char * PthreadLock::Impl<T,false>::next = PthreadLock::Impl<T,false>::lockStorage;
	template<typename T> bool PthreadLock::Impl<T,false>::first = true;
} // UPP


using namespace UPP;


//######################### uPthreadable #########################


const uPthreadable::Pthread_attr_t uPthreadable::u_pthread_attr_defaults = {
	PTHREAD_SCOPE_SYSTEM,
	PTHREAD_CREATE_JOINABLE,
	__U_DEFAULT_STACK_SIZE__,
	nullptr,
	0,
	PTHREAD_EXPLICIT_SCHED,
	{ 0 }
};

void uPthreadable::createPthreadable( const pthread_attr_t * attr_ ) {
	uexc.exception_class = 0;							// any class will work
	uexc.exception_cleanup = restart_unwinding;			// makes sure cancellation cannot be stopped
	stop_unwinding = false;
	attr = attr_ != nullptr ? *get( attr_ ) : u_pthread_attr_defaults;
	get( &pthread_attr ) = &attr;						// point factious pthread attr at real attr
	// obtain a pid during construction. Can fail with a CreationFailure exception
	pthreadId_ = PthreadPid::Impl< sizeof(pthread_t) >= sizeof(uPthreadable *) >::create( this );
} // uPthreadable::createPthreadable

uPthreadable::~uPthreadable() {
	PthreadPid::Impl< sizeof(pthread_t) >= sizeof(uPthreadable *) >::recycle( pthreadId_ ); // clean up ID
} // uPthreadable::~uPthreadable

// unwinder_cleaner gets called after each unwinding step Assumptions: memory/call stack grows DOWN !

_Unwind_Reason_Code uPthreadable::unwinder_cleaner( int /* version */, _Unwind_Action /* actions */,_Unwind_Exception_Class /* exc_class */,
													_Unwind_Exception * /* exc_obj */, _Unwind_Context * context, void * arg ) {
	uPthreadable * p = (uPthreadable *)arg;
	void * cfa = (void *)_Unwind_GetCFA(context);		// identify the current stack frame
	uBaseCoroutine::PthreadCleanup * cst = p->cleanupStackTop(); // find the address of the top cleanup information

	while ( cst != nullptr && cst <= cfa ) {			// as long as there are cleanup handlers
		p->cleanup_pop(1);								// keep popping off cleanups and execute them
		cst = p->cleanupStackTop();
	} // while

	// continue the unwinding until we reach the catch(...) inside invokeTask()
	return _URC_NO_REASON;
} // uPthreadable::unwinder_cleaner

void uPthreadable::restart_unwinding( _Unwind_Reason_Code /* urc */, _Unwind_Exception * /* e */ ) {
	uPthreadable * p = dynamic_cast< uPthreadable *>(&uThisTask());
	if ( ! p->stop_unwinding ) {
		p->do_unwind();									// unless turned off in uMachContext, continue unwinding after this exception has been caught
	} // if
} // uPthreadable::restart_unwinding

void uPthreadable::do_unwind() {
	uThisCoroutine().cancelInProgress_ = true;
	_Unwind_ForcedUnwind( &uexc, unwinder_cleaner, this );
} // uPthreadable::do_unwind

void uPthreadable::cleanup_push( void (* routine)(void *), void * args, void * stackaddress ) {
	uBaseCoroutine::PthreadCleanup * buf = new(stackaddress) uBaseCoroutine::PthreadCleanup; // use the buffer data structure (on the stack) to store information
	buf->routine = routine;
	buf->args = args;
	cleanup_handlers.add( buf );

	static_assert( sizeof(_pthread_cleanup_buffer) >= sizeof(uBaseCoroutine::PthreadCleanup), "" );
} // uPthreadable::cleanup_push

void uPthreadable::cleanup_pop( int ex ) {
	uBaseCoroutine::PthreadCleanup * buf;
	assert( ! cleanup_handlers.empty() );
	buf = cleanup_handlers.drop();
	if ( ex ) {
		buf->routine( buf->args );
	} // if
} // uPthreadable::cleanup_pop


// must be called only from within uMain or a Pthread
uBaseCoroutine::PthreadCleanup * uPthreadable::cleanupStackTop() {
	return cleanup_handlers.head();
} // uPthreadable::cleanupStackTop


uPthreadable::Failure::Failure() {
}; // uPthreadable::Failure
 

//######################### pthread simulation #########################


extern "C" {
	//######################### Creation #########################


	void pthread_pid_destroy_( void ) {					// see uKernelBoot::finishup
		PthreadPid::destroy();
	} // pthread_pid_destroy_


	int pthread_create( pthread_t * new_thread_id, const pthread_attr_t * attr, void * (* start_func)( void * ), void * arg ) __THROW {
		return PthreadPid::create( new_thread_id, attr, start_func, arg );
	} // pthread_create

	int pthread_attr_init( pthread_attr_t * attr ) __THROW {
		// storage for pthread_attr_t must be >= void *
		static_assert( sizeof(pthread_attr_t) >= sizeof(void *), "sizeof(pthread_attr_t) < sizeof(void *)" );
		uPthreadable::get( attr ) = new uPthreadable::Pthread_attr_t;
		*uPthreadable::get( attr ) = uPthreadable::u_pthread_attr_defaults; // set all fields to default values
		return 0;
	} //  pthread_attr_init

	int pthread_attr_destroy( pthread_attr_t * attr ) __THROW {
		delete uPthreadable::get( attr );
		return 0;
	} // pthread_attr_destroy

	int pthread_attr_setscope( pthread_attr_t * attr, int contentionscope ) __THROW {
		uPthreadable::get( attr )->contentionscope = contentionscope;
		return 0;
	} // pthread_attr_setscope

	int pthread_attr_getscope( const pthread_attr_t * attr, int * contentionscope ) __THROW {
		*contentionscope = uPthreadable::get( attr )->contentionscope;
		return 0;
	} // pthread_attr_getscope

	int pthread_attr_setdetachstate( pthread_attr_t * attr, int detachstate ) __THROW {
		uPthreadable::get( attr )->detachstate = detachstate;
		return 0;
	} // pthread_attr_setdetachstate

	int pthread_attr_getdetachstate( const pthread_attr_t * attr, int * detachstate ) __THROW {
		*detachstate = uPthreadable::get( attr )->detachstate;
		return 0;
	} // pthread_attr_getdetachstate

	int pthread_attr_setstacksize( pthread_attr_t * attr, size_t stacksize ) __THROW {
		uPthreadable::get( attr )->stacksize = stacksize;
		return 0;
	} // pthread_attr_setstacksize

	int pthread_attr_getstacksize( const pthread_attr_t * attr, size_t * stacksize ) __THROW {
		*stacksize = uPthreadable::get( attr )->stacksize;
		return 0;
	} // pthread_attr_getstacksize

	int pthread_attr_getguardsize( const pthread_attr_t * /* attr */, size_t * /* guardsize */ ) __THROW {
		return 0;
	} // pthread_attr_getguardsize

	int pthread_attr_setguardsize( pthread_attr_t * /* attr */, size_t /* guardsize */ ) __THROW {
		return 0;
	} // pthread_attr_setguardsize

	int pthread_attr_setstackaddr( pthread_attr_t * attr, void * stackaddr ) __THROW {
		uPthreadable::get( attr )->stackaddr = stackaddr;
		return 0;
	} // pthread_attr_setstackaddr

	int pthread_attr_getstackaddr( const pthread_attr_t * attr, void ** stackaddr ) __THROW {
		*stackaddr = uPthreadable::get( attr )->stackaddr;
		return 0;
	} // pthread_attr_getstackaddr

	int pthread_attr_setstack( pthread_attr_t * attr, void * stackaddr, size_t stacksize ) __THROW {
		uPthreadable::get( attr )->stackaddr = stackaddr;
		uPthreadable::get( attr )->stacksize = stacksize;
		return 0;
	} // pthread_attr_setstack

	int pthread_attr_getstack( const pthread_attr_t * attr, void ** stackaddr, size_t * stacksize ) __THROW {
		*stackaddr = uPthreadable::get( attr )->stackaddr;
		*stacksize = uPthreadable::get( attr )->stacksize;
		return 0;
	} // pthread_attr_getstack

	// Initialize thread attribute *attr with attributes corresponding to the already running thread threadID. It shall
	// be called on unitialized attr and destroyed with pthread_attr_destroy when no longer needed.
	int pthread_getattr_np( pthread_t threadID, pthread_attr_t * attr ) __THROW { // GNU extension
		assert( threadID != NOT_A_PTHREAD );
		// race condition during copy
		pthread_attr_init( attr );
		*uPthreadable::get( attr ) = PthreadPid::lookup( threadID )->attr; // copy all fields
		return 0;
	} // pthread_getattr_np

	int pthread_attr_setschedpolicy( pthread_attr_t * attr, int policy ) __THROW {
		uPthreadable::get( attr )->policy = policy;
		return ENOSYS;					// unsupported
	} // pthread_attr_setschedpolicy

	int pthread_attr_getschedpolicy( const pthread_attr_t * attr, int * policy ) __THROW {
		*policy = uPthreadable::get( attr )->policy;
		return 0;
	} // pthread_attr_getschedpolicy

	int pthread_attr_setinheritsched( pthread_attr_t * attr, int inheritsched ) __THROW {
		uPthreadable::get( attr )->inheritsched = inheritsched;
		return ENOSYS;
	} // pthread_attr_setinheritsched

	int pthread_attr_getinheritsched( const pthread_attr_t * attr, int * inheritsched ) __THROW {
		*inheritsched = uPthreadable::get( attr )->inheritsched;
		return 0;
	} // pthread_attr_getinheritsched

	int pthread_attr_setschedparam( pthread_attr_t * attr, const struct sched_param * param ) __THROW {
		uPthreadable::get( attr )->param = *param;
		return ENOSYS;
	} // pthread_attr_setschedparam

	int pthread_attr_getschedparam( const pthread_attr_t * attr, struct sched_param * param ) __THROW {
		*param = uPthreadable::get( attr )->param;
		return 0;
	} // pthread_attr_getschedparam

	int pthread_yield( void ) __THROW {					// GNU extension
		uThisTask().uYieldNoPoll();
		pthread_testcancel();							// pthread_yield is a cancellation point
		return 0;
	} // pthread_yield


	//######################### Exit #########################


	void pthread_exit( void * status ) {
		uPthreadable * p = dynamic_cast<uPthreadable *>(&uThisCoroutine());
#ifdef __U_DEBUG__
		if ( p == nullptr ) abort( "pthread_exit performed on non-pthread task or while executing a uC++ coroutine" );
#endif // __U_DEBUG__
		p->joinval = status;
		p->do_unwind();
		// CONTROL NEVER REACHES HERE!
		abort( "pthread_exit( %p ) : internal error, attempt to return.", status );
	} // pthread_exit

	int pthread_join( pthread_t threadID, void ** status ) {
		assert( threadID != NOT_A_PTHREAD );
		pthread_testcancel();							// pthread_join is a cancellation point
		uPthreadable * p = PthreadPid::lookup( threadID );
	  if ( p->attr.detachstate != PTHREAD_CREATE_JOINABLE ) return EINVAL;
	  if ( pthread_self() == threadID ) return EDEADLK;
		// join() is only necessary for status; otherwise synchronization occurs at delete.
		if ( status != nullptr ) *status = p->join();
		PthreadPid::join( threadID );
		return 0;
	} // pthread_join

	int pthread_tryjoin_np( pthread_t threadID, void ** status ) __THROW { // GNU extension
		uPthreadable * p = PthreadPid::lookup( threadID );
	  if ( p->attr.detachstate != PTHREAD_CREATE_JOINABLE ) return EINVAL;
	  if ( pthread_self() == threadID ) return EDEADLK;
	  	// race condition during check
	  if ( p->getState() != uBaseTask::Terminate ) return EBUSY; // thread not finished ?
		if ( status != nullptr ) *status = p->join();
		PthreadPid::join( threadID );
		return 0;
	} // pthread_tryjoin_np

	int pthread_timedjoin_np( pthread_t /* threadID */, void ** /* status */, const struct timespec * /* abstime */ ) { // GNU extension
		abort( "pthread_timedjoin_np not implemented" );
		return 0;
	} // pthread_timedjoin_np

	int pthread_detach( pthread_t threadID ) __THROW {
		// There is always a race condition in setting the detach flag, even if
		// "detach" is accessed with mutual exclusion.
		PthreadPid::lookup( threadID )->attr.detachstate = PTHREAD_CREATE_DETACHED;
		return 0;
	} // pthread_detach


	//######################### Parallelism #########################


	void pthread_delete_kernel_threads_() __THROW {		// see uMain::~uMain
		Pthread_kernel_threads * p;
		for ( uStackIter<Pthread_kernel_threads> iter( u_pthreads_kernel_threads ); iter >> p; ) {
			delete p;
		} // for
	} // pthread_delete_kernel_threads_

	int pthread_getconcurrency( void ) __THROW {		// XOPEN extension
		return u_pthreads_kernel_threads_zero ? 0 : u_pthreads_no_kernel_threads;
	} // pthread_getconcurrency

	int pthread_setconcurrency( int new_level ) __THROW { // XOPEN extension
	  if ( new_level < 0 ) return EINVAL;
	  if ( new_level == 0 ) {
			u_pthreads_kernel_threads_zero = true;		// remember set to zero, but ignore
			return 0;									// uC++ does not do kernel thread management
		} // exit
		u_pthreads_kernel_threads_zero = false;
		pthread_mutex_lock( &u_pthread_concurrency_lock );
		for ( ; new_level > u_pthreads_no_kernel_threads; u_pthreads_no_kernel_threads += 1 ) { // add processors ?
			u_pthreads_kernel_threads.push( new Pthread_kernel_threads );
		} // for
		for ( ; new_level < u_pthreads_no_kernel_threads; u_pthreads_no_kernel_threads -= 1 ) { // remove processors ?
			Pthread_kernel_threads * p = u_pthreads_kernel_threads.pop();
			delete p;
		} // for
		pthread_mutex_unlock( &u_pthread_concurrency_lock );
		return 0;
	} // pthread_setconcurrency


	//######################### Thread Specific Data #########################


	void pthread_deletespecific_( void * pthreadData ) __THROW { // see uMachContext::invokeTask
		pthread_mutex_lock( &u_pthread_keys_lock );
		Pthread_values * values = (Pthread_values *)pthreadData;

		// If, after all the destructors have been called for all non-null values with associated destructors, there are
		// still some non-null values with associated destructors, then the process is repeated. If, after at least
		// PTHREAD_DESTRUCTOR_ITERATIONS iterations of destructor calls for outstanding non-null values, there are still
		// some non-null values with associated destructors, implementations may stop calling destructors, or they may
		// continue calling destructors until no non-null values with associated destructors exist, even though this
		// might result in an infinite loop.

		bool destcalled = true;
		for ( int attempts = 0; attempts < PTHREAD_DESTRUCTOR_ITERATIONS && destcalled ; attempts += 1 ) {
			destcalled = false;
			for ( int i = 0; i < PTHREAD_KEYS_MAX; i += 1 ) { // remove each value from key list
				if ( values[i].in_use ) {
					uDEBUGPRT( uDebugPrt( "pthread_deletespecific_, value[%d].in_use:%d, value:%p\n", i, values[i].in_use, values[i].value ); );
					Pthread_values &entry = values[i];
					entry.in_use = false;
					u_pthread_keys[i]->threads.remove( &entry );
					if ( u_pthread_keys[i]->destructor != nullptr && entry.value != nullptr ) {
						void * data = entry.value;
						entry.value = nullptr;
						uDEBUGPRT( uDebugPrt( "pthread_deletespecific_, task:%p, destructor:%p, value:%p begin\n",
											  &uThisTask(), u_pthread_keys[i].destructor, data ); );
						destcalled = true;
						u_pthread_keys[i]->destructor( data );
						uDEBUGPRT( uDebugPrt( "pthread_deletespecific_, task:%p, destructor:%p, value:%p end\n",
											  &uThisTask(), u_pthread_keys[i].destructor, data ); );
					} // if
				} // if
			} // for
		} // for
		pthread_mutex_unlock( &u_pthread_keys_lock );
		delete [] values;
	} // pthread_deletespecific_


	int pthread_key_create( pthread_key_t * key, void (* destructor)( void * ) ) __THROW {
		uDEBUGPRT( uDebugPrt( "pthread_key_create(key:%p, destructor:%p) enter task:%p\n", key, destructor, &uThisTask() ); );
		pthread_mutex_lock( &u_pthread_keys_lock );
		for ( int i = 0; i < PTHREAD_KEYS_MAX; i += 1 ) {
			if ( ! u_pthread_keys[i]->in_use ) {
				u_pthread_keys[i]->in_use = true;
				u_pthread_keys[i]->destructor = destructor;
				pthread_mutex_unlock( &u_pthread_keys_lock );
				*key = i;
				uDEBUGPRT( uDebugPrt( "pthread_key_create(key:%d, destructor:%p) exit task:%p\n", *key, destructor, &uThisTask() ); );
				return 0;
			} // if
		} // for
		pthread_mutex_unlock( &u_pthread_keys_lock );
		uDEBUGPRT( uDebugPrt( "pthread_key_create(key:%p, destructor:%p) try again!!! task:%p\n", key, destructor, &uThisTask() ); );
		return EAGAIN;
	} // pthread_key_create

	int pthread_key_delete( pthread_key_t key ) __THROW {
		uDEBUGPRT( uDebugPrt( "pthread_key_delete(key:0x%x) enter task:%p\n", key, &uThisTask() ); );
		pthread_mutex_lock( &u_pthread_keys_lock );
		if ( key >= PTHREAD_KEYS_MAX || ! u_pthread_keys[key]->in_use ) {
			pthread_mutex_unlock( &u_pthread_keys_lock );
			return EINVAL;
		} // if
		u_pthread_keys[key]->in_use = false;
		u_pthread_keys[key]->destructor = nullptr;

		// Remove key from all threads with a value.

		Pthread_values * p;
		uSequence<Pthread_values> &head = u_pthread_keys[key]->threads;
		for ( uSeqIter<Pthread_values> iter( head ); iter >> p; ) {
			p->in_use = false;
			head.remove( p );
		} // for

		pthread_mutex_unlock( &u_pthread_keys_lock );
		uDEBUGPRT( uDebugPrt( "pthread_key_delete(key:0x%x) exit task:%p\n", key, &uThisTask() ); );
		return 0;
	} // pthread_key_delete

	int pthread_setspecific( pthread_key_t key, const void * value ) __THROW {
		uDEBUGPRT( uDebugPrt( "pthread_setspecific(key:0x%x, value:%p) enter task:%p\n", key, value, &uThisTask() ); );
		uBaseTask &t = uThisTask();

		Pthread_values * values;
		if ( t.pthreadData == nullptr ) {
			values = new Pthread_values[PTHREAD_KEYS_MAX];
			t.pthreadData = values;
			for ( int i = 0; i < PTHREAD_KEYS_MAX; i += 1 ) {
				values[i].in_use = false;
			} // for
		} else {
			values = (Pthread_values *)t.pthreadData;
		} // if

		pthread_mutex_lock( &u_pthread_keys_lock );
		if ( key >= PTHREAD_KEYS_MAX || ! u_pthread_keys[key]->in_use ) {
			pthread_mutex_unlock( &u_pthread_keys_lock );
			return EINVAL;
		} // if

		Pthread_values &entry = values[key];
		if ( ! entry.in_use ) {
			entry.in_use = true;
			u_pthread_keys[key]->threads.add( &entry );
		} // if
		entry.value = (void *)value;
		pthread_mutex_unlock( &u_pthread_keys_lock );
		uDEBUGPRT( uDebugPrt( "pthread_setspecific(key:0x%x, value:%p) exit task:%p\n", key, value, &uThisTask() ); );
		return 0;
	} // pthread_setspecific

	void * pthread_getspecific( pthread_key_t key ) __THROW {
		uDEBUGPRT( uDebugPrt( "pthread_getspecific(key:0x%x) enter task:%p\n", key, &uThisTask() ); );
	  if ( key >= PTHREAD_KEYS_MAX ) return nullptr;

		uBaseTask &t = uThisTask();
	  if ( t.pthreadData == nullptr ) return nullptr;

		pthread_mutex_lock( &u_pthread_keys_lock );
		Pthread_values &entry = ((Pthread_values *)t.pthreadData)[key];
	  if ( ! entry.in_use ) {
			pthread_mutex_unlock( &u_pthread_keys_lock );
			return nullptr;
		} // if
		void * value = entry.value;
		pthread_mutex_unlock( &u_pthread_keys_lock );
		uDEBUGPRT( uDebugPrt( "%p = pthread_getspecific(key:0x%x) exit task:%p\n", value, key, &uThisTask() ); );
		return value;
	} // pthread_getspecific


	//######################### Signal #########################


	int pthread_sigmask( int /* how */, const sigset_t * /* set */, sigset_t * /* oset */ ) __THROW {
		return 0;
	} // pthread_sigmask

	int pthread_kill( pthread_t /* thread */, int sig ) __THROW {
		uDEBUGPRT( uDebugPrt( "pthread_kill( 0x%lx, %d )\n", thread, sig ); );
		// assert( thread != NOT_A_PTHREAD );
	  if ( sig == 0 ) {
			return 0;
		} else {
			abort( "pthread_kill not implemented" );
		} // if
		return 0;
	} // pthread_kill


	//######################### ID #########################


	pthread_t pthread_self( void ) __THROW {
		uDEBUGPRT( uDebugPrt( "pthread_self enter task:%p\n", &uThisTask() ); );
//		uPthreadable * p = dynamic_cast<uPthreadable *>(&uThisTask());
		uPthreadable * p = (uPthreadable *)&uThisTask();
		if ( p == nullptr ) {
			return NOT_A_PTHREAD;
		} // if
		return p->pthreadId();
	} // pthread_self


	int pthread_once( pthread_once_t * once_control, void (* init_routine)( void ) ) __OLD_THROW {
		uDEBUGPRT(
			//uDebugPrt( "pthread_once(once_control:%p, init_routine:%p) enter task:%p\n", once_control, init_routine, &uThisTask() );
		);

		// storage for pthread_once_t must be >= int and initialized to zero by PTHREAD_ONCE_INIT
		static_assert( sizeof(pthread_once_t) >= sizeof(int), "sizeof(pthread_once_t) < sizeof(int)" );
		pthread_mutex_lock( &u_pthread_once_lock );
		if ( *((int *)once_control) == 0 ) {
			init_routine();
			*((int *)once_control) = 1;
		} // if
		pthread_mutex_unlock( &u_pthread_once_lock );
		uDEBUGPRT( uDebugPrt( "pthread_once(once_control:%p, init_routine:%p) exit task:%p\n", once_control, init_routine, &uThisTask() ); );
		return 0;
	} // pthread_once


	//######################### Scheduling #########################


	int pthread_setschedparam( pthread_t /* thread */, int /* policy */, const struct sched_param * /* param */ ) __THROW {
		abort( "pthread_setschedparam not implemented" );
		return 0;
	} // pthread_setschedparam

	int pthread_getschedparam( pthread_t /* thread */, int */* policy */, struct sched_param * /* param */ ) __THROW {
		abort( "pthread_getschedparam not implemented" );
		return 0;
	} // pthread_getschedparam


	//######################### Cancellation #########################


	int pthread_cancel( pthread_t threadID ) __OLD_THROW {
		assert( threadID != NOT_A_PTHREAD );
		uPthreadable * p = PthreadPid::lookup( threadID );
		p->cancel();
		return 0;
	} // pthread_cancel

	int pthread_setcancelstate( int state, int * oldstate ) __OLD_THROW {
	  if ( state != PTHREAD_CANCEL_ENABLE && state != PTHREAD_CANCEL_DISABLE ) return EINVAL;
	    uBaseCoroutine * c = &uThisCoroutine();
#ifdef __U_DEBUG__
		uPthreadable * p = dynamic_cast<uPthreadable *>(c);
		if ( p == nullptr ) abort( "pthread_setcancelstate performed on non-pthread task or while executing a uC++ coroutine" );
#endif // __U_DEBUG__
		if ( oldstate != nullptr ) *oldstate = c->getCancelState();
		c->setCancelState( (uBaseCoroutine::CancellationState)state );
		return 0;
	} // pthread_setcancelstate

	int pthread_setcanceltype( int type, int * oldtype ) __OLD_THROW {
		// currently do not support asynchronous cancellation
	  if ( type != PTHREAD_CANCEL_DEFERRED && type != PTHREAD_CANCEL_ASYNCHRONOUS ) return EINVAL;
		uBaseCoroutine * c = &uThisCoroutine();
#ifdef __U_DEBUG__
		uPthreadable * p = dynamic_cast<uPthreadable *>(c);
		if ( p == nullptr ) abort( "pthread_setcancelstate performed on non-pthread task or while executing a uC++ coroutine" );
#endif // __U_DEBUG__
		if ( oldtype != nullptr ) * oldtype = c->getCancelType();
		c->setCancelType( (uBaseCoroutine::CancellationType)type );
		return 0;
	} // pthread_setcanceltype

	void pthread_testcancel( void ) __OLD_THROW {
		uBaseCoroutine * c = &uThisCoroutine();
		uPthreadable * p = dynamic_cast<uPthreadable *>(c);
	  if ( p == nullptr ) return;
	  if ( c->getCancelState() == (uBaseCoroutine::CancellationState)PTHREAD_CANCEL_DISABLE || ! c->cancelled() ) return;
#if defined( __U_BROKEN_CANCEL__ )
		abort( "pthread cancellation is not supported on this system" );
#else
		pthread_exit( PTHREAD_CANCELED );
#endif // __U_BROKEN_CANCEL__
		// CONTROL NEVER REACHES HERE
		assert( false );
	} // pthread_testcancel


	void _pthread_cleanup_push( struct _pthread_cleanup_buffer * buffer, void (* routine) (void *), void * args ) __THROW {
		uPthreadable * p = dynamic_cast< uPthreadable * > ( &uThisCoroutine() );
#ifdef __U_DEBUG__
		if ( p == nullptr ) abort( "pthread_cleanup_push performed on something other than a pthread task" );	    
#endif // __U_DEBUG__
		p->cleanup_push( routine, args, buffer );
	} // pthread_cleanup_push

	void _pthread_cleanup_pop ( struct _pthread_cleanup_buffer * /* buffer */, int ex ) __THROW {
		uPthreadable * p = dynamic_cast< uPthreadable * > ( &uThisCoroutine() );
#ifdef __U_DEBUG__
		if ( p == nullptr ) abort( "pthread_cleanup_pop performed on something other than a pthread task" );	    
#endif // __U_DEBUG__
		p->cleanup_pop( ex );
	} // pthread_cleanup_pop


	//######################### Mutex #########################

	static inline void mutex_check( pthread_mutex_t * mutex ) {
		// Cannot use a pthread_mutex_lock due to recursion on initialization.
		static char magic_check[sizeof(uOwnerLock)] __attribute__(( aligned (16) )); // set to zero
		// Use double check to improve performance. Check is safe on x86; volatile prevents compiler reordering
		volatile pthread_mutex_t * const mutex_ = mutex;
		// SKULLDUGGERY: not a portable way to access the kind field, /usr/include/x86_64-linux-gnu/bits/pthreadtypes.h
		int kind = ((pthread_mutex_t *)mutex_)->__data.__kind;
		// kind is a small pthread enumerated type. If it greater than 32, it is a value in an uOwnerlock field.
		if ( 0 < kind && kind < 32 ) {					// static initialized ?
			((uOwnerLock *)&magic_check)->acquire();	// race
			kind = ((pthread_mutex_t *)mutex_)->__data.__kind;
			if ( 0 < kind && kind < 32 ) {				// static initialized ?
				pthread_mutex_init( mutex, nullptr );
			} // if
			((uOwnerLock *)&magic_check)->release();
		} // if
	} // mutex_check

	int pthread_mutex_init( pthread_mutex_t * mutex, const pthread_mutexattr_t * /* attr */ ) __THROW {
		uDEBUGPRT( uDebugPrt( "pthread_mutex_init(mutex:%p, attr:%p) enter task:%p\n", mutex, attr, &uThisTask() ); );
		PthreadLock::init< uOwnerLock >( mutex );
		return 0;
	} // pthread_mutex_init

	int pthread_mutex_destroy( pthread_mutex_t * mutex ) __THROW {
		PthreadLock::destroy< uOwnerLock >( mutex );
		return 0;
	} // pthread_mutex_destroy

	int pthread_mutex_lock( pthread_mutex_t * mutex ) __THROW {
		if ( ! uKernelModule::kernelModuleInitialized ) {
			uKernelModule::startup();
		} // if

		uDEBUGPRT( uDebugPrt( "pthread_mutex_lock(mutex:%p) enter task:%p\n", mutex, &uThisTask() ); );
		mutex_check( mutex );
		PthreadLock::get< uOwnerLock >( mutex )->acquire();
		return 0;
	} // pthread_mutex_lock

	int pthread_mutex_trylock( pthread_mutex_t * mutex ) __THROW {
		uDEBUGPRT(uDebugPrt( "pthread_mutex_trylock(mutex:%p) enter task:%p\n", mutex, &uThisTask() ); );
		mutex_check( mutex );
		return PthreadLock::get< uOwnerLock >( mutex )->tryacquire() ? 0 : EBUSY;
	} // pthread_mutex_trylock

	int pthread_mutex_unlock( pthread_mutex_t * mutex ) __THROW {
		uDEBUGPRT( uDebugPrt( "pthread_mutex_unlock(mutex:%p) enter task:%p\n", mutex, &uThisTask() ); );
		PthreadLock::get< uOwnerLock >( mutex )->release();
		return 0;
	} // pthread_mutex_unlock


	int pthread_mutexattr_init( pthread_mutexattr_t * /* attr */ ) __THROW {
		return 0;
	} // pthread_mutexattr_init

	int pthread_mutexattr_destroy( pthread_mutexattr_t * /* attr */ ) __THROW {
		return 0;
	} // pthread_mutexattr_destroy

	int pthread_mutexattr_setpshared( pthread_mutexattr_t * /* attr */, int /* pshared */ ) __THROW {
		return 0;
	} // pthread_mutexattr_setpshared

	int pthread_mutexattr_getpshared( const pthread_mutexattr_t * /* attr */, int * /* pshared */ ) __THROW {
		return 0;
	} // pthread_mutexattr_getpshared

	int pthread_mutexattr_setprotocol( pthread_mutexattr_t * /* attr */, int /* protocol */ ) __THROW {
		return 0;
	} // pthread_mutexattr_setprotocol

	int pthread_mutexattr_getprotocol( const pthread_mutexattr_t * /* attr */, int * /* protocol */ ) __THROW {
		return 0;
	} // pthread_mutexattr_getprotocol

	int pthread_mutexattr_setprioceiling( pthread_mutexattr_t * /* attr */, int /* prioceiling */ ) __THROW {
		return 0;
	} // pthread_mutexattr_setprioceiling

	int pthread_mutexattr_getprioceiling( const pthread_mutexattr_t * /* attr */, int * /* ceiling */ ) __THROW {
		return 0;
	} // pthread_mutexattr_getprioceiling

	int pthread_mutex_setprioceiling( pthread_mutex_t * /* mutex */, int /* prioceiling */, int * /* old_ceiling */ ) __THROW {
		return 0;
	} // pthread_mutex_setprioceiling

	int pthread_mutex_getprioceiling( const pthread_mutex_t * /* mutex */, int * /* ceiling */ ) __THROW {
		return 0;
	} // pthread_mutex_getprioceiling

	int pthread_mutexattr_gettype( __const pthread_mutexattr_t * __restrict /* __attr */, int * __restrict /* __kind */ ) __THROW {
		return 0;
	} // pthread_mutexattr_gettype

	int pthread_mutexattr_settype( pthread_mutexattr_t * /* __attr */, int /* __kind */ ) __THROW {
		return 0;
	} // pthread_mutexattr_settype


	//######################### Condition #########################


	int pthread_cond_init( pthread_cond_t * cond, const pthread_condattr_t * /* attr */ ) __THROW {
		PthreadLock::init< uCondLock >( cond );
		return 0;
	} // pthread_cond_init

	int pthread_cond_destroy( pthread_cond_t * cond ) __THROW {
		uDEBUGPRT( uDebugPrt( "pthread_cond_destroy(cond:%p) task:%p\n", cond, &uThisTask() ); );
		PthreadLock::destroy< uCondLock >( cond );
		return 0;
	} // pthread_cond_destroy

	int pthread_cond_wait( pthread_cond_t * cond, pthread_mutex_t * mutex ) {
		uDEBUGPRT( uDebugPrt( "pthread_cond_wait(cond:%p) mutex:%p enter task:%p\n", cond, mutex, &uThisTask() ); );
		pthread_testcancel();							// pthread_cond_wait is a cancellation point
		PthreadLock::get< uCondLock >( cond )->wait( *PthreadLock::get< uOwnerLock >( mutex ) );
		return 0;
	} // pthread_cond_wait

	int pthread_cond_timedwait( pthread_cond_t * cond, pthread_mutex_t * mutex, const struct timespec * abstime ) {
		uDEBUGPRT( uDebugPrt( "pthread_cond_timedwait(cond:%p) mutex:%p enter task:%p\n", cond, mutex, &uThisTask() ); );
		pthread_testcancel();							// pthread_cond_timedwait is a cancellation point
		return PthreadLock::get< uCondLock >( cond )->wait( *PthreadLock::get< uOwnerLock >( mutex ), uTime( *abstime ) ) ? 0 : ETIMEDOUT;
	} // pthread_cond_timedwait

	int pthread_cond_signal( pthread_cond_t * cond ) __THROW {
		uDEBUGPRT(	uDebugPrt( "pthread_cond_signal(cond:%p) enter task:%p\n", cond, &uThisTask() ); );
		PthreadLock::get< uCondLock >( cond )->signal();
		return 0;
	} // pthread_cond_signal

	int pthread_cond_broadcast( pthread_cond_t * cond ) __THROW {
		uDEBUGPRT( uDebugPrt( "pthread_cond_broadcast(cond:%p) task:%p\n", cond, &uThisTask() ); );
		PthreadLock::get< uCondLock >( cond )->broadcast();
		return 0;
	} // pthread_cond_broadcast


	int pthread_condattr_init( pthread_condattr_t * /* attr */ ) __THROW {
		// storage for pthread_condattr_t must be >= int
		return 0;
	} // pthread_condattr_init

	int pthread_condattr_destroy( pthread_condattr_t * /* attr */ ) __THROW {
		return 0;
	} // pthread_condattr_destroy

	int pthread_condattr_setpshared( pthread_condattr_t * attr, int pshared ) __THROW {
		*((int *)attr) = pshared;
		return 0;
	} // pthread_condattr_setpshared

	int pthread_condattr_getpshared( const pthread_condattr_t * attr, int * pshared ) __THROW {
		*pshared = *((int *)attr);
		return 0;
	} // pthread_condattr_getpshared
} // extern "C"


extern "C" {

// XOPEN

	//######################### Mutex #########################

	int pthread_mutex_timedlock( pthread_mutex_t *__restrict /* __mutex */, __const struct timespec *__restrict /* __abstime */ ) __THROW {
		abort( "pthread_mutex_timedlock not implemented" );
	} // pthread_mutex_timedlock

	//######################### Condition #########################

	int pthread_condattr_getclock( __const pthread_condattr_t * __restrict /* __attr */, __clockid_t *__restrict /* __clock_id */ ) __THROW {
		abort( "pthread_condattr_getclock not implemented" );
	} // pthread_condattr_getclock

	int pthread_condattr_setclock( pthread_condattr_t * /* __attr */, __clockid_t /* __clock_id */ ) __THROW {
		abort( "pthread_condattr_setclock not implemented" );
	} // pthread_condattr_setclock

	//######################### Spinlock #########################

	int pthread_spin_init( pthread_spinlock_t * /* __lock */, int /*__pshared */ ) __THROW {
		abort( "pthread_spin_init not implemented" );
	} // pthread_spin_init

	int pthread_spin_destroy( pthread_spinlock_t * /* __lock */ ) __THROW {
		abort( "pthread_spin_destroy not implemented" );
	} // pthread_spin_destroy

	int pthread_spin_lock( pthread_spinlock_t * /* __lock */ ) __THROW {
		abort( "pthread_spin_lock not implemented" );
	} // pthread_spin_lock

	int pthread_spin_trylock( pthread_spinlock_t * /* __lock */ ) __THROW {
		abort( "pthread_spin_trylock not implemented" );
	} // pthread_spin_trylock

	int pthread_spin_unlock( pthread_spinlock_t * /* __lock */ ) __THROW {
		abort( "pthread_spin_unlock not implemented" );
	} // pthread_spin_unlock

	//######################### Barrier #########################

	int pthread_barrier_init( pthread_barrier_t *__restrict /* __barrier */, __const pthread_barrierattr_t *__restrict /* __attr */, unsigned int /* __count */ ) __THROW {
		abort( "pthread_barrier_init not implemented" );
	} // pthread_barrier_init

	int pthread_barrier_destroy( pthread_barrier_t * /* __barrier */ ) __THROW {
		abort( "pthread_barrier_destroy not implemented" );
	} // pthread_barrier_destroy

	int pthread_barrier_wait( pthread_barrier_t * /* __barrier */ ) __THROW {
		abort( "pthread_barrier_wait not implemented" );
	} // pthread_barrier_wait

	int pthread_barrierattr_init( pthread_barrierattr_t * /* __attr */ ) __THROW {
		abort( "pthread_barrierattr_init not implemented" );
	} // pthread_barrierattr_init

	int pthread_barrierattr_destroy( pthread_barrierattr_t * /* __attr */ ) __THROW {
		abort( "pthread_barrierattr_destroy not implemented" );
	} // pthread_barrierattr_destroy

	int pthread_barrierattr_getpshared( __const pthread_barrierattr_t * __restrict /* __attr */, int *__restrict /* __pshared */ ) __THROW {
		abort( "pthread_barrierattr_getpshared not implemented" );
	} // pthread_barrierattr_getpshared

	int pthread_barrierattr_setpshared( pthread_barrierattr_t * /* __attr */, int /* __pshared */ ) __THROW {
		abort( "pthread_barrierattr_setpshared not implemented" );
	} // pthread_barrierattr_setpshared

	//######################### Clock #########################

	int pthread_getcpuclockid( pthread_t /* __thread_id */, __clockid_t * /* __clock_id */ ) __THROW {
		abort( "pthread_getcpuclockid not implemented" );
	} // pthread_getcpuclockid

	// pthread_atfork()

// UNIX98

	//######################### Read/Write #########################

	int pthread_rwlock_init( pthread_rwlock_t *__restrict /* __rwlock */, __const pthread_rwlockattr_t *__restrict /* __attr */ ) __THROW {
		abort( "pthread_rwlock_init not implemented" );
	} // pthread_rwlock_init

	int pthread_rwlock_destroy( pthread_rwlock_t * /* __rwlock */ ) __THROW {
		abort( "pthread_rwlock_destroy not implemented" );
	} // pthread_rwlock_destroy

	int pthread_rwlock_rdlock( pthread_rwlock_t * /* __rwlock */ ) __THROW {
		abort( "pthread_rwlock_rdlock not implemented" );
	} // pthread_rwlock_rdlock

	int pthread_rwlock_tryrdlock( pthread_rwlock_t * /* __rwlock */ ) __THROW {
		abort( "pthread_rwlock_tryrdlock not implemented" );
	} // pthread_rwlock_tryrdlock

	int pthread_rwlock_wrlock( pthread_rwlock_t * /* __rwlock */ ) __THROW {
		abort( "pthread_rwlock_wrlock not implemented" );
	} // pthread_rwlock_wrlock

	int pthread_rwlock_trywrlock( pthread_rwlock_t * /* __rwlock */ ) __THROW {
		abort( "pthread_rwlock_trywrlock not implemented" );
	} // pthread_rwlock_trywrlock

	int pthread_rwlock_unlock( pthread_rwlock_t * /* __rwlock */ ) __THROW {
		abort( "pthread_rwlock_unlock not implemented" );
	} // pthread_rwlock_unlock

	int pthread_rwlockattr_init( pthread_rwlockattr_t * /* __attr */ ) __THROW {
		abort( "pthread_rwlockattr_init not implemented" );
	} // pthread_rwlockattr_init

	int pthread_rwlockattr_destroy( pthread_rwlockattr_t * /*__attr */ ) __THROW {
		abort( "pthread_rwlockattr_destroy not implemented" );
	} // pthread_rwlockattr_destroy

	int pthread_rwlockattr_getpshared( __const pthread_rwlockattr_t * __restrict /* __attr */, int *__restrict /* __pshared */ ) __THROW {
		abort( "pthread_rwlockattr_getpshared not implemented" );
	} // pthread_rwlockattr_getpshared

	int pthread_rwlockattr_setpshared( pthread_rwlockattr_t * /* __attr */, int /* __pshared */ ) __THROW {
		abort( "pthread_rwlockattr_setpshared not implemented" );
	} // pthread_rwlockattr_setpshared

	int pthread_rwlockattr_getkind_np( __const pthread_rwlockattr_t * /* __attr */, int * /* __pref */ ) __THROW {
		abort( "pthread_rwlockattr_getkind_np not implemented" );
	} // pthread_rwlockattr_getkind_np

	int pthread_rwlockattr_setkind_np( pthread_rwlockattr_t * /* __attr */, int /* __pref */ ) __THROW {
		abort( "pthread_rwlockattr_setkind_np not implemented" );
	} // pthread_rwlockattr_setkind_np

// UNIX98 + XOPEN

	int pthread_rwlock_timedrdlock( pthread_rwlock_t *__restrict  /* __rwlock */, __const struct timespec *__restrict /* __abstime */ ) __THROW {
		abort( "pthread_rwlock_timedrdlock not implemented" );
	} // pthread_rwlock_timedrdlock

	int pthread_rwlock_timedwrlock( pthread_rwlock_t *__restrict  /* __rwlock */, __const struct timespec *__restrict /* __abstime */ ) __THROW {
		abort( "pthread_rwlock_timedwrlock not implemented" );
	} // pthread_rwlock_timedwrlock

// GNU

	//######################### Parallelism #########################

	// Use the pthread versions.

	int pthread_setaffinity_np( pthread_t /* thread */, size_t /* cpusetsize */, const cpu_set_t * /* cpuset */ ) __THROW {
	 	abort( "pthread_setaffinity_np not directly accessible" );
	} // pthread_setaffinity_np

	int pthread_getaffinity_np( pthread_t /* thread */, size_t /* cpusetsize */, cpu_set_t * /* cpuset */ ) __THROW {
	 	abort( "pthread_getaffinity_np not directly accessible" );
	} // pthread_getaffinity_np

	int pthread_attr_setaffinity_np( pthread_attr_t * /* attr */, size_t /* cpusetsize */, const cpu_set_t * /* cpuset */ ) __THROW {
	 	abort( "pthread_attr_setaffinity_np not directly accessible" );
	} // pthread_attr_setaffinity_np

	int pthread_attr_getaffinity_np( __const pthread_attr_t * /* __attr */, size_t /* __cpusetsize */, cpu_set_t * /* __cpuset */ ) __THROW {
	 	abort( "pthread_attr_getaffinity_np not directly accessible" );
	} // pthread_attr_getaffinity_np

	//######################### Cancellation #########################

	// pthread_cleanup_push_defer_np()  GNU extension
	// pthread_cleanup_pop_restore_np() GNU extension

	void _pthread_cleanup_push_defer( struct _pthread_cleanup_buffer * /* __buffer */, void( * /* __routine */ )( void * ), void * /* __arg */ ) __THROW {
		abort( "_pthread_cleanup_push_defer not implemented" );
	} // _pthread_cleanup_push_defer

	void _pthread_cleanup_pop_restore( struct _pthread_cleanup_buffer * /* __buffer */, int /* __execute */ ) __THROW {
		abort( "_pthread_cleanup_pop_restore not implemented" );
	} // _pthread_cleanup_pop_restore
} // extern "C"


// Local Variables: //
// compile-command: "make install" //
// End: //
