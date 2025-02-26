// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2020
// 
// ActorPhoneNo.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sat May  1 09:18:35 2021
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Oct  7 08:07:39 2023
// Update Count     : 181
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

#include <iostream>
#include <fstream>
using namespace std;
#include <ctype.h>										// prototype: isdigit

_CorActor PhoneNo {
  public:
	enum Status { CONT, MATCH, ERROR };
	struct DigitMsg : public uActor::SenderMsg { Status status; char ch; };
  private:
	DigitMsg * msg;
	char ch;											// character passed by cocaller
  public:
	Allocation receive( Message & msg ) {
		Case( DigitMsg, msg ) {
			PhoneNo::msg = msg_d;						// coroutine communication
			ch = msg_d->ch;
			resume();
			*msg_d->sender() | *msg_d;
		} else Case( StopMsg, msg ) return Finished;
		return Nodelete;
	} // PhoneNo::receive
  private:
	_Exception Err {};									// last character => string not in the language

	void error() {
		msg->status = ERROR;
		_Throw Err();
	} // PhoneNo::error

	void must( char test ) {
		if ( ch != test ) error();						// must have
		suspend();
	} // PhoneNo::must

	void getNdigits( int N ) {
		for ( int i = 0; i < N; i += 1 ) {				// must have 3 digits
			if ( ! isdigit( ch ) ) error();
			suspend();									// get next character
		} // for
	} // PhoneNo::getNdigits

	bool getArea() {
		if ( ch != '(' ) return false;					// area code ?
		suspend();
		getNdigits( 3 );
		must( ')' );									// must have a ')'
		return true;
	} // PhoneNo::getNdigits
  
	void main() {										// only handles one phone number
		for ( ;; ) {
			msg->status = CONT;
			try {
				if ( ch == '+' ) {
					suspend();							// get digit
					must( '1' );						// must have a '1'
					if ( ! getArea() ) error();
				} else getArea();
				getNdigits( 3 );
				must( '-' );							// must have a '-'
				getNdigits( 4 );

				if ( ch != '\n' ) error();
				msg->status = MATCH;
			} catch ( Err & ) {
			} // try
			suspend();
		} // for
	} // PhoneNo::main
}; // PhoneNo

_Actor Generator {
	PhoneNo phone;
	PhoneNo::DigitMsg digitMsg;
	char ch;
	fstream infile{ "ActorPhoneNo.data" };

	void preStart() {
		infile >> ch;
		if ( ! infile.fail() ) {
			digitMsg.ch = ch;
			phone | digitMsg;							// kick start generator
		} else {
			phone | uActor::stopMsg;
			*this | uActor::stopMsg;
		} // if			
	} // Generator::preStart

	Allocation receive( Message & msg ) {
		Case( PhoneNo::DigitMsg, msg ) {
			switch ( msg_d->status ) {
			  case PhoneNo::MATCH:
				cout << " phone number" << endl;
				break;
			  case PhoneNo::ERROR:
				cout << " ! phone number ";
				if ( ch != '\n' ) {
					cout << " extras ";
					for ( ;; ) {
						infile >> ch;
						cout << ch;
						if ( ch == '\n' ) break;
					} // for
				} else cout << ch;
				break;
			  case PhoneNo::CONT:
				break;
			} // switch
			infile >> ch;
			if ( infile.fail() ) {
				phone | uActor::stopMsg;
				return Finished;
			} // if
			cout << ch;
			digitMsg.ch = ch;
			phone | digitMsg;
		} else Case( StopMsg, msg ) return Finished;
		return Nodelete;
	} // Fib::receive
  public:
	Generator() {
		infile >> noskipws;
	} // Generator::Generator
}; // Generator

int main( int argc, char * argv[] ) {
	try {
		switch ( argc ) {
		  case 2:
		  case 1:										// use defaults
			break;
		  default:
			throw 1;
		} // switch
	} catch( ... ) {
		cerr << "Usage: " << argv[0] << " [ numbers (> 0) ]" << endl;
		exit( EXIT_FAILURE );
	} // try

	uActor::start();									// start actor system
	Generator phoneNoCheck;
	uActor::stop();										// wait for all actors to terminate
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "u++-work -g -O2 -multi ActorPhoneNo.cc" //
// End: //
