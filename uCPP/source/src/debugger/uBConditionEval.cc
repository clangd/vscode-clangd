//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Jun Shih 1995
// 
// uBConditionEval.cc -- 
// 
// Author           : Jun Shih
// Created On       : Sat Nov 11 14:44:08 EST 1995
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 25 13:50:53 2023
// Update Count     : 53
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
#include <uBConditionEval.h>
//#include <uDebug.h>


/******************************* uBConditionEval *****************************/


int uBConditionEval::eval_int( int which ) {
	assert( bp_cond.var[which].vtype == BreakpointCondition::INT || bp_cond.var[which].vtype == BreakpointCondition::PTR);

	switch ( bp_cond.var[which].atype ) {
	  case BreakpointCondition::LOCAL:
		return *(int*) eval_address_local( which );
	
	  case BreakpointCondition::STATIC:
		return *(int*) eval_address_static( which );

	  case BreakpointCondition::REGISTER:
		return *(int*) eval_address_register( which );

	  case BreakpointCondition::CONST:
		return (size_t) eval_address_const( which );

	  default:
		assert( 0);
		return 0;
	}
} // uBConditionEval::eval_int


CodeAddress uBConditionEval::eval_address_local( int which ) {
	assert( bp_cond.var[which].atype == BreakpointCondition::LOCAL);
	//local address is the real fp + offset (usually negative)

	unsigned int prev_2fp;
#if defined( __i386__ )
	prev_2fp = *(unsigned long*) ( *(unsigned int*) ((int) bp_cond.fp ));
#elif defined( __x86_64__ )
	prev_2fp = 0;
#elif defined( __arm_64__ )
	abort();
#else
	#error uC++ internal error : unsupported architecture
#endif

	// local address is the real fp + offset (usually negative)
	unsigned long address = ((unsigned long) prev_2fp + (int)bp_cond.var[which].offset );

	uDEBUGPRT( uDebugPrt( " EVAL_ADDRESS_LOCAL: fp = 0x%x %d 0x%x 0x%x\n", bp_cond.fp, bp_cond.var[which].offset, (unsigned int)bp_cond.fp + (int)bp_cond.var[which].offset, address ); );

	if ( bp_cond.var[which].field_off ) {
		if ( bp_cond.var[which].field_off == -1) { // *p
			bp_cond.var[which].field_off = 0;
		} // if
		uDEBUGPRT( uDebugPrt( "address = 0x%x \n", *(long*)address ); );
		return (CodeAddress)(*(long*) address + bp_cond.var[which].field_off);
	} // if
	return (CodeAddress) address;
} // uBConditionEval::eval_address_local


CodeAddress uBConditionEval::eval_address_static( int which ) {
	assert( bp_cond.var[which].atype == BreakpointCondition::STATIC);

	if ( bp_cond.var[which].field_off == -1 ) { // *p
		uDEBUGPRT( uDebugPrt( "address = 0x%x %d\n", bp_cond.var[which].offset, *(long*)bp_cond.var[which].offset ); );
		return (CodeAddress) *(long*) bp_cond.var[which].offset;
	} // if

	unsigned long address = bp_cond.var[which].offset + bp_cond.var[which].field_off;

	if ( bp_cond.var[which].field_off ) {
		uDEBUGPRT( uDebugPrt( "address = 0x%x \n", *(long*)address ); );
		return (CodeAddress) address + bp_cond.var[which].field_off;
	} // if

	// offset is the address
	return (CodeAddress) address;
} // uBConditionEval::eval_address_static

CodeAddress uBConditionEval::eval_address_const( int which ) {
	assert( bp_cond.var[which].atype == BreakpointCondition::CONST);
	
	// offset is the value of the const
	return (CodeAddress) bp_cond.var[which].offset;
} // uBConditionEval::eval_address_const


CodeAddress uBConditionEval::eval_address_register( int which ) {
	assert( bp_cond.var[which].atype == BreakpointCondition::REGISTER);

	unsigned long address;

#if defined( __i386__ )
	unsigned long prev_fp = *(unsigned int*) ((int) bp_cond.fp);
	address =  (unsigned long) prev_fp - (sizeof(long)  *(bp_cond.var[which].offset+1));
	uDEBUGPRT(
		for ( int i = -10; i < 10; i += 1 ) {
			unsigned long addr = (unsigned long) prev_fp + (long) (i * sizeof(long));
			uDebugPrt( "i:%d addr:%p val:%d %p\n",i,addr, *(int*)addr,*(int*)addr );
		} // for
		);
#elif defined( __x86_64__ )
	address = 0;
#elif defined( __arm_64__ )
	abort();
#else
	#error uC++ internal error : unsupported architecture
#endif

	if ( bp_cond.var[which].field_off ) {
		if ( bp_cond.var[which].field_off == -1) { // *p
			bp_cond.var[which].field_off = 0;
		} // if
		uDEBUGPRT( uDebugPrt( "address = 0x%x \n", *(long*)address); );
		return (CodeAddress)(*(long*) address + bp_cond.var[which].field_off);
	} // if
	return (CodeAddress) address;
} // uBConditionEval::eval_address_register


