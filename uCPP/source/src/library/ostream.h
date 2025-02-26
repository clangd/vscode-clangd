//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2005
// 
// ostream.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Thu Jul 28 14:42:29 2005
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 19 12:10:16 2022
// Update Count     : 48
// 
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


#include_next <ostream>


#pragma once


#include <uFilebuf.h>


namespace std {

//######################### new stream manipulators/routines #########################


	template<typename streamtype>
	class basic_acquire {
		typedef typename streamtype::char_type char_type;
		typedef typename streamtype::traits_type traits_type;

		streamtype & lockedStream;

		basic_acquire( const basic_acquire & ) = delete; // no copy
		basic_acquire( basic_acquire && ) = delete;
		basic_acquire & operator=( const basic_acquire & ) = delete; // no assignment
		basic_acquire & operator=( basic_acquire && ) = delete;
	  public:
		basic_acquire( streamtype & ios ) : lockedStream( ios ) {
#ifdef __U_DEBUG__
			filebuf * buf = dynamic_cast<filebuf *>( ios.rdbuf() );
			if ( buf == nullptr ) {
				abort( "Attempt to acquire mutual exclusion for a non-concurrent stream." );
			} // if
#else
			filebuf * buf = (filebuf *)ios.rdbuf();
#endif // __U_DEBUG__
			buf->ownerlock.acquire();
		} // basic_acquire

		~basic_acquire() {
			((filebuf *)lockedStream.rdbuf())->ownerlock.release();
		} // ~basic_acquire

		template< typename datatype >
		streamtype &operator<<( const datatype &data ) {
			return lockedStream << data;
		} // operator<<

		template< typename datatype >
		streamtype &operator>>( const datatype &data ) {
			return lockedStream >> data;
		} // operator>>

		template< typename datatype >
		streamtype &operator<<( datatype &data ) {
			return lockedStream << data;
		} // operator<<

		template< typename datatype >
		streamtype &operator>>( datatype &data ) {
			return lockedStream >> data;
		} // operator>>

		streamtype &operator<<( streamtype &(*__pf)(streamtype & ) ) {
			return lockedStream << __pf;
		} // operator<<

		streamtype &operator>>( streamtype &(*__pf)(streamtype & ) ) {
			return lockedStream >> __pf;
		} // operator>>

		streamtype &operator<<( basic_ios<char_type, traits_type> &(*__pf)(basic_ios<char_type, traits_type> & ) ) {
			return lockedStream << __pf;
		} // operator<<

		streamtype &operator>>( basic_ios<char_type, traits_type> &(*__pf)(basic_ios<char_type, traits_type> & ) ) {
			return lockedStream >> __pf;
		} // operator>>

		streamtype &operator<<( ios_base &(*__pf)(ios_base & ) ) {
			return lockedStream << __pf;
		} // operator<<

		streamtype &operator>>( ios_base &(*__pf)(ios_base & ) ) {
			return lockedStream >> __pf;
		} // operator>>
	}; // basic_acquire


	typedef basic_acquire<std::ostream> osacquire;
	typedef basic_acquire<std::istream> isacquire;
} // namespace std


// Local Variables: //
// compile-command: "make install" //
// End: //
