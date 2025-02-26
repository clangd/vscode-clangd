// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2016
// 
// ActorPromise.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Dec 19 08:22:37 2016
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Jul 22 10:52:22 2022
// Update Count     : 726
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
#include <string>
using namespace std;
#include <uActor.h>

struct IntMsg : public uActor::PromiseMsg<int> { int val; };
struct StrMsg : public uActor::PromiseMsg<string> { string val; };
struct IntStrMsg : public uActor::PromiseMsg< int > { string val; };
static struct ClientStopMsg : public uActor::Message {} clientStopmsg;

int Delay = 10;
#define delay( N ) for ( volatile int delay = 0; delay < N; delay += 1 ) {}

_Actor Server {
	const unsigned int NoOfClients;
	unsigned int cntClients = 0;

	Allocation receive( uActor::Message & msg ) {
        delay( Delay )							// pretend to perform client work
		// work
		Case( IntMsg, msg ) { msg_d->delivery( 7 ); }
		else Case( StrMsg, msg ) { msg_d->delivery( "XYZ" ); }
		else Case( IntStrMsg, msg ) { msg_d->delivery( 12 ); }
		// admin
		else Case( ClientStopMsg, msg ) {
			cntClients += 1;
			if ( cntClients == NoOfClients ) return Delete; // delete actor
		// errors
		} else Case( UnhandledMsg, msg ) {				// receiver complained
			abort( "sent unknown message to %p", msg_d->sender() );
		} else {										// unknown void message
			osacquire( cout ) << "server unhandled" << endl;
			*msg_d->sender() | uActor::unhandledMsg;	// complain to sender
		} // Case
		return Nodelete;								// reuse actor
	} // Server::receive
  public:
	Server( unsigned int NoOfClients ) : NoOfClients( NoOfClients ) {}
}; // Server


_Actor Client {
	enum { MsgKinds = 3,								// number of message kinds
		   Messages = 100,								// number of message kinds sent
		   Times = 1000  };								// number of send repetitions

	Server & server;
	unsigned int times = 0, processed = 0, maybes = 0, callbacks = 0, tmaybes = 0, tcallbacks = 0;

	IntMsg intmsg[Messages];
	StrMsg strmsg[Messages];
	IntStrMsg intstrmsg[Messages];

	uActor::Promise< int > pi[Messages];
	uActor::Promise< string > ps[Messages];
	uActor::Promise< int > pis[Messages];

	#define ICB [this, li = i]( int ) { callbacks += 1; *this | intmsg[li]; return Nodelete; }
	#define SCB [this, li = i]( string ) { callbacks += 1; *this | strmsg[li]; return Nodelete; }

	void preStart() {
		become( &Client::send );
		*this | uActor::startMsg;
	} // Client::preStart

	Allocation shutdown() {
		tmaybes += maybes; tcallbacks += callbacks;
		times += 1;
		if ( times == Times ) {
			server | clientStopmsg;						// terminate server
			osacquire( cout ) << "client maybe " << tmaybes << " callbacks " << tcallbacks << endl;
			return Delete;								// terminate client
		} // if
		// Reset internal state and send a restart message.
		processed = maybes = callbacks = 0;
		for ( unsigned int i = 0; i < Messages; i += 1 ) { pi[i].reset(); ps[i].reset(); pis[i].reset(); }
		restart();										// call preStart
		return Nodelete;								// reuse actor
	} // Client::shutdown

	Allocation send( uActor::Message & ) {
		for ( unsigned int i = 0; i < Messages; i += 1 ) { // send out work
			intmsg[i].val = 4;
			pi[i] = server || intmsg[i];				// ask messages
			strmsg[i].val = "DEF";
			ps[i] = server || strmsg[i];				// store promise
		} // for
		delay( Delay );									// work asynchronously
		// check for finished work
		for ( unsigned int i = 0; i < Messages; i += 1 ) { // any promise fulfilled ?
			#ifdef THEN
			pi[i].then( ICB );
			ps[i].then( SCB );
			#else
			if ( pi[i].maybe( ICB ) ) { maybes += 1; assert( pi[i]() == 7 ); }
			if ( ps[i].maybe( SCB ) ) { maybes += 1; assert( ps[i]() == "XYZ" ); }
			#endif // THEN
		} // for

		for ( unsigned int i = 0; i < Messages; i += 1 ) { // send out work
			intstrmsg[i].val = "DEF";
			pis[i] = server || intstrmsg[i];			// store promise
		} // for
		delay( Delay );									// work asynchronously
		// check for finished work
		for ( unsigned int i = 0; i < Messages; i += 1 ) { // access promise values
			#ifdef THEN
			pis[i].then( ICB );
			#else
			if ( pis[i].maybe( ICB ) ) { maybes += 1; assert( pis[i]() == 12 ); }
			#endif // THEN
		} // for

		if ( maybes == MsgKinds * Messages ) return shutdown(); // all requests fulfilled ?
		processed = maybes;								// otherwise receive callback messages
		become( &Client::receive );
		return Nodelete;								// reuse actor
	} // Client::send

	Allocation receive( uActor::Message & msg ) {		// receive callback messages
		Case( IntMsg, msg ) {							// ask messages
			processed += 1; assert( (*msg_d)() == 7 || (*msg_d)() == 12 ); delay( 500 ); // touch result
		} else Case( StrMsg, msg ) {
			processed += 1; assert( (*msg_d)() == "XYZ" ); delay( 500 ); // touch result
		// error cases
		} else Case( UnhandledMsg, msg ) {				// receiver complained
			abort( "sent unknown message to %p", msg_d->sender() );
		} else {										// unknown void message
			osacquire( cout ) << "client unhandled " << &msg << ' ' << msg_d->sender() << endl;
			*msg_d->sender() | uActor::unhandledMsg;		// complain to sender
			return Nodelete;							// reuse actor
		} // Case

		if ( processed == MsgKinds * Messages ) return shutdown(); // all requests fulfilled ?
		return Nodelete;								// reuse actor
	} // Client::receive
  public:
	Client( Server & server ) : server( server ) {}
}; // Client


int main( int argc, char * argv[] ) {
    switch ( argc ) {
	  case 2:
		if ( strcmp( argv[1], "d" ) != 0 ) {			// default ?
			if ( stoi( argv[1] ) < 0 ) goto Usage;
			Delay = stoi( argv[1] );
		} // if
	  case 1:											// use defaults
		break;
	  default:
	  Usage:
		cerr << "Usage: " << argv[0]
			 << " [ delay >= 0 | d ]" << endl;
		exit( EXIT_FAILURE );
	} // switch

	enum { Times = 1, NoOfServers = 1, NoOfClients = 2 };
	uProcessor p[NoOfServers + NoOfClients - 1];		// processor for server and client

	Server * server;
	for ( unsigned int t  = 0; t < Times; t += 1 ) {
		uActor::start();								// wait for all actors to terminate

		for ( unsigned int s = 0; s < NoOfServers; s += 1 ) {
			server = new Server( NoOfClients );
			for ( unsigned int c = 0; c < NoOfClients; c += 1 ) {
				new Client( *server );
			} // for
		} // for

		uActor::stop();									// wait for all actors to terminate
	} // for
	// malloc_stats();
	// UPP::Statistics::print();
} // main

// Local Variables: //
// compile-command: "u++-work -g -O2 -nodebug -multi ActorPromise.cc" //
// End: //
