//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1997
// 
// EHM1.cc -- testing exception handling mechanism
// 
// Author           : Peter A. Buhr
// Created On       : Wed Nov 26 23:06:25 1997
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Nov 28 11:08:23 2024
// Update Count     : 269
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


//######################### uBaseCoroutine::uFailure #########################


_Exception XXX {
  public:
	XXX( const char * const msg ) : uBaseException( msg ) {}
};

_Exception YYY {
  public:
	YYY( const char * msg ) { setMsg( msg ); }
};

_Coroutine CoroutineDummy1 {
	bool res;
	void main() {
		if ( res ) {
			_Resume YYY( "resume test" );
		} else {
			_Throw XXX( "throw test" );
		} // if
	} // CoroutineDummy1::main
  public:
	CoroutineDummy1( bool r ) : res( r ) {}
	void mem() { resume(); }
}; // CoroutineDummy1

_Coroutine CoroutineDummy2 {
	void main() {
		CoroutineDummy1 dummy1( false );
		dummy1.mem();
	} // CoroutineDummy1::main
  public:
	void mem() { resume(); }
}; // CoroutineDummy1


//######################### uCondition::uWaitingFailure #########################


_Task TaskDummy1 {
	uCondition x;
  public:
	void mem() {
		x.wait();
	} // TaskDummy1::mem
  private:
	void main() {
		_Accept( mem );
	} // TaskDummy1::main
}; // TaskDummy1

_Task TaskDummy2 {
	TaskDummy1 &t1;

	void main() {
#if 0
		t1.mem();
#endif
		try {
			t1.mem();
		} catch( uCondition::WaitingFailure &ex ) {
			osacquire( cout ) << "handled exception uCondition::WaitingFailure : task " << ex.source().getName() << " (" << &(ex.source()) << ") found blocked task" << " " << uThisTask().getName() << " (" << &uThisTask() << ") on condition variable " << &ex.conditionId() << endl << endl;
		} // try
	} // TaskDummy2::main
  public:
	TaskDummy2( TaskDummy1 &t1 ) : t1( t1 ) {}
}; // TaskDummy2


//######################### uMutexFailure::uRendezvousFailure #########################


_Task TaskDummy3 {
	uBaseTask &t;
  public:
	TaskDummy3( uBaseTask &t ) : t( t ) {}
	void mem() {
	} // TaskDummy3::mem
  private:
	void main() {
		_Resume XXX( "test" ) _At t;
#if 0
		_Accept( mem );
#endif
		try {
			_Accept( mem );
		} catch ( uMutexFailure::RendezvousFailure &ex ) {
			osacquire( cout ) << "handled exception uMutexFailure::RendezvousFailure : accepted call fails from task " << ex.sourceName() << " (" << &(ex.source()) << ") to mutex member of task " << uThisTask().getName() << " (" << &uThisTask() << ")" << endl << endl;
		} // try
	} // TaskDummy3::main
}; // TaskDummy3


//######################### uMutexFailure::uEntryFailure (acceptor/signalled stack) #########################


_Task TaskDummy4 {
	uCondition temp;
  public:
	void mem() {
		temp.wait();
	} // TaskDummy4::mem
  private:
	void main() {
		_Accept( mem );					// let TaskDummy5 in so it can wait
		temp.signal();					// put TaskDummy5 on A/S stack
		_Accept( ~TaskDummy4 );				// uMain is calling the destructor
	} // TaskDummy4::main
}; // TaskDummy4

_Task TaskDummy5 {
	TaskDummy4 &t4;
  public:
	TaskDummy5( TaskDummy4 &t4 ) : t4( t4 ) {}
  private:
	void main() {
#if 0
		t4.mem();
#endif
		try {
			t4.mem();
		} catch( uMutexFailure::EntryFailure &ex ) {
			osacquire( cout ) << "handled exception uMutexFailure::EntryFailure : while executing mutex destructor, task " << ex.source().getName() << " (" << &(ex.source()) << ") found task " << uThisTask().getName() << " (" << &uThisTask() << ") " << ex.message() << endl << endl;
		} catch( ... ) {
			abort( "invalid exception" );
		} // try
	} // TaskDummy5::main
}; // TaskDummy5


