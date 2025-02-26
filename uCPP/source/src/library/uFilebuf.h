//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uFilebuf.h -- nonblocking stream buffer
// 
// Author           : Peter A. Buhr
// Created On       : Tue Mar 29 16:45:30 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Feb 13 14:38:35 2022
// Update Count     : 60
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


#include <iosfwd>										// basic_ios
#include <streambuf>
#include <cstdio>										// EOF

#include <uFile.h>

#define __U_BUFFER_SIZE__ 512

namespace std {
	template< typename char_t, typename traits >
	class basic_filebuf : public basic_streambuf<char_t, traits> {
		template<typename streamtype> friend class basic_acquire;
		template<typename Ch, typename Tr> friend basic_ios<Ch,Tr> &acquire( basic_ios<Ch,Tr> &os );
		template<typename Ch, typename Tr> friend basic_ios<Ch,Tr> &release( basic_ios<Ch,Tr> &os );
		template<typename Ch, typename Tr> friend uBaseTask *owner( basic_ios<Ch,Tr> &os );

		typedef typename basic_streambuf<char_t, traits>::char_type char_type;
		typedef typename basic_streambuf<char_t, traits>::traits_type traits_type;
		typedef typename basic_streambuf<char_t, traits>::int_type int_type;
		typedef typename basic_streambuf<char_t, traits>::pos_type pos_type;
		typedef typename basic_streambuf<char_t, traits>::off_type off_type;

		using basic_streambuf<char_t, traits>::pbase;
		using basic_streambuf<char_t, traits>::pptr;
		using basic_streambuf<char_t, traits>::epptr;
		using basic_streambuf<char_t, traits>::gptr;
		using basic_streambuf<char_t, traits>::egptr;
		using basic_streambuf<char_t, traits>::eback;
		using basic_streambuf<char_t, traits>::sgetc;
		using basic_streambuf<char_t, traits>::gbump;
		using basic_streambuf<char_t, traits>::setg;
		using basic_streambuf<char_t, traits>::setp;

		uFile *ufile;
		uFile::FileAccess *ufileacc;
		char_type buffer[__U_BUFFER_SIZE__];
		bool endOfFile;
		char_type *bufptr;
		streamsize bufsize;

		static int IosToUnixMode( ios_base::openmode mode );
		static int char_tarToUnixMode( const char *mode );
	  protected:
		uOwnerLock ownerlock;

		int_type underflow();
		int_type uflow();
		int_type overflow( int_type c = EOF );
		basic_filebuf *setbuf( char_type *buf, streamsize size );
		pos_type seekoff( off_type off, ios_base::seekdir dir, ios_base::openmode which = ios_base::in | ios_base::out );
		pos_type seekpos( pos_type sp, ios_base::openmode which = ios_base::in | ios_base::out );
		int sync();
		int pbackfail( int_type c = EOF );
		void imbue( const locale & );
	  public:
		basic_filebuf();
		basic_filebuf( int fd, int bufsize = __U_BUFFER_SIZE__ );
		basic_filebuf( int fd, char *buf, int bufsize );
		virtual ~basic_filebuf();

		bool is_open() const;
		basic_filebuf *open( const char *filename, ios_base::openmode mode );
		basic_filebuf *close();

		int fd();
	}; // basic_filebuf


	template< typename char_t, typename traits >
	int basic_filebuf<char_t, traits>::IosToUnixMode( ios_base::openmode mode ) {
		int m;

		if ( (mode & (ios_base::in | ios_base::out)) == (ios_base::in | ios_base::out) ) {
			m = O_RDWR;
		} else if ( mode & (ios_base::out | ios_base::app) ) {
			m = O_WRONLY;
		} else if ( mode & (int)ios_base::in ) {
			m = O_RDONLY;
		} else {
			abort( "IosToUnixMode( %d ) : Invalid I/O mode.", mode );
		} // if

		if ( (mode & ios_base::trunc) || mode == ios_base::out ) {
			m |= O_TRUNC;
		} // if
		if ( mode & ios_base::app ) {
			m |= O_APPEND;
		} // if
		if ( mode != ios_base::in ) {
			m |= O_CREAT;
		} // if

		return m;
	} // basic_filebuf<char_t, traits>::IosToUnixMode


