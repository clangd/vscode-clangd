// 
// Copyright (C) Peter A. Buhr 2016
// 
// ActorInherit.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Fri Dec 23 17:05:06 2016
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 26 13:06:48 2022
// Update Count     : 13
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
#include <uActor.h>

_Actor B {
	Allocation receive( Message & ) { return Finished; }
	int i;
  protected:
	void preStart() {
		osacquire( cout ) << "B" << endl;
	} // B::preStart
  public:
	B() {}
	B( int i ) { B::i = i; }
}; // B

_Actor D : public B {
	void preStart() {
		B::preStart();									// call base member
		osacquire( cout ) << "D" << endl;
	} // D::preStart
	Allocation receive( Message & ) { return Finished; }
  public:
	D() {}
	D( int i ) : B( i ) {}
}; // D

int main() {
	uActor::start();									// start actor system
	B b;
	D d;
	// Output is non-deterministic because actor b or d may run first.
	b | uActor::stopMsg;
	d | uActor::stopMsg;
	uActor::stop();										// wait for all actors to terminate
} // main

// Local Variables: //
// compile-command: "u++-work -g -O2 -multi ActorInherit.cc" //
// End: //
