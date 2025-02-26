// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1999
// 
// ClientINETDGRAM.cc -- Client for INET/datagram socket test. Client reads from standard input, writes the data to the
//     server, reads the data from the server, and writes that data to standard output.
// 
// Author           : Peter A. Buhr
// Created On       : Thu Apr 29 16:04:50 1999
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Jan 20 08:02:00 2024
// Update Count     : 82
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

#include <uSemaphore.h>
#include <uSocket.h>
#include <iostream>
using std::cin;
using std::cout;
using std::cerr;
using std::osacquire;
using std::endl;

// minimum buffer size is 2, 1 character and string terminator, '\0'
enum { BufferSize = 65 };
const char EOD = '\377';
unsigned int rcnt = 0, wcnt = 0;

// Datagram sockets are lossy (i.e., drop packets). To prevent clients from flooding the server with packets, resulting
// in dropped packets, a semaphore is used to synchronize the reader and writer tasks so at most N writes occur before a
// read. As well, if the buffer size is increased substantially, it may be necessary to decrease N to ensure the server
// buffer does not fill.

enum { MaxWriteBeforeRead = 5 };
uSemaphore readSync( MaxWriteBeforeRead );

_Task Reader {
	uSocketClient &client;

	void main() {
		uDuration timeout( 20, 0 );						// timeout for read
		char buf[BufferSize];
		int len;

		try {
			for ( ;; ) {
				len = client.recvfrom( buf, sizeof(buf), 0, &timeout );
				readSync.V();
				// osacquire( cerr ) << "reader read len:" << len << endl;
			  if ( len == 0 ) abort( "client %d : EOF ecountered without EOD", getpid() );
				rcnt += len;
				// The EOD character can be piggy-backed onto the end of the message.
			  if ( buf[len - 1] == EOD ) {
					rcnt -= 1;							// do not count the EOD
					cout.write( buf, len - 1 );			// do not write the EOD
					break;
				} // exit
				cout.write( buf, len );
			} // for
		} catch( uSocketServer::ReadTimeout & ) {
			cout << "Warning: client timeout, possible cause lost data on datagram socket" << endl;
		} // try
	} // Reader::main
  public:
	Reader( uSocketClient &client ) : client ( client ) {
	} // Reader::Reader
}; // Reader

_Task Writer {
	uSocketClient &client;

	void main() {
		char buf[BufferSize];

		for ( ;; ) {
			cin.get( buf, sizeof(buf), '\0' );			// leave room for string terminator
			int len = strlen( buf );
			// osacquire( cerr ) << "writer read len:" << len << endl;
		  if ( buf[0] == '\0' ) break;
			wcnt += len;
			readSync.P();
			client.sendto( buf, len );
		} // for
		readSync.P();
		client.sendto( const_cast<char *>(&EOD), sizeof(EOD) );
	} // Writer::main
  public:
	Writer( uSocketClient &client ) : client( client ) {
	} // Writer::Writer
}; // Writer

int main( int argc, char *argv[] ) {
	switch ( argc ) {
	  case 2:
		break;
	  default:
		cerr << "Usage: " << argv[0] << " port-number" << endl;
		exit( EXIT_FAILURE );
	} // switch

	// remove tie due to race between cin flush and cout write by writer and reader tasks
	cin.tie( nullptr );
	uSocketClient client( atoi( argv[1] ), SOCK_DGRAM ); // connection to server
	{
		Reader rd( client );							// emit worker to read from server and write to output
		Writer wr( client );							// emit worker to read from input and write to server
	}
	if ( wcnt != rcnt ) {
		abort( "Error: client not all data transferred, wcnt:%d rcnt:%d", wcnt, rcnt );
	} // if
} // main

// Local Variables: //
// compile-command: "u++-work ClientINETDGRAM.cc -o Client" //
// End: //
