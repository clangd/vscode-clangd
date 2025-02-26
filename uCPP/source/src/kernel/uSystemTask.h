//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1998
// 
// uSystemTask.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Jun 22 15:23:25 1998
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Apr  3 10:51:23 2022
// Update Count     : 25
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


_Task uSystemTask {
	friend _Task UPP::uPthread;							// access: pthreadDetachEnd

	uBaseTask *victim;									// communication

	// pthread

	_Mutex void pthreadDetachEnd( uBaseTask &victim );

	void main();
  public:
	uSystemTask();
	~uSystemTask();
	void reaper( uBaseTask &victim );
}; // uSystemTask


// Local Variables: //
// compile-command: "make install" //
// End: //
