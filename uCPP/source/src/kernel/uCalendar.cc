//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Philipp E. Lim 1996
// 
// uCalendar.cc -- 
// 
// Author           : Philipp E. Lim
// Created On       : Thu Jan 11 08:23:17 1996
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Apr  3 09:42:39 2022
// Update Count     : 280
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
//#include <uDebug.h>

#include <ostream>


//######################### uDuration #########################


std::ostream & operator<<( std::ostream & os, const uDuration op ) {
	os << op.tn / TIMEGRAN << ".";
	os.width(9);										// nanoseconds
	char oc = os.fill( '0' );
	os << ( op.tn < 0 ? -op.tn : op.tn ) % TIMEGRAN;
	os.fill( oc );
	return os;
} // operator<<


//######################### uTime #########################


std::ostream & operator<<( std::ostream & os, const uTime op ) {
	os << op.tn / TIMEGRAN << ".";
	os.width(9);										// nanoseconds
	char oc = os.fill( '0' );
	os << op.tn % TIMEGRAN;
	os.fill( oc );
	return os;
} // operator<<


#ifdef __U_DEBUG__
#define uCreateFmt "Attempt to create uTime( year=%d, month=%d, day=%d, hour=%d, min=%d, sec=%d, nsec=%jd ), " \
	"which is not in the range 00:00:00 UTC, January 1, 1970 to 03:14:07 UTC, January 19, 2038, where month and day have 1 origin."
#endif // __U_DEBUG__


uTime::uTime( int year, int month, int day, int hour, int min, int sec, int64_t nsec ) {
	tm tm;

	// Values may be in any range (+/-) but result must be in the epoch.
	tm.tm_year = year - 1900;							// mktime uses 1900 as its starting point
	// Make month in range 1-12 to match with day.
	tm.tm_mon = month - 1;								// mktime uses range 0-11
	tm.tm_mday = day;									// mktime uses range 1-31
	tm.tm_hour = hour;
	tm.tm_min = min;
	tm.tm_sec = sec;
	tm.tm_isdst = -1;									// let mktime determine if alternate timezone is in effect
	time_t epochsec = mktime( &tm );
#ifdef __U_DEBUG__
	if ( epochsec <= (time_t)-1 ) {						// MUST BE LESS THAN OR EQUAL!
		abort( uCreateFmt, year, month, day, hour, min, sec, nsec );
	} // if
#endif // __U_DEBUG__
	tn = (int64_t)(epochsec) * TIMEGRAN + nsec;			// convert to nanoseconds
#ifdef __U_DEBUG__
	if ( tn > 2147483647LL * TIMEGRAN ) {				// between 00:00:00 UTC, January 1, 1970 and 03:14:07 UTC, January 19, 2038.
		abort( uCreateFmt, year, month, day, hour, min, sec, nsec );
	} // if
#endif // __U_DEBUG__
} // uTime::uCreateTime


// Local Variables: //
// compile-command: "make install" //
// End: //
