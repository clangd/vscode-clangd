//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2012
// 
// TimeoutProcessor.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Thu Apr  5 08:06:57 2012
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Apr 24 18:27:38 2022
// Update Count     : 8
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

// The processor is removed from cluster so task "t" cannot run after the timeout moves it to the ready queue on cluster
// "clus". When the processor is returned to "clus", the task "t" can wakeup from the sleep and continue.

_Task T {
	void main() {
		osacquire( cout ) << "start " << uThisCluster().getName() << " " << &uThisProcessor() << endl;
		sleep( uDuration( 5 ) );
		osacquire( cout ) << "done " << uThisCluster().getName() << " " << &uThisProcessor() << endl;
	} // T:: main
  public:
	T( uCluster &clus ) : uBaseTask( clus ) {}
}; // T

int main() {
	uCluster clus( "clus" );
	osacquire( cout ) << "cluster: " << &clus << endl;
	uProcessor p;
	T t( clus );
	osacquire( cout ) << "over" << endl;
	p.setCluster( clus );
	sleep( 1 );
	osacquire( cout ) << "back" << endl;
	p.setCluster( uThisCluster() );
	sleep( 10 );
	osacquire( cout ) << "over" << endl;
	p.setCluster( clus );
	osacquire( cout ) << "finish" << endl;
} // main

// Local Variables: //
// compile-command: "u++ TimeoutProcessor.cc" //
// End: //
