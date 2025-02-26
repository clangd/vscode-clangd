//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Philipp E. Lim and Ashif S. Harji 1995, 1997
// 
// uAlarm.h -- 
// 
// Author           : Philipp E. Lim
// Created On       : Thu Dec 21 14:01:24 1995
// Last Modified By : Peter A. Buhr
// Last Modified On : Wed May  3 18:04:54 2023
// Update Count     : 191
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


class uSignalHandler;									// forward declarations

namespace UPP {
	_Coroutine uProcessorKernel;
	class uSemaphore;
	class uNBIO;
} // UPP


//######################### uEventNode #########################


class uEventNode : public uSeqable {
	friend class uEventList;							// access: everything
	friend class uEventListPop;							// access: everything
	friend class uBaseTask;								// access: everything
	friend class UPP::uSerial;							// access: everything
	friend class UPP::uNBIO;							// access: everything
	friend class UPP::uKernelBoot;						// access: uEventNode
	friend class uProcessor;							// access: everything
	friend class uCondLock;								// access: everything
	friend class UPP::uSemaphore;						// access: everything

	uTime alarm;										// time when alarm goes off
	uDuration period;									// if > 0 => period of alarm
	uBaseTask *task;									// task who created event
	uSignalHandler *sigHandler;							// action to perform when timer expires
	bool executeLocked;									// true => handler executed with uEventlock acquired

	void createEventNode( uBaseTask * task, uSignalHandler * sig, uTime alarm, uDuration period );
	uEventNode();
	uEventNode( uBaseTask & task, uSignalHandler & sig, uTime alarm, uDuration period = 0 );
	uEventNode( uSignalHandler &sig );

	void add( bool block = false );						// activate event
	void remove();										// deactivate event
}; // uEventNode


//######################### uEventList #########################


class uEventList {
	friend _Coroutine UPP::uProcessorKernel;			// access: userEventPresent
	friend class uSerial;								// access: addEvent, removeEvent
	friend class uCondLock;								// access: addEvent, removeEvent
	friend class UPP::uNBIO;							// access: addEvent, removeEvent, uNextAlarm
	friend class uBaseTask;								// access: addEvent
	template< typename T, bool runDtor > friend class uNoCtor; //  access: ~uEventList
	friend class UPP::uKernelBoot;						// access: uEventList
	friend class uProcessor;							// access: uEventList
	friend class uEventListPop;							// access: eventLock, eventlist
	friend class uEventNode;							// access: addEvent, removeEvent
  protected:
	uSpinLock eventLock;								// protect EventQueue
	uSequence<uEventNode> eventlist;					// event list

	virtual ~uEventList() {}

	void addEvent( uEventNode &newAlarm, bool block = false );
	void removeEvent( uEventNode &event );

	#if ! defined( __U_MULTI__ )
	bool userEventPresent();
	#endif // ! __U_MULTI__

	void setTimer( uDuration duration );				// call actual processor countdown timer
	void setTimer( uTime time );						// convert to duration
  public:
}; // uEventList


//######################### uEventListPop #########################


class uEventListPop {
	uEventList * events;								// list of handler events
	uClusterSeq wakeupList;								// list of clusters with wakeups
	uTime currTime;										// time iteration starts
	uSignalHandler * cxtSwHandler;						// ContextSwitch for system processor
	bool inKernel;										// processing events from the kernel
  public:
	uEventListPop();									// use with "over" (not defined, uses default)
	uEventListPop( uEventList & events, bool inKernel );
	~uEventListPop();
	void over( uEventList & events, bool inKernel );	// initialize iterator
	bool operator>>( uEventNode *&node );				// traverse each event on list
}; // uEventListPop


// Local Variables: //
// compile-command: "make install" //
// End: //
