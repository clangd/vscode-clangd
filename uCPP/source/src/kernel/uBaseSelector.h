//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Jingge Fu 2015
//
// uBaseSelector.h --
//
// Author           : Jingge Fu
// Created On       : Sat Jul 14 07:25:52 2007
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Dec 31 09:04:17 2024
// Update Count     : 464
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

#include "../collection/uSequence.h"
//#include <iostream>
//using namespace std;
//static const char *StateName[] = { "NonAvail", "Avil" };
//static const char *TypeName[] = { "And", "Or" };


namespace UPP {
	// The complexity of this algorithm stems from the need to support all possible executable statements in the
	// statement after a _Select clause, e.g., lexical accesses and direct control transfers. Lambda expressions cannot
	// handle gotos because labels have routine scope. As a result, the evaluation of the _Select expression must keep
	// returning to the root of the expression tree so the statement of a select clause can be executed, and then the
	// tree must be traversed again to determine when the expression is complete. To mitigate having to restart the tree
	// walk, nodes in the tree that have triggered execution of a select clause are pruned so the tree walk is shortened
	// for each selected clause.

	// The evaluation tree is composed of unary and binary nodes, and futures. A unary node represents a future or
	// binary node, with an optional action from the _Select clause. A unary node represents a binary node for this
	// case:
	//
	//    _Select( f1 || f2 ) S;
	//
	// The binary node represents the logical operation (|| or &&) relating unary or future nodes.
	//
	//  U(S)  U(S)      B(no S)
	//   |     |        /     \.
	//   F     B     U | B   U | B
	//
	// Tree nodes are built and chained together on the stack of the thread executing the _Select statement, so there is
	// no dynamic allocation.

	// State represents the availability of a node; when the root is available, the _Select ends.  When a leaf node has
	// a false _When condition, it returns GuardFalse.  It is up to the parent node to interpret GuardFalse.  Note there
	// are only three possibilities.  If the parent node is an "and" binary node, GuardFalse is interpreted as Avail,
	// i.e., that branch has an available future, so the other branch still has to become available.  If the parent node
	// is an "or" binary node, GuardFalse is interpreted as NonAvail, i.e., that branch has an unavailable future, so
	// the other branch still has to become available. GuardFalse on both branches means that branch is turned off, and
	// is interpreted as Avail.
	enum State { NonAvail, Avail, GuardFalse };

	struct BaseFutureDL : public uSeqable {				// interface class containing blocking sem.
		virtual void signal() = 0;
		virtual ~BaseFutureDL() {}
	};  // BaseFutureDL


	struct SelectorClient {								// wrapper around the sem
		uSemaphore sem;									// selection client waits if no future available
		SelectorClient() : sem( 0 ) {};
	};  // SelectorClient


	// remove unnecessary warnings for SelectorDL::client
#if __GNUC__ >= 7										// valid GNU compiler diagnostic ?
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif // __GNUC__ >= 7

	// Each UnarySelector owns a SelectorDL, it is used for registering. 
	struct SelectorDL : public BaseFutureDL {
		SelectorClient * client;						// client data for server, set in registerSelf
		virtual ~SelectorDL() {}
		virtual void signal() { client->sem.V(); }		// SelectorDL::signal
	}; // SelectorDL

#if __GNUC__ >= 7										// valid GNU compiler diagnostic ?
#pragma GCC diagnostic pop
#endif // __GNUC__ >= 7

	struct Condition {									// represent And and Or condition
		enum Type { And, Or };

