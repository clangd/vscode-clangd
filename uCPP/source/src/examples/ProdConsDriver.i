//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// ProdConsDriver.i -- Producer/Consumer Driver for a bounded buffer
// 
// Author           : Peter A. Buhr
// Created On       : Sun Sep 15 18:19:38 1991
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Jan 22 21:50:50 2017
// Update Count     : 77
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

_Task producer {
	BoundedBuffer<int> &buf;

	void main() {
		const int NoOfItems = rand() % 20;
		int item;

		for ( int i = 1; i <= NoOfItems; i += 1 ) {		// produce a bunch of items
			yield( rand() % 20 );						// pretend to spend some time producing
			item = rand() % 100 + 1;					// produce a random number
			osacquire( cout ) << "Producer:" << this << ", value:" << item << endl;
			buf.insert( item );							// insert element into queue
		} // for
		osacquire( cout ) << "Producer " << this << " is finished!" << endl;
	} // producer::main
  public:
	producer( BoundedBuffer<int> &buf ) : buf( buf ) {
	} // producer::producer
}; // producer

_Task consumer {
	BoundedBuffer<int> &buf;

	void main() {
		int item;

		for ( ;; ) {									// consume until a negative element appears
			item = buf.remove();						// remove from front of queue
			osacquire( cout ) << "Consumer:" << this << ", value:" << item << endl;
		  if ( item == -1 ) break;
			yield( rand() % 20 );						// pretend to spend some time consuming
		} // for
		osacquire( cout ) << "Consumer " << this << " is finished!" << endl;
	} // consumer::main
  public:
	consumer( BoundedBuffer<int> &buf ) : buf( buf ) {
	} // consumer::consumer
}; // consumer

int main() {
	const int NoOfCons = 2, NoOfProds = 3;
	BoundedBuffer<int> buf;								// create a buffer monitor
	consumer *cons[NoOfCons];							// pointer to an array of consumers
	producer *prods[NoOfProds];							// pointer to an array of producers

	for ( int i = 0; i < NoOfCons; i += 1 ) {			// create consumers
	    cons[i] = new consumer( buf );
	} // for
	for ( int i = 0; i < NoOfProds; i += 1 ) {			// create producers
	    prods[i] = new producer( buf );
	} // for

	for ( int i = 0; i < NoOfProds; i += 1 ) {			// wait for producers to end
	    delete prods[i];
	} // for
	for ( int i = 0; i < NoOfCons; i += 1 ) {			// terminate each consumer
		buf.insert( -1 );
	} // for
	for ( int i = 0; i < NoOfCons; i += 1 ) {			// wait for consumers to end
	    delete cons[i];
	} // for

	cout << "successful completion" << endl;
} // main

// Local Variables: //
// tab-width: 4 //
// End: //
