// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// ServerUNIXSTREAM.cc -- Server for UNIX/stream socket test. Server accepts multiple connections from clients. Each
//     client then communicates with an acceptor.  The acceptor reads the data from the client and writes it back.
// 
// Author           : Peter A. Buhr
// Created On       : Tue Jan  7 08:40:22 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Jan 20 08:04:38 2024
// Update Count     : 158
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

#include <uSocket.h>
#include <iostream>
#include <unistd.h>										// unlink
using std::cerr;
using std::osacquire;
using std::endl;

enum { BufferSize = 8 * 1024 };
const char EOD = '\377';
const char EOT = '\376';

_Task Server;											// forward declaration

_Task Acceptor {
	uSocketServer &sockserver;
	Server &server;

	void main();
  public:
	Acceptor( uSocketServer &socks, Server &server ) : sockserver( socks ), server( server ) {
	} // Acceptor::Acceptor
}; // Acceptor

_Task Server {
	uSocketServer &sockserver;
	Acceptor *terminate;
	int acceptorCnt;
	bool timeout;
  public:
	Server( uSocketServer &socks ) : sockserver( socks ), acceptorCnt( 1 ), timeout( false ) {
	} // Server::Server

	void connection() {
	} // Server::connection

	void complete( Acceptor *terminate, bool timeout ) {
		Server::terminate = terminate;
		Server::timeout = timeout;
	} // Server::complete
  private:
	void main() {
		new Acceptor( sockserver, *this );				// create initial acceptor
		for ( ;; ) {
			_Accept( connection ) {
				new Acceptor( sockserver, *this );		// create new acceptor after a connection
				acceptorCnt += 1;
			} or _Accept( complete ) {					// acceptor has completed with client
				delete terminate;						// delete must appear here or deadlock
				acceptorCnt -= 1;
		  if ( acceptorCnt == 0 ) break;				// if no outstanding connections, stop
				if ( timeout ) {
					new Acceptor( sockserver, *this );	// create new acceptor after a timeout
					acceptorCnt += 1;
				} // if
			} // _Accept
		} // for
	} // Server::main
}; // Server

void Acceptor::main() {
	try {
		uDuration timeout( 20, 0 );						// timeout for accept
		uSocketAccept acceptor( sockserver, &timeout );	// accept a connection from a client
		char buf[BufferSize];
		int len;

		server.connection();							// tell server about client connection
		for ( ;; ) {
			len = acceptor.read( buf, sizeof(buf) );	// read byte from client
			// osacquire( cerr ) << "Server::acceptor read len:" << len << endl;
		  if ( len == 0 ) abort( "server %d : EOF ecountered without EOD", getpid() );
			acceptor.write( buf, len );					// write byte back to client
			// The EOD character can be piggy-backed onto the end of the message.
		  if ( buf[len - 1] == EOD ) break;				// end of data ?
		} // for
		len = acceptor.read( buf, sizeof(buf) );		// read EOT from client
		if ( len != 1 && buf[0] != EOT ) {
			abort( "server %d : failed to read EOT", getpid() );
		} // if
		server.complete( this, false );					// terminate
	} catch( uSocketAccept::OpenTimeout & ) {
		server.complete( this, true );					// terminate
	} // try
} // Acceptor::main

int main( int argc, char *argv[] ) {
	switch ( argc ) {
	  case 2:
		break;
	  default:
		cerr << "Usage: " << argv[0] << " socket-name" << endl;
		exit( EXIT_FAILURE );
	} // switch

	uSocketServer sockserver( argv[1] );				// create and bind a server socket
	{
		Server s( sockserver );							// execute until acceptor times out
	}
	unlink( argv[1] );									// remove socket file
} // uMain

// Local Variables: //
// compile-command: "u++-work ServerUNIXSTREAM.cc -o Server" //
// End: //
