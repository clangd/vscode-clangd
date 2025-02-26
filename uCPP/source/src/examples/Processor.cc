//                              -*- Mode: C++ -*- 
// 
// Copyright (C) Peter A. Buhr 2016
// 
// Processor.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Dec 19 22:28:04 2016
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Apr 21 18:39:13 2022
// Update Count     : 2
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

volatile bool stop = false;

_Task Worker {
	void main() {
		while ( ! stop ) {
			yield();
		}
	}
};

int main() {
	uProcessor *processor2 = nullptr;
	{
		Worker tasks[2];

		uProcessor *processor1 = new uProcessor();

		while ( &uThisProcessor() != processor1 ) {
			yield();
		} // while

		processor2 = new uProcessor();

//	delete processor2;
		delete processor1;

		stop = true;
	}
	cout << "here" << endl;
	delete processor2;
} // main

// Local Variables: //
// compile-command: "../../bin/u++ -g -multi Processor.cc" //
// End: //