		static State sat( State & left, State & right, Type type ) {
			if ( left == GuardFalse && right == GuardFalse ) { // branch disabled ?
				//cerr << "sat left/right GuardFalse" << endl;
				left = right = Avail;
			} else {
				if ( left == GuardFalse ) {				// process left branch ?
					//cerr << "sat left GuardFalse" << endl;
					left = type == And ? Avail : NonAvail;
				} // if

				if ( right == GuardFalse ) {			// process right branch ?
					//cerr << "sat right GuardFalse" << endl;
					right = type == And ? Avail : NonAvail;
				} // if
			} // if

			bool result;
			if ( type == And ) {						// simple condition
				//osacquire( cerr ) << "sat And left " << StateName[left] << " right " << StateName[right] << endl;
				result = left == Avail && right == Avail;
			} else {
				//osacquire( cerr ) << "sat Or left " << StateName[left] << " right " << StateName[right] << endl;
				result = left == Avail || right == Avail;
			} // if

			//osacquire( cerr ) << "sat " << result << endl;
			return result ? Avail : NonAvail;
		} // Condition::sat
	}; // Condition


	// UnarySelector is a leaf containing a future. 
	template <typename Future>
	class UnarySelector {
		int myAction;									// action #
		Future & future;								// future
		UPP::SelectorDL bench;							// bench it registers with
		const bool WhenCondition;						// _When clause present
		bool remove;									// need to remove bench from future
	  public:
		UnarySelector( Future & future ) : myAction( 0 ), future( future ), WhenCondition( true ), remove( false ) {};
		UnarySelector( Future & future, int action ) : myAction( action ), future( future ), WhenCondition( true ), remove( false ) {};
		UnarySelector( bool guard, Future & future, int action ) : myAction( action ), future( future ), WhenCondition( guard ), remove( false ) {};

		~UnarySelector() {
			assert( ! remove );							// should have been removed when select available
			assert( ! bench.listed() );
		} // UnarySelector::~UnarySelector

		void registerSelf( UPP::SelectorClient * selectState ) { // register with future
			bench.client = selectState;
			remove = ! future.addSelect( &bench );		// not added if future available => do not remove
		} // UnarySelector::registerSelf

		State nextAction( int & action ) {				// give out action
			//osacquire( cerr ) << "nextAction uni" << " future " << &future << " action " << action << " myAction " << myAction << endl;
		  if ( ! WhenCondition ) return GuardFalse;	// _Select clause cancelled

			if ( myAction != 0 ) {						// _Select clause has action statement ?
				//osacquire( cerr ) << "nextAction uni1" << endl;
				if ( future.available() ) {				// future available ?
					action = myAction;					// set output parameter with my action
					myAction = 0;						// mark my action as executed
					removeFuture();						// remove select from future
					//osacquire( cerr ) << "nextAction uni2 Avail action " << action << endl;
					return Avail;
				} else {								// wait for future to become available
					//osacquire( cerr ) << "nextAction uni3 NonAvail action " << action << endl;
					return NonAvail;
				} // if
			} else {									// action already executed, done
				//osacquire( cerr ) << "nextAction uni4 Avail action " << action << endl;
				return Avail;
			} // if
		} // UnarySelector:nextAction

		void removeFuture() {
			if ( remove ) {								// _Select client chained onto available future?
				//osacquire( cerr ) << "nextAction uni" << " remove future " << &future << " bench " << &bench << endl;
				future.removeSelect( &bench );			// remove node from future
				remove = false;
			} // if
		} // UnarySelector:removeFuture

		void setAction( int action ) {					// set action for child expression
			//osacquire( cerr ) << "setAction uni " << action << endl;
			myAction = action;
		} // UnarySelector:setAction
	}; // UnarySelector


	// BinarySelector represents conditions and, or
	template <typename Left, typename Right>
	class BinarySelector {
		Left left;										// left child
		Right right;									// right child
		State leftStatus;								// left child status
		State rightStatus;								// right child status
		Condition::Type type;							// And or Or condition
		const bool WhenCondition;						// _When clause present
	  public:
		BinarySelector( const Left & left, const Right & right, Condition::Type type )
			: left( left ), right( right ), leftStatus( NonAvail ), rightStatus( NonAvail ), type( type ), WhenCondition( true ) {}

		BinarySelector( bool guard, const Left & left, const Right & right, Condition::Type type )
			: left( left ), right( right ), leftStatus( NonAvail ), rightStatus( NonAvail ), type( type ), WhenCondition( guard ) {}