	template< typename char_t, typename traits >
	int basic_filebuf<char_t, traits>::char_tarToUnixMode( const char *mode ) {
		int m;

		if ( strcmp( mode, "r" ) == 0 ) {				// convert
			m = O_RDONLY;
		} else if ( strcmp( mode, "w" ) == 0 ) {
			m = O_WRONLY | O_CREAT;
		} else if ( strcmp( mode, "a" ) == 0 ) {
			m = O_WRONLY | O_APPEND | O_CREAT;
		} else if ( strcmp( mode, "r+" ) == 0 ) {
			m = O_RDWR;
		} else if ( strcmp( mode, "w+" ) == 0 ) {
			m = O_RDWR | O_TRUNC | O_CREAT;
		} else if ( strcmp( mode, "a+" ) == 0 ) {
			m = O_RDWR | O_APPEND | O_CREAT;
		} else {
			abort( "char_tarToUnixMode( %s ) : Invalid I/O mode (r,w,r+,a+,w+).", mode );
		} // if

		return m;
	} // basic_filebuf<char_t, traits>::char_tarToUnixMode


	// 27.8.1.2 basic_filebuf constructors


	template< typename char_t, typename traits >
	basic_filebuf<char_t, traits>::basic_filebuf() {
		ufile = nullptr;
		ufileacc = nullptr;
		setbuf( buffer, __U_BUFFER_SIZE__ );			// reset all buffer pointers
		endOfFile = false;
	} // basic_filebuf<char_t, traits>::basic_filebuf


	// non-standard
	template< typename char_t, typename traits >
	basic_filebuf<char_t, traits>::basic_filebuf( int fd, int bufsize ) {
		ufile = new uFile( "/dev/tty" );
		ufileacc = new uFile::FileAccess( fd, *ufile );
		assert( bufsize <= __U_BUFFER_SIZE__ );
		setbuf( buffer, bufsize );						// reset all buffer pointers
		endOfFile = false;
	} // basic_filebuf<char_t, traits>::basic_filebuf


	// non-standard
	template< typename char_t, typename traits >
	basic_filebuf<char_t, traits>::basic_filebuf( int fd, char *buf, int bufsize ) {
		ufile = new uFile( "unknown" );
		ufileacc = new uFile::FileAccess( fd, *ufile );
		setbuf( buf, bufsize );							// reset all buffer pointers
		endOfFile = false;
	} // basic_filebuf<char_t, traits>::basic_filebuf


	template< typename char_t, typename traits >
	basic_filebuf<char_t, traits>::~basic_filebuf() {
		close();
	} // basic_filebuf<char_t, traits>::~basic_filebuf


	// 27.8.1.3 Member functions


	template< typename char_t, typename traits >
	bool basic_filebuf<char_t, traits>::is_open() const {
		return ufileacc != nullptr;
	} // basic_filebuf<char_t, traits>::is_open


	template< typename char_t, typename traits >
	basic_filebuf<char_t, traits> *basic_filebuf<char_t, traits>::open( const char *filename, ios_base::openmode mode ) {
		if ( is_open() ) return nullptr;
		ufile = new uFile( filename );
		ufileacc = new uFile::FileAccess( *ufile, IosToUnixMode( mode ) );
		if ( mode & ios_base::ate ) {					// seek to the end of the file
			if ( seekoff( 0, ios_base::end ) == pos_type( off_type( -1 ) ) ) {
				close();
				return nullptr;
			} // if
		} // if
		endOfFile = false;
		return this;
	} // basic_filebuf<char_t, traits>::open


