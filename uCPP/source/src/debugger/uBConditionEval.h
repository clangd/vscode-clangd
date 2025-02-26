//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Jun Shih 1995
// 
// uBConditionEval.h -- 
// 
// Author           : Jun Shih
// Created On       : Sat Nov 11 14:44:08 EST 1995
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr  5 08:04:41 2022
// Update Count     : 22
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

#ifndef _uBConditionEval_h_
#define _uBConditionEval_h_
#include <uDebuggerProtocolUnit.h>


class uBConditionEval : public uSeqable {
	friend _Task uLocalDebugger;
	struct BreakpointCondition bp_cond;
	ULThreadId ul_thread_id;
	
	int eval_int( int which );
	
	CodeAddress eval_address_local( int which );
	CodeAddress eval_address_static( int which );
	CodeAddress eval_address_const( int which );
	CodeAddress eval_address_register( int which );
  public:
	uBConditionEval( ULThreadId ul_thread_id );
	~uBConditionEval();
	void setId( ULThreadId Id );
	ULThreadId getId();
	void setFp ( long fp_val );
	void setSp ( long sp_val );
	long getFp();
	long getSp();
	BreakpointCondition::OperationType getOperator();
	BreakpointCondition& getBp_cond();
	int evaluate();
}; // uBConditionEval


// This class is necessary to avoid circular #include's
class uBConditionList {
	uSequence<uBConditionEval> bp_list;
  public:
	uBConditionList();
	~uBConditionList();
	uBConditionEval *search( ULThreadId ul_thread_id );
	void add( uBConditionEval* bc_eval );
	bool del( ULThreadId ul_thread_id );
}; // uBConditionList


#endif // _uBConditionEval_h_


// Local Variables: //
// compile-command: "make install" //
// End: //
