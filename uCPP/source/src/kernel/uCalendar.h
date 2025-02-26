//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Philipp E. Lim 1995
// 
// uCalendar.h -- 
// 
// Author           : Philipp E. Lim
// Created On       : Tue Dec 19 11:58:22 1995
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu May 20 22:26:36 2021
// Update Count     : 332
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


#include <ctime>										// timespec
#include <sys/time.h>									// itimerval


#define CLOCKGRAN 15000000L								// ALWAYS in nanoseconds, MUST BE less than 1 second
#define TIMEGRAN 1000000000L							// nanosecond granularity, except for timeval
#define GETTIMEOFDAY( tp ) gettimeofday( (tp), (struct timezone *)0 )

// fake a few things
#define		CLOCK_REALTIME		0						// real (clock on the wall) time


class uDuration;										// forward declaration
class uTime;											// forward declaration
class uClock;											// forward declaration


//######################### uDuration #########################


uDuration operator+( uDuration op );					// forward declaration
uDuration operator+( uDuration op1, uDuration op2 );	// forward declaration
uDuration operator-( uDuration op );					// forward declaration
uDuration operator-( uDuration op1, uDuration op2 );	// forward declaration
uDuration operator*( uDuration op1, long long int op2 ); // forward declaration
uDuration operator*( long long int op1, uDuration op2 ); // forward declaration
uDuration operator/( uDuration op1, long long int op2 ); // forward declaration
long long int operator/( uDuration op1, uDuration op2 ); // forward declaration
bool operator==( uDuration op1, uDuration op2 );		// forward declaration
bool operator!=( uDuration op1, uDuration op2 );		// forward declaration
bool operator>( uDuration op1, uDuration op2 );			// forward declaration
bool operator<( uDuration op1, uDuration op2 );			// forward declaration
bool operator>=( uDuration op1, uDuration op2 );		// forward declaration
bool operator<=( uDuration op1, uDuration op2 );		// forward declaration
std::ostream & operator<<( std::ostream & os, const uDuration op ); // forward declaration


//######################### uTime #########################


uTime operator+( uTime op1, uDuration op2 );			// forward declaration
uTime operator+( uDuration op1, uTime op2 );			// forward declaration
uDuration operator-( uTime op1, uTime op2 );			// forward declaration
uTime operator-( uTime op1, uDuration op2 );			// forward declaration
bool operator==( uTime op1, uTime op2 );				// forward declaration
bool operator!=( uTime op1, uTime op2 );				// forward declaration
bool operator>( uTime op1, uTime op2 );					// forward declaration
bool operator<( uTime op1, uTime op2 );					// forward declaration
bool operator>=( uTime op1, uTime op2 );				// forward declaration
bool operator<=( uTime op1, uTime op2 );				// forward declaration
std::ostream & operator<<( std::ostream & os, const uTime op ); // forward declaration


//######################### uDuration (cont) #########################


class uDuration {
	friend class uTime;
	friend class uClock;
	friend uDuration operator+( uDuration op1 );
	friend uDuration operator+( uDuration op1, uDuration op2 );
	friend uDuration operator-( uDuration op );
	friend uDuration operator-( uDuration op1, uDuration op2 );
	friend uDuration operator*( uDuration op1, long long int op2 );
	friend uDuration operator/( uDuration op1, long long int op2 );
	friend long long int operator/( uDuration op1, uDuration op2 );
	friend long long int operator%( uDuration op1, uDuration op2 );
	friend bool operator==( uDuration op1, uDuration op2 );
	friend bool operator!=( uDuration op1, uDuration op2 );
	friend bool operator>( uDuration op1, uDuration op2 );
	friend bool operator<( uDuration op1, uDuration op2 );
	friend bool operator>=( uDuration op1, uDuration op2 ); 
	friend bool operator<=( uDuration op1, uDuration op2 );
	friend uDuration abs( uDuration op );
	friend std::ostream & operator<<( std::ostream & os, const uDuration op );

	friend uDuration operator-( uTime op1, uTime op2 );
	friend uTime operator-( uTime op1, uDuration op2 );
	friend uTime operator+( uTime op1, uDuration op2 );

	int64_t tn;											// nanoseconds
  public:
	uDuration() {
		tn = 0;
	} // uDuration::uDuration

	uDuration( long int sec ) {
		tn = (long long int)sec * TIMEGRAN;
	} // uDuration::uDuration

	uDuration( long int sec, long int nsec ) {
		tn = (long long int)sec * TIMEGRAN + nsec;
	} // uDuration::uDuration

	uDuration( const timeval tv ) {
		tn = (long long int)tv.tv_sec * TIMEGRAN + tv.tv_usec * 1000;
	} // uDuration::uDuration

	uDuration( const timespec ts ) {
		tn = (long long int)ts.tv_sec * TIMEGRAN + ts.tv_nsec;
	} // uDuration::uDuration

