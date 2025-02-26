// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2020
// 
// ActorDeviceDriver.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue May  4 21:20:09 2021
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 30 20:56:33 2024
// Update Count     : 93
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
#include <iomanip>
using namespace std;
#include <uActor.h>

#include <iostream>
#include <fstream>
using namespace std;

_CorActor Driver {
  public:
	enum Status { CONT, MSG, ESTX, ELNTH, ECRC };
	enum { MaxMsg = 64 };
	struct ByteMsg : public uActor::SenderMsg { Status status; unsigned char byte; unsigned char * msgtext; };
  private:
	ByteMsg * msg;
	unsigned char byte;									// byte passed by interrupt
  public:
	Allocation receive( Message & msg ) {
		Case( ByteMsg, msg ) {
			Driver::msg = msg_d;						// coroutine communication
			byte = msg_d->byte;
			resume();
			*msg_d->sender() | *msg_d;
		} else Case( StopMsg, msg ) return Finished;
		return Nodelete;
	} // Driver::receive
  private:
	void checkCRC( unsigned int sum ) {
		suspend();
		unsigned short int crc = byte << 8;				// sign extension over written
		suspend();
		// prevent sign extension for signed char
		msg->status = (crc | (unsigned char)byte) == sum ? MSG : ECRC;
	} // Driver::checkCRC

 	void main() {
		enum { STX = '\002', ESC = '\033', ETX = '\003' };
	  msg: for ( ;; ) {									// parse messages
			msg->status = CONT;
			unsigned int lnth = 0, sum = 0;
			while ( byte != STX ) {						// look for start of message
				cout << ' ' << (char)byte;
				suspend();
			} // while
			cout << " STX ";
		  eom:
			for ( ;; ) {								// scan message data
				suspend();
				switch ( byte ) {
				  case STX:								// protocol violation
					cout << " STX";
					msg->status = ESTX;
					suspend();
					continue msg;						// uC++ labelled continue
				  case ETX:								// end of message
					cout << " ETX";
					break eom;							// uC++ labelled break
				  case ESC:								// escape next byte
					cout << " ESC";
					suspend();
					break;
				} // switch	
				if ( lnth >= MaxMsg ) {					// full buffer ?
					msg->status = ELNTH;
					suspend();
					continue msg;
				} // if
				cout << (char)byte;
				msg->msgtext[lnth++] = byte;			// store message
				sum += byte;
			} // for
			checkCRC( sum );							// refactor CRC check
			msg->byte = lnth;							// cheat, add string terminator
			suspend();
		} // for
	} // Driver::main
}; // Driver

_Actor Generator {
	Driver driver;
	Driver::ByteMsg byteMsg;
	unsigned char byte;
	unsigned char msgtext[Driver::MaxMsg];
	fstream infile{ "ActorDeviceDriver.data" };

	void preStart() {
		byteMsg.msgtext = msgtext;
		infile >> byte;
		if ( ! infile.fail() ) {
			byteMsg.byte = byte;
		} else {
			byteMsg.byte = '\0';
		} // if			
		driver | byteMsg;								// kick start generator
	} // Generator::preStart

	Allocation receive( Message & msg ) {
		Case( Driver::ByteMsg, msg ) {
			switch ( msg_d->status ) {
			  case Driver::MSG:
				cout << " message \"";
				for ( unsigned int i = 0; i < msg_d->byte; i += 1 ) cout << msgtext[i];
				cout << "\"" << endl;
				break;
			  case Driver::ESTX:
				cout << " STX in message";
				goto common;
			  case Driver::ELNTH:
				cout << " Message too long";
				goto common;
			  case Driver::ECRC:
				cout << " CRC invalid";
			  common:
				byteMsg.status = Driver::CONT;
			  default: ;
			} // switch
			infile >> byte;
			if ( infile.fail() ) {
				driver | uActor::stopMsg;
				return Finished;
			} // if
			byteMsg.byte = byte;
			driver | byteMsg;
		} // Case
		return Nodelete;
	} // Fib::receive
  public:
	Generator() {
		infile >> noskipws;
	} // Generator::Generator
}; // Generator

int main() {
	uActor::start();									// start actor system
	Generator driver;
	uActor::stop();										// wait for all actors to terminate
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "u++-work -g -O2 -multi ActorDeviceDriver.cc" //
// End: //
