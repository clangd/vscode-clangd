//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2001
// 
// Locks.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Dec 10 15:08:22 2001
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Feb 22 11:59:43 2022
// Update Count     : 56
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

#include <uAdaptiveLock.h>
#include <uSemaphore.h>
#include <iostream>
using std::cout;
using std::endl;

const unsigned int NoOfTimes = 50000;

uMutexLock sharedLock0;
uOwnerLock sharedLock1;
uAdaptiveLock<> sharedLock2;
volatile uBaseTask *checkID;
uSemaphore start(0);

_Task testerOL_AR {
	void main() {
		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			sharedLock1.acquire();
			checkID = &uThisTask();
			yield();
			if ( checkID != &uThisTask() ) abort( "interference" );
			sharedLock1.acquire();
			if ( checkID != &uThisTask() ) abort( "interference" );
			yield();
			if ( checkID != &uThisTask() ) abort( "interference" );
			sharedLock1.release();
			if ( checkID != &uThisTask() ) abort( "interference" );
			yield();
			if ( checkID != &uThisTask() ) abort( "interference" );
			sharedLock1.release();
			yield(2);
		} // for
	}
};

_Task testerOL_TAR {
	void main() {
		for ( unsigned int i = 0; i < 100; i += 1 ) {
			while ( ! sharedLock1.tryacquire() ) yield();
			checkID = &uThisTask();
			yield();
			if ( checkID != &uThisTask() ) abort( "interference" );
			if ( ! sharedLock1.tryacquire() ) abort( "interference" );
			if ( checkID != &uThisTask() ) abort( "interference" );
			yield();
			if ( checkID != &uThisTask() ) abort( "interference" );
			sharedLock1.release();
			if ( checkID != &uThisTask() ) abort( "interference" );
			yield();
			if ( checkID != &uThisTask() ) abort( "interference" );
			sharedLock1.release();
			yield(2);
		} // for
	}
};

_Task testerAL_AR {
	void main() {
		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			sharedLock2.acquire();
			checkID = &uThisTask();
			yield();
			if ( checkID != &uThisTask() ) abort( "interference" );
			sharedLock2.acquire();
			if ( checkID != &uThisTask() ) abort( "interference" );
			yield();
			if ( checkID != &uThisTask() ) abort( "interference" );
			sharedLock2.release();
			if ( checkID != &uThisTask() ) abort( "interference" );
			yield();
			if ( checkID != &uThisTask() ) abort( "interference" );
			sharedLock2.release();
			yield(2);
		} // for
	}
};

_Task testerAL_TAR {
	void main() {
		for ( unsigned int i = 0; i < 100; i += 1 ) {
			while ( ! sharedLock2.tryacquire() ) yield();
			checkID = &uThisTask();
			yield();
			if ( checkID != &uThisTask() ) abort( "interference" );
			if ( ! sharedLock2.tryacquire() ) abort( "interference" );
			if ( checkID != &uThisTask() ) abort( "interference" );
			yield();
			if ( checkID != &uThisTask() ) abort( "interference" );
			sharedLock2.release();
			if ( checkID != &uThisTask() ) abort( "interference" );
			yield();
			if ( checkID != &uThisTask() ) abort( "interference" );
			sharedLock2.release();
			yield(2);
		} // for
	}
};

_Task testerML_AR {
	void main() {
		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			sharedLock0.acquire();
			checkID = &uThisTask();
			if ( checkID != &uThisTask() ) abort( "interference" );
			yield();
			if ( checkID != &uThisTask() ) abort( "interference" );
			sharedLock0.release();
			yield(2);
		} // for
	}
};

_Task testerML_TAR {
	void main() {
		for ( unsigned int i = 0; i < 100; i += 1 ) {
			while ( ! sharedLock0.tryacquire() ) yield();
			checkID = &uThisTask();
			yield();
			if ( checkID != &uThisTask() ) abort( "interference" );
			yield();
			if ( checkID != &uThisTask() ) abort( "interference" );
			sharedLock0.release();
			yield(2);
		} // for
	}
};

class monitor {
	uOwnerLock lock;
	uMutexLock m_lock;
	uCondLock cond1, cond2;
	unsigned int cnt;
  public:
	monitor() { cnt = 0; }
	void mS2W1() {
		lock.acquire();
		lock.acquire();
		lock.acquire();
		if ( ! cond2.empty() ) {
			cond2.signal();
		} else {
			cond1.wait( lock );
		}
		lock.release();
		lock.release();
		lock.release();
	}
	void mLS1W2() {
		lock.acquire();
		if ( ! cond1.empty() ) {
			cond1.signal();
		} else {
			cond2.wait( lock );
		}
		lock.release();
	}
	void mLB() {
		lock.acquire();
		lock.acquire();
		lock.acquire();
		cond1.broadcast();
		lock.release();
		lock.release();
		lock.release();
	}
	void mLWSem() {
		lock.acquire();
		lock.acquire();
		cnt += 1;
		if ( cnt == 3 ) start.V();
		cond1.wait( lock );
		cnt -= 1;
		lock.release();
		lock.release();
	}
	void mLW() {
		lock.acquire();
		lock.acquire();
		cond1.wait( lock );
		lock.release();
		lock.release();
	}
	void mS() {						// same for both uMutexLock and uOwnerLock
		cond1.signal();
	}
	void mB() {						// same for both uMutexLock and uOwnerLock
		cond1.broadcast();
	}

