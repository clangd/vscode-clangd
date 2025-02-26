//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// DatingTrad.cc -- Exchanging Values Between Tasks using blocking signal
// 
// Author           : Peter A. Buhr
// Created On       : Thu Aug  2 11:47:55 1990
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Dec 18 23:50:27 2016
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
using std::cout;
using std::osacquire;
using std::endl;
#include <iomanip>
using std::setw;

_Monitor DatingService {
	int GirlPhoneNo, BoyPhoneNo;
	uCondition GirlWaiting, BoyWaiting;
  public:
	int Girl( int PhoneNo ) {
		if ( BoyWaiting.empty() ) {
			GirlWaiting.wait();
			GirlPhoneNo = PhoneNo;
		} else {
			GirlPhoneNo = PhoneNo;
			BoyWaiting.signalBlock();
		} // if
		return BoyPhoneNo;
	} // DatingService::Girl

	int Boy( int PhoneNo ) {
		if ( GirlWaiting.empty() ) {
			BoyWaiting.wait();
			BoyPhoneNo = PhoneNo;
		} else {
			BoyPhoneNo = PhoneNo;
			GirlWaiting.signalBlock();
		} // if
		return GirlPhoneNo;
	} // DatingService::Boy
}; // DatingService

_Task Girl {
	DatingService &TheExchange;

	void main() {
		yield( rand() % 100 );							// don't all start at the same time
		int PhoneNo = rand() % 10000000;
		int partner = TheExchange.Girl( PhoneNo );
		osacquire( cout ) << "Girl:" << setw(8) << &uThisTask() << " at " << setw(8) << PhoneNo
			<< " is dating Boy  at " << setw(8) << partner << endl;
	} // main
  public:
	Girl( DatingService &TheExchange ) : TheExchange( TheExchange ) {
	} // Girl
}; // Girl

_Task Boy {
	DatingService &TheExchange;

	void main() {
		yield( rand() % 100 );							// don't all start at the same time
		int PhoneNo = rand() % 10000000;
		int partner = TheExchange.Boy( PhoneNo );
		osacquire( cout ) << " Boy:" << setw(8) << &uThisTask() << " at " << setw(8) << PhoneNo
			<< " is dating Girl at " << setw(8) << partner << endl;
	} // main
  public:
	Boy( DatingService &TheExchange ) : TheExchange( TheExchange ) {
	} // Boy
}; // Boy


int main() {
	const int NoOfGirls = 20;
	const int NoOfBoys = 20;

	DatingService TheExchange;
	Girl *girls;
	Boy  *boys;

	girls = new Girl[NoOfGirls]( TheExchange );
	boys  = new Boy[NoOfBoys]( TheExchange );

	delete [] girls;
	delete [] boys;

	osacquire( cout ) << "successful completion" << endl;
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "u++ DatingTrad.cc" //
// End: //
