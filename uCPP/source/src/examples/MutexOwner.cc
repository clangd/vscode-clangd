//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1996
// 
// MutexOwner.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue Feb  6 13:51:59 1996
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 26 15:02:03 2022
// Update Count     : 26
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

_Cormonitor tom {
  public:
  tom() {
	  reset();
  }
  void reset() {
	  osacquire( cout ) << "(" << &uThisTask() << ") tom::reset" << endl;
  }
};    

_Cormonitor fred : public tom {
	void main() {
		for ( int i = 0; i < 3; i += 1 ) {
			mem();
		} // for
		resume();
	}
  public:
	fred() {
		reset();
	}
	void reset() {
		osacquire( cout ) << "(" << &uThisTask() << ") fred::reset" << endl;
	}
	void mem() {
		resume();
	}
};

_Monitor mary {
	uCondition delay;
  public:
	void mem( int i ) {
		if ( i >= 0 ) {
			mem( i - 1 );
			osacquire( cout ) << "(" << &uThisTask() << ") mary::mem, i:" << i << endl;
		} else {
			if ( delay.empty() ) {
				delay.wait();
			} else {
				delay.signal();
			} // if
		} // if
	} // mary::mem
}; // mary

_Task jane {
	mary &m;
  public:
	jane( mary &m ) : m( m ) {}
	void mem( int i ) {
		if ( i == 2 ) {
			m.mem( i );
		} // if
	}
  protected:
	void main() {
		for ( int i = 0; i < 3; i += 1 ) {
			osacquire( cout ) << "(" << &uThisTask() << ") jane::main, i:" << i << endl;
			mem( i );
		} // for
	} // jane::main
}; // jane

int main() {
	fred f;
	f.mem();

	mary m;
	jane j1( m );
	jane j2( m );
} // main

// Local Variables: //
// compile-command: "../../bin/u++ -multi -g MutexOwner.cc" //
// End: //
