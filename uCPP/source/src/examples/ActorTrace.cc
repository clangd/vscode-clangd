// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2021
// 
// ActorTrace.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Fri May 21 07:42:26 2021
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri May 21 07:45:13 2021
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
using namespace std;
#include <uActor.h>

struct TMsg : public uActor::TraceMsg { int cnt = 0; } tmsg; // traceable message

_CorActor Trace {
	Allocation receive( Message & msg ) {
		Case( TMsg, msg ) { resume(); }
		else Case( StopMsg, msg ) return Finished;
		return Nodelete;
	} // Trace::receive

	void main() {
		cout << "Build Trace" << endl;
		for ( int i = 0; i < 4; i += 1 ) {
			tmsg.print();  *this | tmsg;				// send message to self
			suspend();
		} // for

		cout << "Move Cursor Back" << endl;
		for ( int i = 0; i < 4; i += 1 ) {
			tmsg.print();  tmsg.Return();				// move cursor back 1 hop
			suspend();
		} // for

		cout << "Move Cursor Head" << endl;
		tmsg.resume(); tmsg.print();					// move cursor to head
		suspend();

		cout << "Delete Trace" << endl;
		tmsg.retSender();  tmsg.reset();  tmsg.print();	// move cursor and delete all hops
		suspend();

		cout << "Build Trace" << endl;
		for ( int i = 0; i < 4; i += 1 ) {
			tmsg.print();  *this | tmsg;				// send message to self
			suspend();
		} // for

		cout << "Erase trace" << endl;
		tmsg.erase();  tmsg.print();					// remove hops, except cursor
		*this | uActor::stopMsg;
	} // Trace::main
}; // Trace

int main() {
	uActor::start();									// start actor system
	Trace trace;
	trace | tmsg;
	uActor::stop();										// wait for all actors to terminate
} // main

// Local Variables: //
// tab-width: 4 //
// End: //
