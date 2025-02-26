//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Thierry Delisle 2016
// 
// uActor.cc -- 
// 
// Author           : Peter A. Buhr and Thierry Delisle
// Created On       : Mon Nov 14 22:41:44 2016
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Mar 10 10:41:58 2023
// Update Count     : 116
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

#define __U_KERNEL__
#include <uC++.h>
#include <uActor.h>
#include <iostream>
using namespace std;

uExecutor * uActor::executor_ = nullptr;				// executor for all actors
bool uActor::executorp = false;							// executor passed to start member
uSemaphore uActor::wait_( 0 );							// wait for all actors to be destroyed
size_t uActor::actors_ = 0;								// number of actor objects in system
uActor::StartMsg uActor::startMsg;						// start actor
uActor::StopMsg uActor::stopMsg;						// terminate actor
uActor::UnhandledMsg uActor::unhandledMsg;				// tell error

void uActor::TraceMsg::print() {
	enum { PerLine = 8 };
	TraceMsg::Hop * route;

	cout << "message " << this
		 << " source:" << hops.head()
		 << " cursor (node:" << cursor << ", actor:" << (cursor ? cursor->actor : nullptr) << ')'
		 << endl << "  trace ";
	size_t cnt = 0;
	for ( uQueueIter<TraceMsg::Hop> iter(cursor); iter >> route; cnt += 1 ) { // print route
		if ( cnt != 0 && cnt % PerLine == 0 ) cout << endl << "\t";
		cout << route->actor;
		// HACK, know queue is circular
		if ( route->getnext() != route && (cnt + 1) % PerLine != 0 ) cout << " ";
	} // for
	cout << endl;
} // uActor::TraceMsg::print


// Local Variables: //
// compile-command: "make install" //
// End: //
