//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uContext.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Wed Feb 23 17:32:14 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Sep 30 15:39:15 2022
// Update Count     : 76
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


// Adding or removing from a task's context list requires mutual exclusion because a context switch can occur while
// executing this code, which results in the context routine traversing a possibly inconsistent list structure.


uContext::uContext( void *key ) : key( key ) {
	// Check the lists of additional contexts for this execution state for a context with the same unique key as the
	// context being added in this call.  If there is a similar context already active for this state, do not add this
	// context again.

	uBaseCoroutine &coroutine = uThisCoroutine();		// optimization
	uContext *context;
	for ( uSeqIter<uContext> iter( coroutine.additionalContexts_ ); iter >> context; ) {
		if ( context->key == key ) return;
	} // for

	// If no similar context is found, add this context to the list of contexts active for this state.

	uKernelModule::uKernelModuleData::disableInterrupts();
	coroutine.additionalContexts_.addTail( this );
	coroutine.extras_.is.usercxts = 1;
	uKernelModule::uKernelModuleData::enableInterrupts();
} // uContext::uContext

uContext::uContext() : key( this ) {
	uBaseCoroutine &coroutine = uThisCoroutine();		// optimization
	uKernelModule::uKernelModuleData::disableInterrupts();
	coroutine.additionalContexts_.addTail( this );
	coroutine.extras_.is.usercxts = 1;
	uKernelModule::uKernelModuleData::enableInterrupts();
} // uContext::uContext

uContext::~uContext() {
	uBaseCoroutine &coroutine = uThisCoroutine();		// optimization
	// If the usercxts is present on a list of contexts, remove it.
	if ( listed() ) {
		uKernelModule::uKernelModuleData::disableInterrupts();
		coroutine.additionalContexts_.remove( this );
		if ( coroutine.additionalContexts_.empty() ) {
			coroutine.extras_.is.usercxts = 0;
		} // if
		uKernelModule::uKernelModuleData::enableInterrupts();
	} // if
} // uContext::~uContext


void uContext::save() {
} // uContext::save

void uContext::restore() {
} // uContext::restore


// Local Variables: //
// compile-command: "make install" //
// End: //
