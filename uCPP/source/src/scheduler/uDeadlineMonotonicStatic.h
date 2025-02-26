//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Ashif S. Harji 2000
//
// uDeadlineMonotonic.h --
//
// Author           : Ashif S. Harji
// Created On       : Fri Oct 27 16:09:42 2000
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr  9 17:03:06 2022
// Update Count     : 158
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


#include <uRealTime.h>
#include <uStaticPriorityQ.h>


class uDeadlineMonotonicStatic : public uStaticPriorityScheduleSeq<uBaseTaskSeq, uBaseTaskDL> {
	int compare( uBaseTask &task1, uBaseTask &task2 );
  public:
	void addInitialize( uSequence<uBaseTaskDL> &taskList );
	void removeInitialize( uSequence<uBaseTaskDL> & );
	void rescheduleTask( uBaseTaskDL *taskNode, uBaseTaskSeq &taskList );
}; // uDeadlineMonotonicStatic


// Local Variables: //
// compile-command: "make install" //
// End: //
