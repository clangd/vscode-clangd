//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// AlarmClock.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Wed Oct 23 16:10:01 1991
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Nov 12 13:32:47 2020
// Update Count     : 104
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

class Node : public uSeqable {
  public:
	Node( const Node & ) = delete;						// no copy
	Node( Node && ) = delete;
	Node & operator=( const Node & ) = delete;			// no assignment
	Node & operator=( Node && ) = delete;

	int ticks;
	uCondition block;
	Node( int t ) { ticks = t; };
};

class OrderedList : public uSequence<Node> {
	OrderedList( const OrderedList & );					// no copy
	OrderedList( OrderedList && );
	OrderedList &operator=( const OrderedList & );		// no assignment
  public:
	OrderedList() {}
	void insert( Node *np ) {
		Node *lp;
		for ( lp = head();								// insert in ascending order by time
			 lp != 0 && lp->ticks < np->ticks;
			 lp = succ( lp ) );
		uSequence<Node>::insertBef( np, lp );
	} // OrderedList::insert
}; // OrderedList

_Task Clock;											// forward declaration

_Monitor Alarm {
	friend _Task Clock;									// so Clock can call private Mutex member
	int ticks;											// current time
	OrderedList TimeReq;								// ordered list of time requests

	_Mutex void tick() {
		Node *client;

		ticks += 1;
		for ( uSeqIter<Node> iter(TimeReq); iter >> client && client->ticks <= ticks; ) {
			osacquire( cout ) << "ticks = " << ticks << ", signalling!" << endl;
			TimeReq.remove( client );
			client->block.signal();
		} // for
	}; // Alarm::tick
  public:
	Alarm() {
		ticks = 0;
	} // Alarm::Alarm

	_Nomutex int time() {
		return ticks;
	} // Alarm::time

	void sleep_until( int t ) {
		Node time( t );

		TimeReq.insert( &time );
		time.block.wait();
	} // Alarm::sleep_until

	void sleep_for( int n ) {
		Node time( ticks + n );

		TimeReq.insert( &time );
		time.block.wait();
	} // Alarm::sleep_for
}; // Alarm

_Task Clock {
	Alarm &alarm;

	void main() {
		for ( ;; ) {
			_Accept( ~Clock ) {						// destructor called ?
				break;
			} else {									// don't block
				yield( 100 );							// should be a fixed period of time
				alarm.tick();							// advance the clock
			} // _Accept
		} // for
	} // Clock::main
  public:
	Clock(Alarm &alarm) : alarm( alarm ) {
	} // Clock::Clock
}; // Clock

_Task SampleUser {
	Alarm &alarm;

	void main() {
		int ticks;
		
		ticks = rand() % 17;      
		osacquire( cout ) << "Task " << this << " : Current Time = " << alarm.time() << ", Going to sleep for " << ticks << " ticks" << endl;
		alarm.sleep_for( ticks );
		osacquire( cout ) << "Task " << this << " : Now awake!  Current Time = " << alarm.time() << endl;
		
		ticks = rand() % 13 + alarm.time();      
		osacquire( cout ) << "Task " << this << " : Current Time = " << alarm.time() << ", Going to sleep until time " << ticks << endl;
		alarm.sleep_until( ticks );
		osacquire( cout ) << "Task " << this << " : Now awake!  Current Time = " << alarm.time() << endl;
		osacquire( cout ) << "Task " << this << " : All done, now dying" << endl;
	}; // SampleUser::main
  public:
	SampleUser( Alarm &alarm ) : alarm( alarm ) {
	}; // SampleUser::SampleUser
}; // SampleUser

int main() {
	const int NoOfUsers = 8;
	Alarm alarm;
	Clock clock( alarm );

	SampleUser *sample_users[NoOfUsers];
	for ( int i = 0; i < NoOfUsers; i += 1 ) {
		sample_users[i] = new SampleUser( alarm );
	} // for
	for ( int i = 0; i < NoOfUsers; i += 1 ) {
		delete sample_users[i];
	} // for

	osacquire( cout ) << "successful completion" << endl;
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "../../bin/u++ AlarmClock.cc" //
// End: //