	uDuration operator=( const timeval tv ) {
		tn = (long long int)tv.tv_sec * TIMEGRAN + tv.tv_usec * 1000;
		return *this;
	} // uDuration::operator=

	uDuration operator=( const timespec ts ) {
		tn = (long long int)ts.tv_sec * TIMEGRAN + ts.tv_nsec;
		return *this;
	} // uDuration::operator=

	operator timeval() const {
		timeval tv;
		tv.tv_sec = tn / TIMEGRAN;						// seconds
		tv.tv_usec = tn % TIMEGRAN / ( TIMEGRAN / 1000000L ); // microseconds
		return tv;
	} // uDuration::operator timeval

	operator timespec() const {
		timespec ts;
		ts.tv_sec = tn / TIMEGRAN;						// seconds
		ts.tv_nsec = tn % TIMEGRAN;						// nanoseconds
		return ts;
	} // uDuration::operator timespec

	long int seconds() const {
		return tn / TIMEGRAN;							// seconds
	} // uDuration::nanoseconds

	int64_t nanoseconds() const {
		return tn;
	} // uDuration::nanoseconds

	uDuration operator-=( uDuration op ) {
		*this = *this - op;
		return *this;
	} // uDuration::operator-=

	uDuration operator+=( uDuration op ) {
		*this = *this + op;
		return *this;
	} // uDuration::operator+=

	uDuration operator*=( int64_t op ) {
		*this = *this * op;
		return *this;
	} // uDuration::operator*=

	uDuration operator/=( int64_t op ) {
		*this = *this / op;
		return *this;
	} // uDuration::operator/=
}; // uDuration


inline uDuration operator+( uDuration op ) {			// unary
	uDuration ans;
	ans.tn = +op.tn;
	return ans;
} // operator+

inline uDuration operator+( uDuration op1, uDuration op2 ) { // binary
	uDuration ans;
	ans.tn = op1.tn + op2.tn;
	return ans;
} // operator+

inline uDuration operator-( uDuration op ) {			// unary
	uDuration ans;
	ans.tn = -op.tn;
	return ans;
} // operator-

inline uDuration operator-( uDuration op1, uDuration op2 ) { // binary
	uDuration ans;
	ans.tn = op1.tn - op2.tn;
	return ans;
} // operator-

inline uDuration operator*( uDuration op1, long long int op2 ) {
	uDuration ans;
	ans.tn = op1.tn * op2;
	return ans;
} // operator*

inline uDuration operator*( long long int op1, uDuration op2 ) {
	return op2 * op1;
} // operator*

inline uDuration operator/( uDuration op1, long long int op2 ) {
	uDuration ans;
	ans.tn = op1.tn / op2;
	return ans;
} // operator/

inline long long int operator/( uDuration op1, uDuration op2 ) {
	return op1.tn / op2.tn;
} // operator/

inline bool operator==( uDuration op1, uDuration op2 ) {
	return op1.tn == op2.tn;
} // operator==

inline bool operator!=( uDuration op1, uDuration op2 ) {
	return op1.tn != op2.tn;
} // operator!=

inline bool operator>( uDuration op1, uDuration op2 ) {
	return op1.tn > op2.tn;
} // operator>

inline bool operator<( uDuration op1, uDuration op2 ) {
	return op1.tn < op2.tn;
} // operator<

inline bool operator>=( uDuration op1, uDuration op2 ) { 
	return op1.tn >= op2.tn;
} // operator>=

inline bool operator<=( uDuration op1, uDuration op2 ) {
	return op1.tn <= op2.tn;
} // operator<=

inline long long int operator%( uDuration op1, uDuration op2 ) {
	return op1.tn % op2.tn;
} // operator%

inline uDuration abs( uDuration op1 ) {
	if ( op1.tn < 0 ) op1.tn = -op1.tn;
	return op1;
} // abs


//######################### uTime (cont) #########################


class uTime {
	friend class uDuration;
	friend class uClock;
	friend uTime operator+( uTime op1, uDuration op2 );
	friend uDuration operator-( uTime op1, uTime op2 );
	friend uTime operator-( uTime op1, uDuration op2 );
	friend bool operator==( uTime op1, uTime op2 );
	friend bool operator!=( uTime op1, uTime op2 );
	friend bool operator>( uTime op1, uTime op2 );
	friend bool operator<( uTime op1, uTime op2 );
	friend bool operator>=( uTime op1, uTime op2 ); 
	friend bool operator<=( uTime op1, uTime op2 );
	friend std::ostream & operator<<( std::ostream & os, const uTime op );

	uint64_t tn;										// nanoseconds since UNIX epoch
  public:
	uTime() {
		tn = 0;
	} // uTime::uTime

	// explicit => unambiguous with uDuration( long int sec )
	explicit uTime( int year, int month = 1, int day = 1, int hour = 0, int min = 0, int sec = 0, int64_t nsec = 0 );