		void registerSelf( UPP::SelectorClient * selectState ) {
			left.registerSelf( selectState );
			right.registerSelf( selectState );
		} // BinarySelector::registerSelf

		State nextAction( int & action ) {
			//osacquire( cerr ) << "nextAction bin left " << &left << " right " << &right << " type " << TypeName[type] << " action " << action << endl;
			if ( ! WhenCondition ) return GuardFalse;	// _Select clause cancelled

			if ( leftStatus != Avail ) leftStatus = left.nextAction( action );
			//osacquire( cerr ) << "nextAction bin0 " << StateName[leftStatus] << " type " << TypeName[type] << " action " << action << endl;

			// Left branch may not be available but its lower branch may be available and has an action, so the action
			// must be executed at the top level.
			if ( action != 0 ) {
				if ( leftStatus == Avail && type == Condition::Or ) rightStatus = Avail; // short circuit right side
				//osacquire( cerr ) << "nextAction bin1 left " << StateName[leftStatus] << " right " << StateName[rightStatus] << " type " << TypeName[type] << " action " << action << endl;
				return Condition::sat( leftStatus, rightStatus, type );
			} // if

			if ( rightStatus != Avail ) rightStatus = right.nextAction( action );
			//osacquire( cerr ) << "nextAction bin2 left " << StateName[leftStatus] << " right " << StateName[rightStatus] << " type " << TypeName[type] << " action " << action << endl;
			return Condition::sat( leftStatus, rightStatus, type );
		} // BinarySelector::nextAction

		void removeFuture() {							// recursively unchain select from futures
			left.removeFuture();
			right.removeFuture();
		} // BinarySelector::removeFuture

		void setAction( int action ) {					// recursively set action for child expressions
			// Trick: left branch of && is marked without action to force evaluation of right branch since there is only
			// one action for both.
			left.setAction( type == Condition::And ? 0 : action );
			right.setAction( action );
		} // BinarySelector::setAction

		Condition::Type getType() const { return type; }
	}; // BinarySelector


	// Specialization for future expression, e.g., _Select( f1 || f2 )
	template < typename Left, typename Right >
	class UnarySelector< BinarySelector< Left, Right > > {
		BinarySelector< Left, Right > binary;
		const bool WhenCondition;						// _When clause present
	  public:
		UnarySelector( const BinarySelector<Left, Right> & binary, int action ) : binary( binary ), WhenCondition( true ) {
			UnarySelector::binary.setAction( action );	// recursively send action to all child expressions
		} // UnarySelector::UnarySelector

		UnarySelector( bool guard, const BinarySelector<Left, Right> & binary, int action ) : binary( binary ), WhenCondition( guard ) {
			UnarySelector::binary.setAction( action );	// recursively send action to all child expressions
		} // UnarySelector::UnarySelector

		void registerSelf( UPP::SelectorClient * selectState ) {
			binary.registerSelf( selectState );
		} // UnarySelector::registerSelf

		State nextAction( int & action ) {
			//osacquire( cerr ) << "nextAction uni<>" << " binary " << &binary << " action " << action << endl;
			if ( ! WhenCondition ) return GuardFalse;	// _Select clause cancelled

			State s = binary.nextAction( action );
			//osacquire( cerr ) << "nextAction uni<> 1 " << StateName[s] << " " << TypeName[binary.getType()] << " action " << action << endl;
			return s;
		} // UnarySelector::nextAction

		void removeFuture() {							// recursively unchain select from futures
			binary.removeFuture();
		} // UnarySelector:removeFuture

		void setAction( int /* action */ )  {
			assert( false );							// should not be called
			return;
		} // UnarySelector::setAction
	}; // UnarySelector


	// Executor holds tree root, and evaluates tree and gets next action.
	template<typename Root>
	class Executor {
		Root & root;
		uTime timeout;
		bool hasTimeout, hasElse;
		State isFinish;
		UPP::SelectorClient selectState;
	  public:
		Executor( Root & root ) : root( root ), hasTimeout( false ), hasElse( false ), isFinish( NonAvail ) {
			root.registerSelf( & selectState );
		} // Executor::Executor

