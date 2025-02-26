//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Philipp E. Lim 1996
//
// uDeadlineMonotonic.h --
//
// Author           : Philipp E. Lim and Ashif S. Harji
// Created On       : Fri Jul 19 23:20:17 1996
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr  9 17:01:04 2022
// Update Count     : 248
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
#include <uHeapQ.h>


class uDeadlineMonotonic1 : public uPriorityScheduleQSeq<uBaseTaskSeq, uBaseTaskDL> {
  public:
	void addInitialize( uSequence<uBaseTaskDL> &taskList );
	void removeInitialize( uSequence<uBaseTaskDL> & );
	void rescheduleTask( uBaseTaskDL *taskNode, uBaseTaskSeq &taskList );
}; // uDeadlineMonotonic1


// Local Variables: //
// compile-command: "make install" //
// End: //
