//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Richard C. Bilson 2007
// 
// uWaitQueue.h -- 
// 
// Author           : Richard C. Bilson
// Created On       : Mon Jul 16 08:17:06 2007
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 19 16:38:53 2022
// Update Count     : 21
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


#pragma once


template< typename Selectee >
class uWaitQueue_ESM {
	struct Helper {
		Selectee *s;
		Helper( Selectee *s ) : s( s ) {}
		bool available() const { return s->available(); }
		bool addAccept( UPP::BaseFutureDL *acceptState ) { return s->addAccept( acceptState ); }
		void removeAccept( UPP::BaseFutureDL *acceptState ) { return s->removeAccept( acceptState ); }
		bool equals( const Helper &other ) const { return s == other.s; }
	}; // Helper

	uWaitQueue_ISM< Helper > q;
  public:
	uWaitQueue_ESM( const uWaitQueue_ESM & ) = delete;	// no copy
	uWaitQueue_ESM( uWaitQueue_ESM && ) = delete;
	uWaitQueue_ESM & operator=( const uWaitQueue_ESM & ) = delete; // no assignment
	uWaitQueue_ESM & operator=( uWaitQueue_ESM && ) = delete;

	uWaitQueue_ESM() {}

	template< typename Iterator > uWaitQueue_ESM( Iterator begin, Iterator end ) {
		add( begin, end );
	} // uWaitQueue_ESM::uWaitQueue_ESM

	bool empty() const {
		return q.empty();
	} // uWaitQueue_ESM::empty

	void add( Selectee *n ) {
		q.add( Helper( n ) );
	} // uWaitQueue_ESM::add

	template< typename Iterator > void add( Iterator begin, Iterator end ) {
		for ( Iterator i = begin; i != end; ++i ) {
			add( &*i );
		} // for
	} // uWaitQueue_ESM::add

	void remove( Selectee *s ) {
		q.remove( Helper( s ) );
	} // uWaitQueue_ESM::remove

	Selectee *drop() {
		return empty() ? 0 : q.drop().s;
	} // uWaitQueue_ESM::drop
}; // uWaitQueue_ESM


template< typename Selectee >
class uWaitQueue_ISM {
	struct DL;

	struct DropClient {
		UPP::uSemaphore sem;							// selection client waits if no future available
		unsigned int tst;								// test-and-set for server race
		DL *winner;										// indicate winner of race

		DropClient() : sem( 0 ), tst( 0 ) {};
	}; // DropClient

	struct DL : public uSeqable {
		struct uBaseFutureDL : public UPP::BaseFutureDL {
			DropClient *client;							// client data for server
			DL *s;										// iterator corresponding to this DL

			uBaseFutureDL( DL *t ) : s( t ) {}

			virtual void signal() {
				if ( uTestSet( client->tst ) == 0 ) {	// returns 0 or non-zero
					client->winner = s;
					client->sem.V();					// client see changes because semaphore does memory barriers
				} // if
			} // signal
		}; // uBaseFutureDL

		uBaseFutureDL acceptState;
		Selectee selectee;

		DL( Selectee t ) : acceptState( this ), selectee( t ) {}
	}; // DL

	uSequence< DL > q;
  public:
	uWaitQueue_ISM( const uWaitQueue_ISM & ) = delete;	// no copy
	uWaitQueue_ISM( uWaitQueue_ISM && ) = delete;
	uWaitQueue_ISM & operator=( const uWaitQueue_ISM & ) = delete; // no assignment
	uWaitQueue_ISM & operator=( uWaitQueue_ISM && ) = delete;

	uWaitQueue_ISM() {}

	template< typename Iterator > uWaitQueue_ISM( Iterator begin, Iterator end ) {
		add( begin, end );
	} // uWaitQueue_ISM::uWaitQueue_ISM

	~uWaitQueue_ISM() {
		DL *t;
		for ( uSeqIter< DL > i( q ); i >> t; ) {
			delete t;
		} // for
	} // uWaitQueue_ISM::~uWaitQueue_ISM

	bool empty() const {
		return q.empty();
	} // uWaitQueue_ISM::empty

	void add( Selectee n ) {
		q.add( new DL( n ) );
	} // uWaitQueue_ISM::add

	template< typename Iterator > void add( Iterator begin, Iterator end ) {
		for ( Iterator i = begin; i != end; ++i ) {
			add( *i );
		} // for
	} // uWaitQueue_ISM::add

	void remove( Selectee n ) {
		DL *t = 0;
		for ( uSeqIter< DL > i( q ); i >> t; ) {
			if ( t->selectee.equals( n ) ) {
				q.remove( t );
				delete t;
			} // if
		} // for
	} // uWaitQueue_ISM::remove

	Selectee drop() {
		if ( q.empty() ) abort( "uWaitQueue_ISM: attempt to drop from an empty queue" );

		DropClient client;
		DL *t = 0;
		for ( uSeqIter< DL > i( q ); i >> t; ) {
			t->acceptState.client = &client;
			if ( t->selectee.addAccept( &t->acceptState ) ) {
				DL *s;
				for ( uSeqIter< DL > i( q ); i >> s && s != t; ) {
					s->selectee.removeAccept( &s->acceptState );
				} // for
				goto cleanup;
			} // if
		} // for

		client.sem.P();
		t = client.winner;
		DL *s;
		for ( uSeqIter< DL > i( q ); i >> s; ) {
			s->selectee.removeAccept( &s->acceptState );
		} // for

	  cleanup:
		Selectee selectee = t->selectee;
		q.remove( t );
		delete t;
		return selectee;
	} // uWaitQueue_ISM::drop

	// not implemented, since the "head" of the queue is not fixed i.e., if another item comes ready it may become the
	// new "head" use "drop" instead
	//T *head() const;
}; // uWaitQueue_ISM


// Local Variables: //
// compile-command: "make install" //
// End: //
