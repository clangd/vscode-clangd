//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Aaron Moss and Peter A. Buhr 2014
// 
// uCobegin.h -- 
// 
// Author           : Aaron Moss and Peter A. Buhr
// Created On       : Sat Dec 27 18:31:33 2014
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Sep 11 23:21:11 2023
// Update Count     : 67
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


#if __cplusplus > 201103L

#include <functional>
#include <memory>


// COBEGIN

#define COBEGIN uCobegin( {
#define COEND } );
#define BEGIN [&]( unsigned int uLid __attribute__(( unused )) ) {
#define END } ,

void uCobegin( std::initializer_list< std::function< void ( unsigned int ) >> funcs );

// COFOR

#define COFOR( lidname, low, high, body ) uCofor( low, high, [&]( decltype(high) lidname ){ body } );

template<typename Low, typename High>					// allow bounds to have different types (needed for constants)
void uCofor( Low low, High high, std::function<void ( decltype(high) )> f ) {
	_Task Runner {
		typedef std::function<void ( High )> Func;		// function type
		const Low low;									// work subrange
		const High high;
		Func f;											// function to run for each lid

		void main() {
			for ( High i = low; i < high; i += 1 ) f( i );
		} // Runner::main
	  public:
		Runner( Low low, High high, Func f ) : low( low ), high( high ), f( f ) {}
	}; // Runner

	static_assert(std::is_integral<Low>::value, "Integral type required for COFOR initialization.");
	static_assert(std::is_integral<High>::value, "Integral type required for COFOR comparison.");

	const long int range = high - low;					// number of iterations
  if ( range <= 0 ) return;								// no work ? skip otherwise zero divide
	const unsigned int nprocs = uThisCluster().getProcessors();	// parallelism
  if ( nprocs == 0 ) return;							// no processors ? skip otherwise zero divide
	const unsigned int threads = (decltype(nprocs))range < nprocs ? range : nprocs; // (min) may not need all processors
	uNoCtor< Runner > * runners = new uNoCtor< Runner >[threads]; // do not use up task stack
	// distribute extras among threads by increasing stride by 1
	unsigned int stride = range / threads + 1, extras = range % threads;
	// chunk the iterations across the user-threads
	// range 0-23, 3 threads: 23/3=7 stride, 23%3=2 extras => ranges across threads:, 0-8 (8), 8-16 (8), 16-23 (7)
	// range 0-12, 3 threads: 12/3=4 stride, 12%3=0 extras => ranges across threads:, 0-4 (4), 4-8 (4), 8-12 (4)
	unsigned int i = 0;
	High s = low;
	for ( i = 0; i < extras; i += 1, s += stride ) {	// create threads with subranges
		runners[i].ctor( s, s + stride, f );
	} // for
	stride -= 1;										// remove extras from stride
	for ( ; i < threads; i += 1, s += stride ) {		// create threads with subranges
		runners[i].ctor( s, s + stride, f );
	} // for
	delete [] runners;
} // uCofor

// START/WAIT

template<typename T, typename... Args>
_Task uWaitRunner {
	T rVal;
	std::function<T(Args...)> f;
	std::tuple<Args...> args;

	template<std::size_t... I>
		void call( std::index_sequence<I...> ) {
		rVal = f(std::get<I>(args)...);
	} // uWaitRunner::call

	void main() {
		call( std::index_sequence_for<Args...>{} );
	} // uWaitRunner::main
  public:
	uWaitRunner( std::function<T(Args...)> f, Args&&... args ) : f( f ), args( std::forward_as_tuple( args... ) ) {}
	T join() { return rVal; }
}; // uWaitRunner

template<typename... Args>
_Task uWaitRunner<void, Args...> {
	std::function<void (Args...)> f;
	std::tuple<Args...> args;

	template<std::size_t... I>
		void call( std::index_sequence<I...> ) {
		f( std::get<I>(args)... );
	} // uWaitRunner::call

	void main() {
		call( std::index_sequence_for<Args...>{} );
	} // uWaitRunner::main
  public:
	uWaitRunner( std::function<void (Args...)> f, Args&&... args ) : f( f ), args( std::forward_as_tuple( args... ) ) {}
	void join() {}
}; // uWaitRunner

template<typename F, typename... Args>
auto START( F f, Args&&... args ) -> std::unique_ptr<uWaitRunner<decltype( f( args... ) ), Args...>> {
	return std::make_unique<uWaitRunner<decltype( f(args...) ), Args...>>( f, std::forward<Args>(args)... );
} // START

#define WAIT( handle ) handle->join()

#else
#error requires C++14 (-std=c++1y)
#endif


// Local Variables: //
// compile-command: "make install" //
// End: //
