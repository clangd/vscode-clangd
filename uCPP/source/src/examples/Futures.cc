//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2015
// 
// Futures.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Thu Dec 27 08:49:10 2015
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Mar 14 09:14:34 2022
// Update Count     : 6
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
using namespace std;
#include <uFuture.h>

_Task Worker {
	Future_ISM<int> &f;

	void main() {
		yield( rand() % 10 );
		f.delivery( 3 );
		yield( rand() % 10 );
	} // Worker::main
  public:
	Worker( Future_ISM<int> &f ) : f( f ) {}
}; // Worker

enum { NoOfTime = 10 };

_Task Worker2 {
	Future_ISM<int> &f;

	void main() {
		for ( unsigned int i = 0; i < NoOfTime; i += 1 ) {
			yield( rand() % 10 );
			f.delivery( 3 );
			while ( f.available() ) yield();
		} // for
	} // Worker2::main
  public:
	Worker2( Future_ISM<int> &f ) : f( f ) {}
}; // Worker2


int main() {
	uProcessor p[3];
	int check;

	srand( getpid() ) ;
	{
		Future_ISM<int> f;
		Worker w( f );
		check = 0;
		_When( false ) _Select( f ) { check += 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[2];
		Worker w1( f[0] ), w2( f[1] );
		check = 0;
		_When( false ) _Select( f[0] || f[1] ) { check += 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[2];
		Worker w1( f[0] ), w2( f[1] );
		check = 0;
		_When( false ) _Select( f[0] || f[1] ) { check += 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[2];
		Worker w1( f[0] ), w2( f[1] );
		check = 0;
		_When( false ) _Select( f[0] ) { check += 1; }
		or
			_When( false ) _Select( f[1] ) { check -= 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[2];
		Worker w1( f[0] ), w2( f[1] );
		check = 0;
		_When( true ) _Select( f[0] ) { check += 1; }
		or
			_When( false ) _Select( f[1] ) { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[2];
		Worker w1( f[0] ), w2( f[1] );
		check = 0;
		_When( false ) _Select( f[0] ) { check += 1; }
		or
			_When( true ) _Select( f[1] ) { check -= 1; }
		assert( check == -1 );
	}
	{
		Future_ISM<int> f[2];
		Worker w1( f[0] ), w2( f[1] );
		check = 0;
		_When( true ) _Select( f[0] ) { check += 1; }
		or
			_When( true ) _Select( f[1] ) { check -= 1; }
		assert( check == 1 || check == -1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] || f[1] || f[2] ) { check += 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] || f[1] || f[2] ) { check += 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] || f[1] ) { check += 1; }
		or
			_When( false ) _Select( f[2] ) { check -= 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] || f[1] ) { check += 1; }
		or
			_When( false ) _Select( f[2] ) { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] || f[1] ) { check += 1; }
		or
			_When( true ) _Select( f[2] ) { check -= 1; }
		assert( check == -1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] || f[1] ) { check += 1; }
		or
			_When( true ) _Select( f[2] ) { check -= 1; }
		assert( check == 1 || check == -1 );
	}

	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] ) { check += 1; }
		or
			_When( false ) _Select( f[1] || f[2] ) { check -= 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] ) { check += 1; }
		or
			_When( false ) _Select( f[1] || f[2] ) { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] ) { check += 1; }
		or
			_When( true ) _Select( f[1] || f[2] ) { check -= 1; }
		assert( check == -1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] ) { check += 1; }
		or
			_When( true ) _Select( f[1] || f[2] ) { check -= 1; }
		assert( check == 1 || check == -1 );
	}

	{
		Future_ISM<int> f[2];
		Worker w1( f[0] ), w2( f[1] );
		check = 0;
		_When( false ) _Select( f[0] && f[1] ) { check += 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[2];
		Worker w1( f[0] ), w2( f[1] );
		check = 0;
		_When( true ) _Select( f[0] && f[1] ) { check += 1; }
		assert( check == 1 );
	}

	{
		Future_ISM<int> f[2];
		Worker w1( f[0] ), w2( f[1] );
		check = 0;
		_When( false ) _Select( f[0] ) { check += 1; }
		and
			_When( false ) _Select( f[1] ) { check -= 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[2];
		Worker w1( f[0] ), w2( f[1] );
		check = 0;
		_When( true ) _Select( f[0] ) { check += 1; }
		and
			_When( false ) _Select( f[1] ) { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[2];
		Worker w1( f[0] ), w2( f[1] );
		check = 0;
		_When( false ) _Select( f[0] ) { check += 1; }
		and
			_When( true ) _Select( f[1] ) { check -= 1; }
		assert( check == -1 );
	}
	{
		Future_ISM<int> f[2];
		Worker w1( f[0] ), w2( f[1] );
		check = 0;
		_When( true ) _Select( f[0] ) { check += 2; }
		and
			_When( true ) _Select( f[1] ) { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] && f[1] && f[2] ) { check += 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] && f[1] && f[2] ) { check += 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] && f[1] ) { check += 1; }
		and
			_When( false ) _Select( f[2] ) { check -= 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] && f[1] ) { check += 1; }
		and
			_When( false ) _Select( f[2] ) { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] && f[1] ) { check += 1; }
		and
			_When( true ) _Select( f[2] ) { check -= 1; }
		assert( check == -1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] && f[1] ) { check += 2; }
		and
			_When( true ) _Select( f[2] ) { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] ) { check += 1; }
		and
			_When( false ) _Select( f[1] && f[2] ) { check -= 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] ) { check += 1; }
		and
			_When( false ) _Select( f[1] && f[2] ) { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] ) { check += 1; }
		and
			_When( true ) _Select( f[1] && f[2] ) { check -= 1; }
		assert( check == -1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] ) { check += 2; }
		and
			_When( true ) _Select( f[1] && f[2] ) { check -= 1; }
		assert( check == 1 );
	}

	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] || (f[1] && f[2]) ) { check += 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] || (f[1] && f[2]) ) { check += 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] || f[1] ) { check += 1; }
		and
			_When( false ) _Select( f[2] ) { check -= 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] || f[1] ) { check += 1; }
		and
			_When( false ) _Select( f[2] ) { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] || f[1] ) { check += 1; }
		and
			_When( true ) _Select( f[2] ) { check -= 1; }
		assert( check == -1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] || f[1] ) { check += 2; }
		and
			_When( true ) _Select( f[2] ) { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] ) { check += 1; }
		or
			_When( false ) _Select( f[1] && f[2] ) { check -= 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] ) { check += 1; }
		or
			_When( false ) _Select( f[1] && f[2] ) { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] ) { check += 1; }
		or
			_When( true ) _Select( f[1] && f[2] ) { check -= 1; }
		assert( check == -1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] ) { check += 1; }
		or
			_When( true ) _Select( f[1] && f[2] ) { check -= 1; }
		assert( check == 1 || check == -1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( (f[0] && f[1]) || f[2] ) { check += 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( (f[0] && f[1]) || f[2] ) { check += 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] && f[1] ) { check += 1; }
		or
			_When( false ) _Select( f[2] ) { check -= 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] && f[1] ) { check += 1; }
		or
			_When( false ) _Select( f[2] ) { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] && f[1] ) { check += 1; }
		or
			_When( true ) _Select( f[2] ) { check -= 1; }
		assert( check == -1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] && f[1] ) { check += 1; }
		or
			_When( true ) _Select( f[2] ) { check -= 1; }
		assert( check == 1 || check == -1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] ) { check += 2; }
		and
			_When( false ) _Select( f[1] || f[2] ) { check -= 1; }
		assert( check == 0 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] ) { check += 1; }
		and
			_When( false ) _Select( f[1] || f[2] ) { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( false ) _Select( f[0] ) { check += 1; }
		and
			_When( true ) _Select( f[1] || f[2] ) { check -= 1; }
		assert( check == -1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		_When( true ) _Select( f[0] ) { check += 2; }
		and
			_When( true ) _Select( f[1] || f[2] ) { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		( _Select( f[0] ); and _Select( f[1] ) { check += 1; } )
			or
			( _Select( f[1] ) { check -= 2; } or _Select( f[2] ) { check -= 3; } )
			assert( check == 1 || check == -1 || check == -2 || check == -3 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		( _Select( f[0] ) { check += 1; } or _Select( f[1] ) { check += 2; } )
			and
			( _Select( f[1] ) { check -= 3; } or _Select( f[2] ) { check -= 4; } )
			assert( check == -2 || check == -3 || check == -1 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		( _Select( f[0] ) { check += 2; } and _Select( f[1] ) { check += 3; } )
			and
			( _Select( f[1] ) { check -= 6; } or _Select( f[2] ) { check -= 7; } )
			assert( check == -1 || check == -2 );
	}
	{
		Future_ISM<int> f[3];
		Worker w1( f[0] ), w2( f[1] ), w3( f[2] );
		check = 0;
		( _Select( f[0] ) { check += 2; } and _Select( f[1] ) { check += 3; } )
			and
			( _Select( f[1] ) { check -= 6; } and _Select( f[2] ) { check -= 7; } )
			assert( check == -8 );
	}
	{
		Future_ISM<int> f;
		Worker w( f );
		check = 0;
		_When( true ) _Select( f ) { check += 1; }
		_When( false ) _Else { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f;
		Worker w( f );
		check = 0;
		_When( false ) _Select( f ) { check += 1; }
		_When( true ) _Else { check -= 1; }
		assert( check == -1 );
	}
	{
		Future_ISM<int> f;
		Worker w( f );
		check = 0;
		_When( true ) _Select( f ) { check += 1; }
		_When( true ) _Else { check -= 1; }
		assert( check == -1 );
	}
	{
		Future_ISM<int> f;
		Worker w( f );
		check = 0;
		_When( true ) _Select( f ) { check += 1; }
		or _When( false ) _Timeout( uDuration( 0, 10000000 ) ) { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f;
		Worker w( f );
		check = 0;
		_When( false ) _Select( f ) { check += 1; }
		or _When( true ) _Timeout( uDuration( 0, 10000000 ) ) { check -= 1; }
		assert( check == -1 );
	}
	{
		Future_ISM<int> f;
		//Worker w( f );
		check = 0;
		_When( false ) _Select( f ) { check += 1; }
		or _When( true ) _Timeout( uDuration( 0, 10000000 ) ) { check -= 1; }
		assert( check == -1 );
	}
	{
		Future_ISM<int> f;
		Worker w( f );
		check = 0;
		_When( true ) _Select( f ) { check += 1; }
		or _When( true ) _Timeout( uDuration( 0, 10000000 ) ) { check -= 1; }
		assert( check == 1 );
	}
	{
		Future_ISM<int> f;
		//Worker w( f );
		check = 0;
		_When( true ) _Select( f ) { check += 1; }
		or _When( true ) _Timeout( uDuration( 0, 10000000 ) ) { check -= 1; }
		assert( check == -1 );
	}
	{
		Future_ISM<int> f[3];
		Worker2 w1( f[0] ), w2( f[1] ), w3( f[2] );

		for ( unsigned int i = 0; i < NoOfTime; i += 1 ) {
			//std::osacquire( std::cerr ) << "X " << i << endl;
			_Select( f[0] || (f[1] && f[2]) ) {
				// keep workers and the enclosing loop in lock step
				while ( ! f[0].available() ) uBaseTask::yield();
				while ( ! f[1].available() ) uBaseTask::yield();
				while ( ! f[2].available() ) uBaseTask::yield();
				f[0].reset();  f[1].reset();  f[2].reset();
			} // _Select
		} // for
	}
	{
		Future_ISM<int> f[3];
		Worker2 w1( f[0] ), w2( f[1] ), w3( f[2] );
		int cnt;
		bool check;

		for ( unsigned int i = 0; i < NoOfTime; i += 1 ) {
			check = true;
			//std::osacquire( std::cerr ) << "Y " << i << endl;

			cnt = 0;
			_Select( f[0] ) {
				assert( check );
				check = false;
				// keep workers and the enclosing loop in lock step
				while ( ! f[1].available() ) uBaseTask::yield();
				while ( ! f[2].available() ) uBaseTask::yield();
				f[0].reset();  f[1].reset();  f[2].reset();
			} or _Select( f[1] ) {
				assert( check );
				if ( cnt == 0 ) cnt += 1;
				else {
					check = false;
					// keep workers and the enclosing loop in lock step
					while ( ! f[0].available() ) uBaseTask::yield();
					while ( ! f[2].available() ) uBaseTask::yield();
					f[0].reset();  f[1].reset();  f[2].reset();
				} // if
			} and _Select( f[2] ) {
				assert( check );
				if ( cnt == 0 ) cnt += 1;
				else {
					check = false;
					// keep workers and the enclosing loop in lock step
					while ( ! f[0].available() ) uBaseTask::yield();
					while ( ! f[1].available() ) uBaseTask::yield();
					f[0].reset();  f[1].reset();  f[2].reset();
				} // fi
			} // _Select
		} // for
	}

	cout << "successful completion" << endl;
} // main