	uTime( timeval tv ) {
		tn = (long long int)tv.tv_sec * TIMEGRAN + tv.tv_usec * 1000;
	} // uTime::uTime

	uTime( timespec ts ) {
		tn = (long long int)ts.tv_sec * TIMEGRAN + ts.tv_nsec;
	} // uTime::uTime

	uTime operator=( timeval tv ) {
		tn = (long long int)tv.tv_sec * TIMEGRAN + tv.tv_usec * 1000;
		return *this;
	} // uTime::operator=

	uTime operator=( timespec ts ) {
		tn = (long long int)ts.tv_sec * TIMEGRAN + ts.tv_nsec;
		return *this;
	} // uTime::operator=

	operator timeval() const {
		timeval tv;
		tv.tv_sec = tn / TIMEGRAN;						// seconds
		tv.tv_usec = tn % TIMEGRAN / ( TIMEGRAN / 1000000L ); // microseconds
		return tv;
	} // uTime::operator timeval

	operator timespec() const {
		timespec ts;
		ts.tv_sec = tn / TIMEGRAN;						// seconds
		ts.tv_nsec = tn % TIMEGRAN;						// nanoseconds
		return ts;
	} // uTime::operator timespec

	operator tm() const {
		tm tm;
		time_t sec = tn / TIMEGRAN;						// seconds
		localtime_r( &sec, &tm );
		return tm;
	} // uTime::operator tm

	void convert( int & year, int & month, int & day, int & hour, int & minutes, int & seconds, int64_t & nseconds ) const {
		tm tm = *this;
		year = tm.tm_year; month = tm.tm_mon; day = tm.tm_mday; hour = tm.tm_hour; minutes = tm.tm_min; seconds = tm. tm_sec;
		nseconds = tn % TIMEGRAN;
	} // uTime::convert

	uint64_t nanoseconds() const {
		return tn;
	} // uTime::nanoseconds

	uTime operator-=( uDuration op ) {
		*this = *this - op;
		return *this;
	} // uTime::operator-=

	uTime operator+=( uDuration op ) {
		*this = *this + op;
		return *this;
	} //  uTime::operator+=
}; // uTime


inline uTime operator+( uTime op1, uDuration op2 ) {
	uTime ans;
	ans.tn = op1.tn + op2.tn;
	return ans;
} // operator+

inline uTime operator+( uDuration op1, uTime op2 ) {
	return op2 + op1;
} // operator+

inline uDuration operator-( uTime op1, uTime op2 ) {
	uDuration ans;
	ans.tn = op1.tn - op2.tn;
	return ans;
} // operator-

inline uTime operator-( uTime op1, uDuration op2 ) {
	uTime ans;
	ans.tn = op1.tn - op2.tn;
	return ans;
} // operator-

inline bool operator==( uTime op1, uTime op2 ) {
	return op1.tn == op2.tn;
} // operator==

inline bool operator!=( uTime op1, uTime op2 ) {
	return op1.tn != op2.tn;
} // operator!=

inline bool operator>( uTime op1, uTime op2 ) {
	return op1.tn > op2.tn;
} // operator>

inline bool operator<( uTime op1, uTime op2 ) {
	return op1.tn < op2.tn;
} // operator<

inline bool operator>=( uTime op1, uTime op2 ) { 
	return op1.tn >= op2.tn;
} // operator>=

inline bool operator<=( uTime op1, uTime op2 ) {
	return op1.tn <= op2.tn;
} // operator<=


//######################### uClock #########################


class uClock {
	uDuration offset;									// for virtual clock: contains offset from real-time
  public:
	uClock() : offset( 0 ) {
	} // uClock::uClock

	void resetClock( uDuration adj ) {
		offset = adj + __timezone * TIMEGRAN;			// timezone (global) is (UTC - local time) in seconds
	} // uClock::resetClock

	uClock( uDuration adj ) {
		resetClock( adj );
	} // uClock::uClock

	static uDuration getResNsec() {
		timespec res;
		clock_getres( CLOCK_REALTIME, &res );
		return uDuration( res.tv_sec * TIMEGRAN, res.tv_nsec);
	} // uClock::getRes

	static uDuration getRes() {
		struct timespec res;
		clock_getres( CLOCK_REALTIME_COARSE, &res );
		return uDuration( res.tv_sec * TIMEGRAN, res.tv_nsec );
	} // uClock::getRes

	// cannot overload static getTime with member getTime
	static uTime currTime() {							// ##### REFERENCED IN TRANSLATOR #####
		timespec ts;
		clock_gettime( CLOCK_REALTIME, &ts );
		return uTime( ts );
	} // uClock::currTime

	uTime getTime() {
		return currTime() + offset;
	} // uClock::getTime

	static uTime getCPUTime() {
		timespec ts;
		clock_gettime( CLOCK_THREAD_CPUTIME_ID, &ts );
		return uTime( ts );
	} // uClock::getCPUTime
}; // uClock


// Local Variables: //
// compile-command: "make install" //
// End: //