//######################### uMutexFailure::uEntryFailure (entry queue) #########################


_Task TaskDummy6 {
  public:
  void mem() {
  } // TaskDummy6::mem
  private:
  void main() {
	  _Accept( ~TaskDummy6 );
  } // TaskDummy6::main
};

_Task TaskDummy7 {
	TaskDummy6 &t6;
  public:
	TaskDummy7( TaskDummy6 &t6 ) : t6( t6 ) {}
  private:
	void main() {
#if 0
		t6.mem();
#endif
		try {
			t6.mem();
		} catch( uMutexFailure::EntryFailure &ex ) {
			osacquire( cout ) << "handled exception uMutexFailure::EntryFailure : while executing mutex destructor, task " << ex.source().getName() << " (" << &(ex.source()) << ") found task " << uThisTask().getName() << " (" << &uThisTask() << ") " << ex.message() << endl << endl;
		} catch( ... ) {
			abort( "invalid exception" );
		} // try
	} // TaskDummy7::main
}; // TaskDummy7


int main() {
	//######################### uBaseCoroutine::UnhandledException #########################
	{
		CoroutineDummy1 dummy1( true );

		try {
			dummy1.mem();
		} catch( uBaseCoroutine::UnhandledException &ex ) {
			osacquire( cout ) << "handled exception uBaseCoroutine::UnhandledException : in coroutine " << uThisCoroutine().getName() << " (" << &uThisCoroutine() << ") raised non-locally from resumed coroutine " << ex.sourceName() << " (" << &(ex.source()) << ") because of " << ex.message() << "." << endl << endl;
			try {
				osacquire( cout ) << "before trigger" << endl;
				ex.triggerCause();
				osacquire( cout ) << "after trigger" << endl;
			} _CatchResume( YYY & ) {
				osacquire( cout ) << "CatchResume YYY" << endl;
			} // try
		} catch( ... ) {
			abort( "invalid exception" );
		} // try
	}
	{
		CoroutineDummy2 dummy2;

		try {
			dummy2.mem();
		} catch( uBaseCoroutine::UnhandledException &ex ) {
			osacquire( cout ) << "handled exception uBaseCoroutine::UnhandledException : in coroutine " << uThisCoroutine().getName() << " (" << &uThisCoroutine() << ") raised non-locally from resumed coroutine " << ex.sourceName() << " (" << &(ex.source()) << ") because of " << ex.message() << " through " << ex.unhandled() << " unhandled exceptions." << endl << endl;

			osacquire( cout ) << "Now triggering exception" << endl;
			try {
				ex.triggerCause();
			} _CatchResume ( uBaseCoroutine::UnhandledException &e ) {
				osacquire( cout ) << "Now triggering embedded UnhandledException" << endl;    
				e.triggerCause();
			} catch ( XXX &ex ) {
				osacquire( cout ) << "caught XXX : " << ex.message() << endl;
			} catch ( ... ) {
				abort( "invalid exception" );
			} // try
		} catch( ... ) {
			abort( "invalid exception" );
		} // try
	}
	//######################### uCondition::uWaitingFailure #########################

	TaskDummy1 * t1 = new TaskDummy1;
	TaskDummy2 * t2 = new TaskDummy2( *t1 );
	delete t1;						// delete t1 with t2 blocked on condition variable
	delete t2;

	//######################### uMutexFailure::uRendezvousFailure #########################
	{
		TaskDummy3 t3( uThisTask() );
		try {
			_Enable {
				t3.mem();
			} // _Enable
		} catch( XXX &ex ) {
		} catch( ... ) {
			abort( "invalid exception" );
		} // try
	}
	//######################### uMutexFailure::uEntryFailure (acceptor/signalled stack) #########################

	TaskDummy4 * t4 = new TaskDummy4;
	TaskDummy5 * t5 = new TaskDummy5( *t4 );
	delete t4;
	delete t5;

	//######################### uMutexFailure::uEntryFailure (entry queue) #########################

	TaskDummy6 * t6 = new TaskDummy6;
	TaskDummy7 * t7 = new TaskDummy7( *t6 );
	delete t6;
	delete t7;

	cout << "successful completion" << endl;
} // main