	// uMutexLock testing methods
	void m_mS2W1() {
		m_lock.acquire();
		if ( ! cond2.empty() ) {
			cond2.signal();
			m_lock.release();
		} else {
			cond1.wait( m_lock );
		}
	}
	void m_mLS1W2() {
		m_lock.acquire();
		if ( ! cond1.empty() ) {
			cond1.signal();
			m_lock.release();
		} else {
			cond2.wait( m_lock );
		}
	}
	void m_mLB() {
		m_lock.acquire();
		cond1.broadcast();
		m_lock.release();
	}
	void m_mLWSem() {
		m_lock.acquire();
		cnt += 1;
		if ( cnt == 3 ) {
			start.V();
			cnt = 0;
		}
		cond1.wait( m_lock );
	}
	void m_mLW() {
		m_lock.acquire();
		cond1.wait( m_lock );
	}
};

_Task testerCL_S2W1_M {
	monitor &m;
	void main() {
		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			m.m_mS2W1();
			yield(2);
		} // for
	}
  public:
	testerCL_S2W1_M( monitor &m ) : m( m ) {}
};

_Task testerCL_S1W2_M {
	monitor &m;
	void main() {
		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			m.m_mLS1W2();
			yield(2);
		} // for
	}
  public:
	testerCL_S1W2_M( monitor &m ) : m( m ) {}
};

_Task testerCL_S2W1_O {
	monitor &m;
	void main() {
		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			m.mS2W1();
			yield(2);
		} // for
	}
  public:
	testerCL_S2W1_O( monitor &m ) : m( m ) {}
};

_Task testerCL_S1W2_O {
	monitor &m;
	void main() {
		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			m.mLS1W2();
			yield(2);
		} // for
	}
  public:
	testerCL_S1W2_O( monitor &m ) : m( m ) {}
};

_Task testerCL_LB_M {  
	monitor &m;
	void main() {
		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			start.P();
			m.m_mLB();
			yield(2);
		} // for
	}
  public:
	testerCL_LB_M( monitor &m ) : m( m ) {}
};

_Task testerCL_LWSem_M {
	monitor &m;
	void main() {
		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			m.m_mLWSem();
			yield(2);
		} // for
	}
  public:
	testerCL_LWSem_M( monitor &m ) : m( m ) {}
};

_Task testerCL_LW_M {
	monitor &m;
	void main() {
		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			m.m_mLW();
			yield(2);
		} // for
	}
  public:
	testerCL_LW_M( monitor &m ) : m( m ) {}
};

_Task testerCL_LB_O {
	monitor &m;
	void main() {
		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			start.P();
			m.mLB();
			yield(2);
		} // for
	}
  public:
	testerCL_LB_O( monitor &m ) : m( m ) {}
};

_Task testerCL_LWSem_O {
	monitor &m;
	void main() {
		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			m.mLWSem();
			yield(2);
		} // for
	}
  public:
	testerCL_LWSem_O( monitor &m ) : m( m ) {}
};

_Task testerCL_LW_O {
	monitor &m;
	void main() {
		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			m.mLW();
			yield(2);
		} // for
	}
  public:
	testerCL_LW_O( monitor &m ) : m( m ) {}
};

_Task testerCL_S {					// same for both uMutexLock and uOwnerLock
	monitor &m;
	void main() {
		for ( ;; ) {
			_Accept( ~testerCL_S ) {			// poll for destructor call
				break;
			} _Else;
			m.mS();
			yield( 2 );
		} // for
	}
  public:
	testerCL_S( monitor &m ) : m( m ) {}
};

_Task testerCL_B {					// same for both uMutexLock and uOwnerLock
	monitor &m;
	void main() {
		for ( ;; ) {
			_Accept( ~testerCL_B ) {			// poll for destructor call
				break;
			} _Else;
			m.mB();
			yield( rand() % 5 );
		} // for
	}
  public:
	testerCL_B( monitor &m ) : m( m ) {}
};

int main() {
	uProcessor processor[1] __attribute__(( unused ));	// more than one processor
#if 1
	{							// test uMutexLock
		testerML_AR t1[2] __attribute__(( unused ));
		testerML_TAR t2[2] __attribute__(( unused ));

		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			sharedLock0.acquire();
			uBaseTask::yield();
			sharedLock0.release();
			uBaseTask::yield( 2 );
		} // for
	}
	cout << "completion uMutexLock test" << endl;
