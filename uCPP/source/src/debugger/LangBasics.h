//                              -*- Mode: C++ -*-
// 
// uC++ Version 7.0.0, Copyright (C) Martin Karsten 1995
// 
// LangBasics.h -- 
// 
// Author           : Martin Karsten
// Created On       : Sat May 13 14:55:53 1995
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Dec  2 23:15:23 2016
// Update Count     : 10
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

#ifndef _LangBasics_h_
#define _LangBasics_h_ 1

#ifdef __U_CPLUSPLUS__
#include <uC++.h>

typedef	uBaseTask*		ULThreadId;
typedef	uCluster*		ClusterId;
typedef uProcessor*		KernelThreadId;

_Task ULThread;
class Cluster;
_Monitor KernelThread;

struct ULThreadDescriptor {
	ULThreadId	id;
	ULThread*	pointer;
	int operator != ( const ULThreadDescriptor& x ) {
		return x.id != id;
	} // operator !=
	ULThreadDescriptor( ULThreadId id, ULThread* p ) : id(id), pointer(p) {}
	ULThreadDescriptor( ULThreadId id ) : id(id), pointer(nullptr) {}
	ULThreadDescriptor();
}; // struct ULThreadDescriptor

inline int operator != ( const ULThreadDescriptor& x, const ULThreadDescriptor& y ) {
	return x.id != y.id;
} // operator !=

struct ClusterDescriptor {
	ClusterId	id;
	Cluster*	pointer;
	int operator != ( const ClusterDescriptor& x ) {
		return x.id != id;
	} // operator !=
	ClusterDescriptor( ClusterId id, Cluster* p ) : id(id), pointer(p) {}
	ClusterDescriptor( ClusterId id ) : id(id), pointer(nullptr) {}
	ClusterDescriptor();
}; // struct ClusterDescriptor

inline int operator != ( const ClusterDescriptor& x, const ClusterDescriptor& y ) {
	return x.id != y.id;
} // operator !=

struct KernelThreadDescriptor {
	KernelThreadId	id;
	KernelThread*	pointer;
	int operator != ( const KernelThreadDescriptor& x ) {
		return x.id != id;
	} // operator !=
	KernelThreadDescriptor( KernelThreadId id, KernelThread* p ) : id(id), pointer(p) {}
	KernelThreadDescriptor( KernelThreadId id ) : id(id), pointer(nullptr) {}
	KernelThreadDescriptor();
}; // struct KernelThreadDescriptor

inline int operator != ( const KernelThreadDescriptor& x, const KernelThreadDescriptor& y ) {
	return x.id != y.id;
} // operator !=

#endif // __U_CPLUSPLUS__


#endif // _LangBasics_h_


// Local Variables: //
// tab-width: 4 //
// End: //
