//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1998
// 
// EHM3.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue Oct 27 21:24:48 1998
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Oct  7 08:07:23 2023
// Update Count     : 55
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

const int NUM = 3;

_Exception R1 {
  public:
	int &i; char &c;
	R1( int &i, char &c ) : i( i ), c( c ) {}
};

_Exception R2 {};

_Exception R3 {};


void f( int x, char y ) {
	osacquire( cout ) << "enter f, x:" << x << " y:" << y << endl;
	_Resume R2();
	osacquire( cout ) << "exit f, x:" << x << " y:" << y << endl;
}
void g( int &x, char &y ) {
	osacquire( cout ) << "enter g, x:" << x << " y:" << y << endl;
	_Resume R1( x, y );
	osacquire( cout ) << "exit g, x:" << x << " y:" << y << endl;
}

_Exception D {};

_Task T1 {
	uBaseTask &partner;

	void main() {
		osacquire( cout ) << "T1::main enter" << endl;
		{
			try {
				uBaseTask &p = uThisTask();
				int count = NUM;
				try {
					_Resume D() _At uThisTask();
					_Enable;
				} _CatchResume( D ) {
					count -= 1;
					osacquire( cout ) << "Handler R: first" << endl;
					if ( count > 0 ) {
						// note that recursive resumption is possible here because of the asynchronous nature of the raise
						_Resume _At p;
					} // if
				} // try
				_Enable;
			} _CatchResume( D ) {
				osacquire( cout ) << "Handler second" << endl;
			} // try
		}
		osacquire( cout ) << "T1::main exit ^^ should be " << NUM + 1 << " identical lines above" << endl;
	}
  public:
	T1( uBaseTask &partner ) : partner( partner ) {}
};

_Task T2 {
	uBaseTask &partner;

	void main() {
		osacquire( cout ) << "T2::main enter" << endl;
		{
			try {
				try {
					_Resume D() _At uThisTask();
					_Enable;
				} catch( D & ) {
					osacquire( cout ) << "first" << endl;
					_Resume _At uThisTask();
					_Enable;
				} catch (...) {
					assert( false );
				} // try
			} catch( D & ) {
				osacquire( cout ) << "second" << endl;
			}
		}
		osacquire( cout ) << "T2::main middle" << endl;
		{
			try {
				_Enable;
			} catch( D & ) {
				osacquire( cout ) << "first" << endl;
				_Resume _At partner;
			}
		}
		osacquire( cout ) << "T2::main exit" << endl;
	}
  public:
	T2( uBaseTask &partner ) : partner( partner ) {}
};


int main() {
	int x = 1;
	char y = 'a';
	try {
		g( x, y );
		osacquire( cout ) << "try<R1,rtn1> x:" << x << " y:" << y << endl;

		// Check multiple handlers, only one is used.
		try {
			f( x, y );
			osacquire( cout ) << "try<R2,rtn2>, x:" << x << " y:" << y << endl;
		} _CatchResume( R1 ) {
		} _CatchResume( R2 ) {
			osacquire( cout ) << "rtn2, i:" << x << " c:" << y << endl;
			x = 2; y = 'c';
		} // try
		try {											// R1 empty handler
			g( x, y );
			osacquire( cout ) << "try<R1>, x:" << x << " y:" << y << endl;
		} _CatchResume( R1 ) {
		} _CatchResume( R2 ) {
			osacquire( cout ) << "rtn2, i:" << x << " c:" << y << endl;
			x = 2; y = 'c';
		} // try
	} _CatchResume( R1 &r ) {
		osacquire( cout ) << "rtn1" << endl;
		r.i = 3; r.c = 'b';
	} // try

	osacquire( cout ) << "=============" << endl;

	try {												// test resume-any
		try {
			_Resume R1( x, y );
			osacquire( cout ) << "[1] First raise is ignored and returns to here" << endl;
		} _CatchResume( ... ) {
		}
		_Resume R3();
		osacquire( cout ) << "[3] Second raise returns to here" << endl;
	} _CatchResume( ... ) {
		osacquire( cout ) << "[2] Second raise handled here by resume-any handler" << endl;
	} // try

	osacquire( cout ) << "=============" << endl;
	{													// test nonlocal reraise
		T1 t( uThisTask() );
		try {
			_Enable {
				_Resume D() _At t;
				uBaseTask::yield();						// let t run
			}
		} _CatchResume( D ) {
			osacquire( cout ) << "Handler second" << endl;
		} // try
	}
	osacquire( cout ) << "=============" << endl;
	{													// test nonlocal rethrow
		T2 t( uThisTask() );
		try {
			_Enable {
				_Resume D() _At t;
				uBaseTask::yield();						// let t run
			}
		} catch( D & ) {
			osacquire( cout ) << "second" << endl;
		} // try
	}
} // main

// Local Variables: //
// tab-width: 4 //
// End: //
