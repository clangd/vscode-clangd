// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2007
// 
// ServerINETSTREAMSendfile.cc -- Server for INET/stream/sendfile socket test. Server accepts multiple connections from
//     clients. Each client then communicates with an acceptor.  The acceptor reads the data from the file sent by the
//     client and writes it back to the client using sendfile.
// 
// Author           : Peter A. Buhr
// Created On       : Mon Oct 15 15:46:35 2007
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Jan 20 08:04:21 2024
// Update Count     : 23
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
using std::cout;
using std::cerr;
using std::osacquire;
using std::endl;

enum { BufferSize = 8 * 1024 };

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
		char fileName[FILENAME_MAX];
		int len;

		server.connection();							// tell server about client connection

		len = acceptor.read( fileName, sizeof(fileName) ); // read filename from client
		fileName[len] = '\0';
		uFile::FileAccess input( fileName, O_RDONLY );	// open file

		struct stat info;
		input.status( info );							// compute file size
		int size = info.st_size;
		off_t off = 0;

		int wlen = acceptor.sendfile( input, &off, size ); // uC++ sends all data
		assert( wlen == size );

		server.complete( this, false );					// terminate
	} catch( uSocketAccept::OpenTimeout & ) {
		server.complete( this, true );					// terminate
	} // try
} // Acceptor::main

int main( int argc, char *argv[] ) {
	switch ( argc ) {
	  case 1:
		break;
	  default:
		cerr << "Usage: " << argv[0] << endl;
		exit( EXIT_FAILURE );
	} // switch

	short unsigned int port;
	uSocketServer sockserver( &port );					// create and bind a server socket to free port

	cout << port << endl;								// print out free port for clients
	{
		Server s( sockserver );							// execute until acceptor times out
	}
} // uMain

// Local Variables: //
// compile-command: "u++-work ServerINETSTREAMSendfile.cc -o Server" //
// End: //