		Executor( Root & root, bool elseGuard ) : root( root ), hasTimeout( false ), hasElse( elseGuard ), isFinish( NonAvail ) {
			root.registerSelf( &selectState );
		} // Executor::Executor

		Executor( Root & root, uTime timeout ) : root( root ), timeout( timeout ), hasTimeout( true ), hasElse( false ), isFinish( NonAvail ) {
			root.registerSelf( &selectState );
		} // Executor::Executor

		Executor( Root & root, uDuration timeout ) : root( root ), timeout( uClock::currTime() + timeout ), hasTimeout( true ), hasElse( false ), isFinish( NonAvail ) {
			root.registerSelf( &selectState );
		} // Executor::Executor

		Executor( Root & root, bool timeoutGuard, uTime timeout ) : root( root ), timeout( timeout ), hasTimeout( timeoutGuard ), hasElse( false ), isFinish( NonAvail ) {
			root.registerSelf( &selectState );
		} // Executor::Executor

		Executor( Root & root, bool timeoutGuard, uDuration timeout ) : root( root ), timeout( uClock::currTime() + timeout ), hasTimeout( timeoutGuard ), hasElse( false ), isFinish( NonAvail ) {
			root.registerSelf( &selectState );
		} // Executor::Executor

		Executor( Root & root, bool timeoutGuard, uTime timeout, bool elseGuard )
			: root( root ), timeout( timeout ), hasTimeout( timeoutGuard ), hasElse( elseGuard ), isFinish( NonAvail ) {
			root.registerSelf( &selectState );
		} // Executor::Executor

		Executor( Root & root, bool timeoutGuard, uDuration timeout, bool elseGuard )
			: root( root ), timeout( uClock::currTime() + timeout ), hasTimeout( timeoutGuard ), hasElse( elseGuard ), isFinish( NonAvail ) {
			root.registerSelf( &selectState );
		} // Executor::Executor

		// Get the next action, if the whole expression is true, return 0.
		// If it has _Else, return 1;
		// If it has _TimeOut, return 2;
		// Otherwise, block until a future becomes available
		int nextAction() {
			int action;

			// must perform last action before stopping => minimum 2 calls with action
			if ( isFinish == Avail ) {
				//osacquire( cerr ) << "nextAction top finish" << endl;
				return 0;
			} // if

			for ( ;; ) {
				action = 0;				// reset
				//osacquire( cerr ) << "nextAction top " << StateName[isFinish] << " action " << action << endl;
				isFinish = root.nextAction( action );

				if ( action != 0 ) {
					//osacquire( cerr ) << "nextAction top action1 " << action << " " << StateName[isFinish] << endl;
					if ( isFinish == Avail ) {			// expression resolved ?
						root.removeFuture();			// walk tree removing select from futures to allow reset
					} // if
					return action;						// perform action in select
				} // if

				if ( hasElse ) {
					//osacquire( cerr ) << "nextAction top hasElse" << endl;
					root.removeFuture();				// walk tree removing select from futures to allow reset
					return 1;							// terminate select
				} // if

				if ( hasTimeout ) {
					//osacquire( cerr ) << "nextAction top hasTimeout" << endl;
					if ( ! selectState.sem.P( timeout ) ) { // timeout expired (false) ?
						root.removeFuture();			// walk tree removing select from futures to allow reset
						return 2;						// terminate select
					} // if 
				} else {
					if ( isFinish == Avail || isFinish == GuardFalse ) {
						//osacquire( cerr ) << "nextAction top action2 " << action << " " << StateName[isFinish] << endl;
						root.removeFuture();			// walk tree removing select from futures to allow reset
						return 0;						// terminate select with no action
					} // if

					//osacquire( cerr ) << "nextAction top block" << endl;
					selectState.sem.P();				// wait for future to become available
				} // if
			} // for
		} // Executor::nextAction
	}; // Executor
} // UPP


// Local Variables: //
// compile-command: "make install" //
// End: //
