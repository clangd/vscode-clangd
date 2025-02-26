// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1999
// 
// ServerUNIXDGRAM.cc -- Server for UNIX/datagram socket test. Server reads data from multiple clients. The server reads
//     the data from the client and writes it back.
// 
// Author           : Peter A. Buhr
// Created On       : Fri Apr 30 16:36:18 1999
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Jan 20 08:04:29 2024
// Update Count     : 50
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

_Task Reader {
	uSocketServer &server;

	void main() {
		uDuration timeout( 20, 0 );						// timeout for read
		char buf[BufferSize];
		int len;
		//struct sockaddr_un to;
		//socklen_t tolen = sizeof( to );

		try {
			for ( ;; ) {
				len = server.recvfrom( buf, sizeof(buf), 0, &timeout );
				//len = server.recvfrom( buf, sizeof(buf), (sockaddr *)&to, &tolen, 0, &timeout );
				// osacquire( cerr ) << "reader read len:" << len << endl;
				if ( len == 0 ) abort( "server %d : EOF ecountered before timeout", getpid() );
				server.sendto( buf, len );				// write byte back to client
				//server.sendto( buf, len, (sockaddr *)&to, tolen ); // write byte back to client
			} // for
		} catch( uSocketServer::ReadTimeout & ) {
		} // try
	} // Reader::main
  public:
	Reader( uSocketServer &server ) : server( server ) {
	} // Reader::Reader
}; // Reader

int main( int argc, char *argv[] ) {
	switch ( argc ) {
	  case 2:
		break;
	  default:
		cerr << "Usage: " << argv[0] << " socket-name" << endl;
		exit( EXIT_FAILURE );
	} // switch

	uSocketServer server( argv[1], SOCK_DGRAM );		// create and bind a server socket
	{
		Reader rd( server );							// execute until reader times out
	}
	unlink( argv[1] );									// remove socket file
} // uMain

// Local Variables: //
// compile-command: "u++-work ServerUNIXDGRAM.cc -o Server" //
// End: //
