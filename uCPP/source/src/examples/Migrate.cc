//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// Migrate.cc -- Test migrating of tasks among clusters.
// 
// Author           : Peter A. Buhr
// Created On       : Tue Jun 28 10:23:11 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Apr 25 20:51:24 2022
// Update Count     : 45
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
using std::endl;

unsigned int uDefaultPreemption() {
	return 1;
} // uDefaultPreemption

unsigned int uDefaultSpin() {
	return 10;											// keep small for 1-core computer
} // uDefaultSpin

_Task Worker {
	uCluster &cluster1, &cluster2, &cluster3;

	void main() {
		const int NoOfTimes = 10000;

	    for ( int i = 0; i < NoOfTimes; i += 1 ) {
			migrate( cluster3 );
			migrate( cluster2 );
			migrate( cluster1 );
	    } // for
	} // Worker::main
  public:
	Worker( uCluster &cluster1, uCluster &cluster2, uCluster &cluster3 ) : uBaseTask( cluster1 ), cluster1( cluster1 ), cluster2( cluster2 ), cluster3( cluster3 ) {
	} // Worker::Worker
}; // Worker

uCluster cluster1( "cluster1" );
uProcessor processor1( cluster1 );

uCluster cluster2( "cluster2" );
uProcessor processor2( cluster2 );

int main() {
	{
		Worker task1( uThisCluster(), cluster1, cluster2 ), task2( cluster2, uThisCluster(), cluster1 ), task3( cluster1, cluster2, uThisCluster() );
	}
	cout << "successful completion" << endl;
} // main

// Local Variables: //
// compile-command: "u++ Migrate.cc" //
// End: //