#endif
#if 1
	{							// test uOwnerLock
		testerOL_AR t1[2] __attribute__(( unused ));
		testerOL_TAR t2[2] __attribute__(( unused ));

		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			sharedLock1.acquire();
			uBaseTask::yield();
			sharedLock1.release();
			uBaseTask::yield( 2 );
		} // for
	}
	cout << "completion uOwnerLock test" << endl;
#endif
#if 1
	{							// test uAdaptiveLock
		testerAL_AR t1[2] __attribute__(( unused ));
		testerAL_TAR t2[2] __attribute__(( unused ));

		for ( unsigned int i = 0; i < NoOfTimes; i += 1 ) {
			sharedLock2.acquire();
			uBaseTask::yield();
			sharedLock2.release();
			uBaseTask::yield( 2 );
		} // for
	}
	cout << "completion uAdaptiveLock test" << endl;
#endif
	{							// test uCondLock
		monitor m;
#if 1
		{						// signal/wait
			testerCL_S2W1_M t1( m );
			testerCL_S1W2_M t2( m );
		}
		cout << "completion uCondLock with uMutexLock test: signal/wait, locked signal" << endl;
#endif
#if 1
		{						// signal/wait
			testerCL_S2W1_O t1( m );
			testerCL_S1W2_O t2( m );
		}
		cout << "completion uCondLock with uOwnerLock test: signal/wait, locked signal" << endl;
#endif
#if 1
		{						// signal/wait
			const unsigned int N = 5;
			testerCL_S t1( m );
			testerCL_LW_M *ti[N];
			for ( unsigned int i = 0; i < N; i += 1 ) {
				ti[i] = new testerCL_LW_M( m );
			} // for
			for ( unsigned int i = 0; i < N; i += 1 ) {
				delete ti[i];
			} // for
		}
		cout << "completion uCondLock with uMutexLock test: signal/wait, unlocked signal" << endl;
#endif
#if 1
		{						// signal/wait
			const unsigned int N = 5;
			testerCL_S t1( m );
			testerCL_LW_O *ti[N];
			for ( unsigned int i = 0; i < N; i += 1 ) {
				ti[i] = new testerCL_LW_O( m );
			} // for
			for ( unsigned int i = 0; i < N; i += 1 ) {
				delete ti[i];
			} // for
		}
		cout << "completion uCondLock with uOwnerLock test: signal/wait, unlocked signal" << endl;
#endif
#if 1
		{						// broadcast/wait
			const unsigned int N = 3;
			testerCL_LB_M t1( m );
			testerCL_LWSem_M *ti[N];
			for ( unsigned int i = 0; i < N; i += 1 ) {
				ti[i] = new testerCL_LWSem_M( m );
			} // for
			for ( unsigned int i = 0; i < N; i += 1 ) {
				delete ti[i];
			} // for
		}
		cout << "completion uCondLock with uMutexLock test: broadcast/wait, locked broadcast" << endl;
#endif
#if 1
		{						// broadcast/wait
			const unsigned int N = 3;
			testerCL_LB_O t1( m );
			testerCL_LWSem_O *ti[N];
			for ( unsigned int i = 0; i < N; i += 1 ) {
				ti[i] = new testerCL_LWSem_O( m );
			} // for
			for ( unsigned int i = 0; i < N; i += 1 ) {
				delete ti[i];
			} // for
		}
		cout << "completion uCondLock with uOwnerLock test: broadcast/wait, locked broadcast" << endl;
#endif
#if 1
		{						// broadcast/wait
			const unsigned int N = 5;
			testerCL_B t1( m );
			testerCL_LW_M *ti[N];
			for ( unsigned int i = 0; i < N; i += 1 ) {
				ti[i] = new testerCL_LW_M( m );
			} // for
			for ( unsigned int i = 0; i < N; i += 1 ) {
				delete ti[i];
			} // for
		}
		cout << "completion uCondLock with uMutexLock test: broadcast/wait, unlocked broadcast" << endl;
#endif
#if 1
		{						// broadcast/wait
			const unsigned int N = 5;
			testerCL_B t1( m );
			testerCL_LW_O *ti[N];
			for ( unsigned int i = 0; i < N; i += 1 ) {
				ti[i] = new testerCL_LW_O( m );
			} // for
			for ( unsigned int i = 0; i < N; i += 1 ) {
				delete ti[i];
			} // for
		}
		cout << "completion uCondLock with uOwnerLock test: broadcast/wait, unlocked broadcast" << endl;
#endif
	}
}


// Local Variables: //
// compile-command: "u++ -g Locks.cc" //
// End: //
