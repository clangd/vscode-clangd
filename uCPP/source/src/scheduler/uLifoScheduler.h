//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Jiongxiong Chen and Ashif S. Harji 2003
//
// uLifoScheduler.h --
//
// Author           : Jiongxiong Chen and Ashif S. Harji
// Created On       : Fri Feb 14 14:26:49 2003
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr  9 17:06:10 2022
// Update Count     : 148
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


#include <uC++.h>

class uLifoScheduler : public uBaseSchedule<uBaseTaskDL> {
	uBaseTaskSeq list;					// list of tasks awaiting execution
  public:
	bool empty() const;
	void add( uBaseTaskDL *node );
	uBaseTaskDL *drop();
	bool checkPriority( uBaseTaskDL &owner, uBaseTaskDL &calling );
	void resetPriority( uBaseTaskDL &owner, uBaseTaskDL &calling );
	void addInitialize( uBaseTaskSeq &taskList );
	void removeInitialize( uBaseTaskSeq &taskList );
	void rescheduleTask( uBaseTaskDL *taskNode, uBaseTaskSeq &taskList );
}; // uLifoScheduler


// Local Variables: //
// compile-command: "make install" //
// End: //
