// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2016
// 
// ActorPingPong.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Dec 19 08:24:00 2016
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Jun 14 17:25:26 2022
// Update Count     : 74
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

#ifdef NOOUTPUT
#define PRT( stmt )
#else
#define PRT( stmt ) stmt
#endif // NOOUTPUT

struct Token : public uActor::SenderMsg { int cnt; } tokenMsg;

// Pass a counter message between a pair of actors. Each actor decrements the counter by 1 before forwarding to its
// partner. Both actors terminate when the counter reaches 0.

_Actor Ping {
	int cycle = 0, cycles;

	Allocation receive( Message & msg ) {
		Case( Token, msg ) {
			int & cnt = msg_d->cnt;						// optimization
			PRT( cout << "ping " << cnt << endl; );
			cnt -= 1;
			if ( cnt > 0 ) { *msg_d->sender() | tokenMsg; return Nodelete; }
			if ( cnt == 0 ) { *msg_d->sender() | tokenMsg; } // special case to stop partner
		} // Case
		return Finished;
	} // Ping::receive
}; // Ping

_Actor Pong {
	Allocation receive( Message & msg ) {
		Case( Token, msg ) {
			int & cnt = msg_d->cnt;						// optimization
			PRT( cout << "pong " << cnt << endl; )
			cnt -= 1;
			if ( cnt > 0 ) { *msg_d->sender() | tokenMsg; return Nodelete; }
			if ( cnt == 0 ) { *msg_d->sender() | tokenMsg; } // special case to stop partner
		} // Case
		return Finished;
	} // Pong::receive
}; // Pong

int main( int argc, char * argv[] ) {
	int Cycles = 10;									// default values

	try {
		switch ( argc ) {
		  case 2:
			Cycles = stoi( argv[1] );
			if ( Cycles < 1 ) throw 1;
		  case 1:										// use defaults
			break;
		  default:
			throw 1;
		} // switch
	} catch( ... ) {
		cout << "Usage: " << argv[0] << " [ cycles (> 0) ]" << endl;
		exit( EXIT_SUCCESS );
	} // try

	tokenMsg.cnt = Cycles;
	uActor::start();									// start actor system
	Ping ping;
	Pong pong;
	ping.tell( tokenMsg, &pong );						// start cycling
	uActor::stop();										// wait for all actors to terminate
} // main

// Local Variables: //
// compile-command: "u++-work -g -O2 -multi -nodebug ActorPingPong.cc" //
// End: //
