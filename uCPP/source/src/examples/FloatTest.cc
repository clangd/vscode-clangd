//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Robert Denda 1996
// 
// FloatTest.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Wed May 11 17:30:51 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Wed Apr 20 23:05:29 2022
// Update Count     : 86
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

#include <cmath>
#ifndef X_EPS
#define X_EPS 1.0E-8
#endif
#include <iostream>
using std::cout;
using std::endl;

unsigned int uDefaultPreemption() {
	return 1;											// set time-slice to one millisecond
} // uDefaultPreemption


const double Range = 100000.0;


_Task Tester {
	double result;
	const int TaskId;

	void main() {
		double d;	     
		uFloatingPointContext context;					// save/restore floating point registers during context switch

		// Each task increments a counter through a range of values and calculates a trigonometric identity of each
		// value in the range. There should be numerous context switches while performing this calculation. If the
		// floating point registers are not saved properly for each task, the calculations interfere producing erroneous
		// results.

		for ( d = TaskId * Range; d < (TaskId + 1) * Range; d += 1.0 ) {
			yield();
			if ( fabs( sqrt( pow( sin( d ), 2.0 ) + pow( cos( d ), 2.0 ) ) - 1.0 ) > X_EPS ) { // self-check 
				abort( "invalid result during Tester[%d]:%6f", TaskId, d );
			} // if
			if ( d < TaskId * Range || d >= (TaskId + 1) * Range ) { // self-check 
				abort( "invalid result during Tester[%d]:%6f", TaskId, d );
			} // if
		} // for
		result = d;
	} // Test::main
  public:
	Tester( int TaskId ) : TaskId( TaskId ) {}
	double Result() { return result; }
}; // Tester


int main() {
	const int numTesters = 10;
	Tester *testers[numTesters];
	int i;
	uFloatingPointContext context;						// save/restore floating point registers during a context switch
	uProcessor p[3];

	for ( i = 0; i < numTesters; i += 1 ) {				// create tasks
		testers[i] = new Tester(i);
		uBaseTask::yield();
	} // for

	for ( i = 0; i < numTesters; i += 1 ) {				// recover results and delete tasks
		double result = testers[i]->Result();
		if ( result != (i + 1) * Range ) {				// check result
			abort( "invalid result in Tester[%d]:%6f", i, result );
		} // if
		delete testers[i];
	} // for

	cout << "successful completion" << endl;
} // main
	
// Local Variables: //
// compile-command: "u++ -O2 FloatTest.cc" //
// End: //
