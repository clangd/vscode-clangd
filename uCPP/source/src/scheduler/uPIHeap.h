//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Ashif S. Harji 2000
//
// uPIHeap.h --
//
// Author           : Ashif S. Harji
// Created On       : Fri Jan 14 17:53:22 2000
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr  9 17:07:15 2022
// Update Count     : 27
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


//#include <uDebug.h>

#include <uHeap.h>


#define __U_MAX_NUMBER_PRIORITIES__ 32


class uPIHeap : public uBasePIQ {
	struct uHeapBaseSeq {
	int index;
	int count;
	int queueNum;
	//uBaseTaskSeq queue;
	};

	uHeapBaseSeq objects[ __U_MAX_NUMBER_PRIORITIES__ ];
	uHeap<int, uHeapBaseSeq *, __U_MAX_NUMBER_PRIORITIES__> heap;
	uSpinLock lock;

	static int compare(int k1, int k2);
	static void exchange( uHeapable<int, uHeapBaseSeq *> &x, uHeapable<int, uHeapBaseSeq *> &y );
  public:
	uPIHeap();
	virtual bool empty() const;
	virtual int head() const;
	virtual int getHighestPriority();
	virtual void add( int priority, int queueNum );
	virtual int drop();
	virtual void remove( int priority, int queueNum );
}; // PIHeap


// Local Variables: //
// compile-command: "make install" //
// End: //