	template< typename char_t, typename traits >
	basic_filebuf<char_t, traits> *basic_filebuf<char_t, traits>::close() {
		basic_filebuf<char_t, traits> *ret = this;
		if ( pptr() > pbase() ) {
			if ( overflow( traits::eof() ) == traits::eof() ) {
				ret = nullptr;
			} // if
		} // if
		if ( is_open() ) {
			try {
				delete ufileacc;
			} catch( uFile::FileAccess::CloseFailure & ) {
				ufileacc = 0;
				if ( ! std::__U_UNCAUGHT_EXCEPTION__() ) throw;
			} // try
			ufileacc = 0;
		} // if
		if ( ufile != 0 ) {
			delete ufile;
			ufile = 0;
		} // if
		return ret;
	} // basic_filebuf<char_t, traits>::close


	// non-standard
	template< typename char_t, typename traits >
	int basic_filebuf<char_t, traits>::fd() {
		return is_open() ? ufileacc->access.fd : EOF;
	} // basic_filebuf<char_t, traits>::fd


	// 27.8.1.4 Overridden virtual functions


	template< typename char_t, typename traits >
	typename basic_filebuf<char_t, traits>::int_type basic_filebuf<char_t, traits>::underflow() {
		if ( ! is_open() ) return traits::eof();		// file open ?

		int rbytes;
		int_type c = traits::eof();						// initialized to silence warning

		// There are times (getline) when underflow is called after EOF has already been returned. To prevent having to
		// enter multiple C-ds from stdin, additional reads must not be done.

		if ( ! endOfFile ) {
			rbytes = ufileacc->read( eback(), bufsize );
		} else {
			rbytes = 0;
		} // if
		setg( eback(), eback(), eback() + rbytes );		// reset input buffer pointers
		if ( rbytes == 0 ) {							// zero bytes read ?
			endOfFile = true;
			c = traits::eof();
		} else {
			c = sgetc();								// get the next character from the buffer
		} // if

		uDEBUGPRT( uDebugPrt( "c:%c = underflow(), rbytes:%d, eback:%p, gptr:%p, egptr:%p, pbase:%p, pptr:%p, epptr:%p, bufsize:%d\n", c, rbytes, eback(), gptr(), egptr(), pbase(), pptr(), epptr(), bufsize ); );

		return c;
	} // basic_filebuf<char_t, traits>::underflow


	template< typename char_t, typename traits >
	typename basic_filebuf<char_t, traits>::int_type basic_filebuf<char_t, traits>::uflow() {
		int_type c = underflow();
		if ( c != traits::eof() ) gbump( 1 );
		return c;
	} // basic_filebuf<char_t, traits>::uflow


	template< typename char_t, typename traits >
	typename basic_filebuf<char_t, traits>::int_type basic_filebuf<char_t, traits>::overflow( int_type c ) {
		if ( ! is_open() ) return traits::eof();		// file open ?

		streamsize len = pptr() - pbase(), wbytes;
		char_type *pos;

		uDEBUGPRT( uDebugPrt( "overflow( %c ), eback:%p, gptr:%p, egptr:%p, pbase:%p, pptr:%p, epptr:%p, len:%d\n", c, eback(), gptr(), egptr(), pbase(), pptr(), epptr(), pptr() - pbase() ); );

		if ( c != traits::eof() ) {						// character passed ?
			*pptr() = c;
			len += 1;
		} // if

		for ( pos = pbase(); len != 0; pos += wbytes, len -= wbytes ) {
			wbytes = ufileacc->write( pos, len );
		} // for

		setp( pbase(), epptr() );						// reset output buffer pointers

		return traits::not_eof( c );
	} // basic_filebuf<char_t, traits>::overflow


