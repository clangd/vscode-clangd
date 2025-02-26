//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2002
// 
// Unix.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Mon Dec  2 22:46:18 2002
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 19 11:22:29 2022
// Update Count     : 26
// 

#define __U_KERNEL__
#include <uC++.h>
//#include <uDebug.h>


extern "C" unsigned int sleep( unsigned int sec ) {
	_Timeout( uDuration( sec, 0 ) );
	return 0;
} // sleep


extern "C" int usleep( unsigned int usec ) {
	_Timeout( uDuration( 0, usec ) );
	return 0;
} // usleep


extern "C" unsigned int alarm( unsigned int /* sec */ ) __THROW {
	abort( "alarm : not implemented" );
	return 0;
} // alarm


// Local Variables: //
// compile-command: "make install" //
// End: //