uBConditionEval::uBConditionEval( ULThreadId ul_thread_id ) : ul_thread_id( ul_thread_id ) {
	bp_cond.Operator = BreakpointCondition::NOT_SET;
} // uBConditionEval::uBConditionEval

uBConditionEval::~uBConditionEval() {}


void uBConditionEval::setId( ULThreadId Id ) {
	ul_thread_id = Id;
} // uBConditionEval::setId


ULThreadId uBConditionEval::getId() {
	return ul_thread_id;
} // uBConditionEval::getId


void uBConditionEval::setFp( long fp_val ) {
	bp_cond.fp = fp_val;
} // uBConditionEval::setFp


void uBConditionEval::setSp( long sp_val ) {
	bp_cond.sp = sp_val;
} // uBConditionEval::setSp


long uBConditionEval::getFp() {
	return bp_cond.fp;
} // uBConditionEval::getFp


long uBConditionEval::getSp() {
	return bp_cond.sp;
} // uBConditionEval::getSp


BreakpointCondition::OperationType uBConditionEval::getOperator() {
	return bp_cond.Operator;
} // uBConditionEval::getOperator


BreakpointCondition &uBConditionEval::getBp_cond() {
	return bp_cond;
} // uBConditionEval::getBp_cond


int uBConditionEval::evaluate() {
	uDEBUGPRT( uDebugPrt( "bp_cond.var[0].vtype is %d and bp_cond.var[1].vtype is %d\n",bp_cond.var[0].vtype,bp_cond.var[1].vtype ); );
  if ( bp_cond.var[0].vtype == BreakpointCondition::INVALID || bp_cond.var[1].vtype == BreakpointCondition::INVALID) {
		return 0;					// can not find address of one of them
	} // if

	assert( ( bp_cond.var[0].vtype == BreakpointCondition::INT && bp_cond.var[1].vtype == BreakpointCondition::INT ) ||
			( bp_cond.var[0].vtype == BreakpointCondition::PTR && bp_cond.var[1].vtype == BreakpointCondition::PTR ) );

	switch (bp_cond.Operator ) {
	  case BreakpointCondition::EQUAL:
		return eval_int(0) == eval_int(1);
	  case BreakpointCondition::NOT_EQUAL:
		return eval_int(0) != eval_int(1);
	  case BreakpointCondition::GREATER_EQUAL:
		return eval_int(0) >= eval_int(1);
	  case BreakpointCondition::GREATER:
		return eval_int(0) >  eval_int(1);
	  case BreakpointCondition::LESS_EQUAL:
		return eval_int(0) <= eval_int(1);
	  case BreakpointCondition::LESS:
		return eval_int(0) <  eval_int(1);
	  default:
		assert(0);
		return 0;
	} // switch
} // uBConditionEval::evaluate


/****************************** uBConditionList ******************************/


uBConditionList::uBConditionList() {
} // uBConditionList::uBConditionList


uBConditionList::~uBConditionList() {
	uSeqIter<uBConditionEval> iter;
	uBConditionEval *bc_eval;

	for ( iter.over(bp_list ); iter >> bc_eval; ) {
		bp_list.remove(bc_eval );
		delete bc_eval;
	} // for
} // uBConditionList::uBConditionList


uBConditionEval *uBConditionList::search( ULThreadId ul_thread_id ) {
	uSeqIter<uBConditionEval> iter;
	uBConditionEval *bc_eval;

  if ( bp_list.empty() ) return nullptr;

	for ( iter.over(bp_list ); iter >> bc_eval; ) {
		if ( bc_eval->getId() == ul_thread_id ) return bc_eval;
	} // for
	return nullptr;
} //  uBConditionList::search


void uBConditionList::add( uBConditionEval *bc_eval ) {
	bp_list.add( bc_eval );
} // uBConditionList::add


bool uBConditionList::del( ULThreadId ul_thread_id ) {
	uBConditionEval *bc_eval = search(ul_thread_id );

	if ( ! bc_eval ) return false;
	bp_list.remove( bc_eval );
	delete bc_eval;
	return true;
} // uBConditionList::del


// Local Variables: //
// compile-command: "make install" //
// End: //
