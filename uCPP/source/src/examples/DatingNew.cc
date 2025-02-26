//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// DatingNew.cc -- Exchanging Values Between Tasks
// 
// Author           : Peter A. Buhr
// Created On       : Thu Aug  2 11:45:46 1990
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Dec 18 23:50:16 2016
// Update Count     : 86
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
  public:
	DatingService() {
		GirlPhoneNo = BoyPhoneNo = -1;
	} // DatingService::DatingService

	int Girl( int PhoneNo );
	int Boy( int PhoneNo );
}; // DatingService

int DatingService::Girl( int PhoneNo ) {
	GirlPhoneNo = PhoneNo;
	if ( BoyPhoneNo == -1 ) _Accept( Boy );
	int temp = BoyPhoneNo;
	BoyPhoneNo = -1;
	return temp;
} // DatingService::Girl

int DatingService::Boy( int PhoneNo ) {
	BoyPhoneNo = PhoneNo;
	if ( GirlPhoneNo == -1 ) _Accept( Girl );
	int temp = GirlPhoneNo;
	GirlPhoneNo = -1;
	return temp;
} // DatingService::Boy

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
// compile-command: "u++ DatingNew.cc" //
// End: //
