//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uCalibrate.cc -- Calibrate the number of iterations of a set piece of code to produce a 100 microsecond delay.
// 
// Author           : Peter A. Buhr
// Created On       : Fri Aug 16 14:12:08 1991
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Apr 24 18:21:30 2022
// Update Count     : 43
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

#define TIMES 5'000'000'000LL							//' cannot be larger or overflow occurs

int main() {
	uTime start = uClock::getCPUTime();
	for ( volatile unsigned long long int i = 1; i <= TIMES; i += 1 ) {
	} // for
	cout << "#define ITERATIONS_FOR_100USECS " << 100000LL * TIMES / ( (uClock::getCPUTime() - start).nanoseconds() ) << endl;
} // main

// Local Variables: //
// compile-command: "u++ uCalibrate.cc" //
// End: //