	template< typename char_t, typename traits >
	basic_filebuf<char_t, traits> *basic_filebuf<char_t, traits>::setbuf( char_type *buf, streamsize size ) {
		// It is necessary to have at least one character of storage to hold the last character read by underflow.
		// Having one character also simplifies the implementation of overflow.
		if ( buf == nullptr || size == 0 ) {
			bufptr = buffer;
			bufsize = 1;
		} else {
			bufptr = buf;
			bufsize = size;
		} // if
		setg( bufptr, bufptr, bufptr );					// reset input buffer pointers
		setp( bufptr, bufptr + bufsize - 1 );			// reset output buffer pointers
		uDEBUGPRT( uDebugPrt( "setbuf(), eback:%p, gptr:%p, egptr:%p, pbase:%p, pptr:%p, epptr:%p, len:%ld\n", eback(), gptr(), egptr(), pbase(), pptr(), epptr(), (long int)(pptr() - pbase()) ); );
		return this;
	} // basic_filebuf<char_t, traits>::setbuf


	template< typename char_t, typename traits >
	typename basic_filebuf<char_t, traits>::pos_type basic_filebuf<char_t, traits>::seekoff( off_type off, ios_base::seekdir dir, ios_base::openmode /* which */ ) {
	  if ( ! is_open() ) return traits::eof();			// file open ?
		off_t pos;

		sync();											// empty output buffer
		endOfFile = false;
		if ( dir == ios_base::beg ) {
			pos = ufileacc->lseek( off, SEEK_SET );
		} else if ( dir == ios_base::cur ) {
			// The following adjustment means that ios_base::cur seeks relative to the file position corresponding to
			// gptr(), not the actual current file position (as set by underflow).  The standard doesn't specify this
			// adjustment in so many words, but ios_base::cur seems much more useful if it is done. GNU does something
			// similar.
			off -= egptr() - gptr();
			pos = ufileacc->lseek( off, SEEK_CUR );
		} else {
			pos = ufileacc->lseek( off, SEEK_END );
		} // if

		uDEBUGPRT( uDebugPrt( "%lld = seekoff( %lld, %d, %d )\n", (long long)pos, (long long)off, dir, which ); );

		setg( bufptr, bufptr, bufptr );					// reset input buffer pointers
		setp( bufptr, bufptr + bufsize - 1 );			// reset output buffer pointers
		return pos_type( off_type( pos ) );
	} // basic_filebuf<char_t, traits>::seekoff


	template< typename char_t, typename traits >
	typename basic_filebuf<char_t, traits>::pos_type basic_filebuf<char_t, traits>::seekpos( pos_type sp, ios_base::openmode which ) {
		return seekoff( sp, ios_base::beg, which );
	} // basic_filebuf<char_t, traits>::seekpos


	template< typename char_t, typename traits >
	int basic_filebuf<char_t, traits>::sync() {
		int ret = 0;
		if ( pptr() > pbase() ) {
			ret = overflow();							// empty output buffer
		} // if
		return ret == traits::eof() ? -1 : 0;
	} // basic_filebuf<char_t, traits>::sync


	template< typename char_t, typename traits >
	void basic_filebuf<char_t, traits>::imbue( const locale & ) {
	} // basic_filebuf<char_t, traits>::imbue


/*
  Members mentioned in 27.8.1.4 but not overridden here:

  template< typename char_t, typename traits >
  int_type basic_filebuf<char_t, traits>::showmanyc();

  Returns a lower bound for the number of characters remaining to be read (i.e., not already in the buffer). Base class
  returns 0.

  template< typename char_t, typename traits >
  int_type basic_filebuf<char_t, traits>::pbackfail( int c );

  This is called on a non-trivial putback (e.g., before the beginning of the buffer, or a character different from the
  one that's in the file).  Whether any of this is supported appears to be a QOI issue.  The GNU version in some cases
  seeks backwards and invokes underflow; other times, it switches to a special 1-character buffer. Base class returns
  traits::eof(), which indicates that putback is impossible.
*/
} // namespace std


// Local Variables: //
// compile-command: "make install" //
// End: //
