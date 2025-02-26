//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Ashif S. Harji 2000
//
// uStaticPIQ.h --
//
// Author           : Ashif S. Harji
// Created On       : Fri Jan 14 17:53:22 2000
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr  9 17:10:35 2022
// Update Count     : 39
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


// TEMPORARY: where should this be defined??
#define __U_MAX_NUMBER_PRIORITIES__ 32


class uStaticPIQ : public uBasePIQ {
	int objects[ __U_MAX_NUMBER_PRIORITIES__ ];
	uSpinLock lock;
	unsigned int mask;

	static int compare( int k1, int k2 );
  public:
	uStaticPIQ();
	virtual bool empty() const;
	//virtual int head();
	virtual int getHighestPriority();
	virtual void add( int priority );
	virtual int drop();
	virtual void remove( int priority );
}; // PIHeap


// Local Variables: //
// compile-command: "make install" //
// End: //
