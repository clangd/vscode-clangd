//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2005
// 
// fstream.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue Jul 19 13:24:20 2005
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Jun 11 09:12:39 2024
// Update Count     : 24
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


#include <iosfwd>
#include <istream>
#include <ostream>

#include <uFilebuf.h>


namespace std {
	//######################### ifstream #########################


	template< typename Ch, typename Tr >
	class basic_ifstream : public std::basic_istream<Ch, Tr> {
		basic_filebuf<Ch,Tr> sb;
	  public:
		basic_ifstream();
		basic_ifstream( const char *name, std::ios_base::openmode mode = std::ios_base::in );
		bool is_open();
		void open( const char *name, std::ios_base::openmode mode = std::ios_base::in );
		void close();
		basic_filebuf<Ch,Tr> *rdbuf() const;
	}; // basic_ifstream


	//######################### ofstream #########################


	template< typename Ch, typename Tr >
	class basic_ofstream : public std::basic_ostream<Ch, Tr> {
		basic_filebuf<Ch,Tr> sb;
	  public:
		basic_ofstream();
		basic_ofstream( const char *name, std::ios_base::openmode mode = std::ios_base::out );
		bool is_open();
		void open( const char *name, std::ios_base::openmode mode = std::ios_base::out );
		void close();
		basic_filebuf<Ch,Tr> *rdbuf();
	}; // basic_ofstream


	//######################### fstream #########################


	template< typename Ch, typename Tr >
	class basic_fstream : public std::basic_iostream<Ch, Tr> {
		basic_filebuf<Ch,Tr> sb;
	  public:
		basic_fstream();
		basic_fstream( const char *name, ios_base::openmode mode = ios_base::in | ios_base::out );
		bool is_open();
		void open( const char *name, ios_base::openmode mode = ios_base::in | ios_base::out );
		void close();
		basic_filebuf<Ch,Tr> *rdbuf();
	}; // basic_fstream


	//######################### ifstream #########################


	template< typename Ch, typename Tr >
	basic_ifstream<Ch, Tr>::basic_ifstream() : std::basic_istream<Ch, Tr>( &sb ) {
	} // ifstream<Ch, Tr>::ifstream

	template< typename Ch, typename Tr >
	basic_ifstream<Ch, Tr>::basic_ifstream( const char *name, std::ios_base::openmode mode ) : std::basic_istream<Ch, Tr>( &sb ) {
		rdbuf()->open( name, mode );
	} // ifstream<Ch, Tr>::ifstream

	template< typename Ch, typename Tr >
	bool basic_ifstream<Ch, Tr>::is_open() {
		return rdbuf()->is_open();
	} // ifstream<Ch, Tr>::is_open

	template< typename Ch, typename Tr >
	void basic_ifstream<Ch, Tr>::open( const char *name, std::ios_base::openmode mode ) {
		try {
			rdbuf()->open( name, mode );
		} catch( uFile::FileAccess::OpenFailure & ex ) {
			ex.setRaiseObject( this );					// change bound object from FileAccess to ifstream
			_Throw;										// rethrow
		} // try
	} // ifstream<Ch, Tr>::open

	template< typename Ch, typename Tr >
	void basic_ifstream<Ch, Tr>::close() {
		rdbuf()->close();
	} // ifstream<Ch, Tr>::close

	template< typename Ch, typename Tr >
	basic_filebuf<Ch,Tr> *basic_ifstream<Ch, Tr>::rdbuf() const {
		return (basic_filebuf<Ch,Tr> *)std::basic_istream<Ch, Tr>::rdbuf();
	} // ifstream<Ch, Tr>::rdbuf


	//######################### ofstream #########################


	template< typename Ch, typename Tr >
	basic_ofstream<Ch, Tr>::basic_ofstream() : std::basic_ostream<Ch, Tr>( &sb ) {
	} // ofstream<Ch, Tr>::ofstream

	template< typename Ch, typename Tr >
	basic_ofstream<Ch, Tr>::basic_ofstream( const char *name, std::ios_base::openmode mode ) : std::basic_ostream<Ch, Tr>( &sb ) {
		rdbuf()->open( name, mode );
	} // ofstream<Ch, Tr>::ofstream

	template< typename Ch, typename Tr >
	bool basic_ofstream<Ch, Tr>::is_open() {
		return rdbuf()->is_open();
	} // ofstream<Ch, Tr>::is_open

	template< typename Ch, typename Tr >
	void basic_ofstream<Ch, Tr>::open( const char *name, std::ios_base::openmode mode ) {
		try {
			rdbuf()->open( name, mode );
		} catch( uFile::FileAccess::OpenFailure & ex ) {
			ex.setRaiseObject( this );					// change bound object from FileAccess to ofstream
			_Throw;										// rethrow
		} // try
	} // ofstream<Ch, Tr>::open

	template< typename Ch, typename Tr >
	void basic_ofstream<Ch, Tr>::close() {
		rdbuf()->close();
	} // ofstream<Ch, Tr>::close

	template< typename Ch, typename Tr >
	basic_filebuf<Ch,Tr> *basic_ofstream<Ch, Tr>::rdbuf() {
		return (basic_filebuf<Ch,Tr> *)std::basic_ostream<Ch, Tr>::rdbuf();
	} // ofstream<Ch, Tr>::rdbuf


	//######################### fstream #########################


	template< typename Ch, typename Tr >
	basic_fstream<Ch, Tr>::basic_fstream() : std::basic_iostream<Ch, Tr>( &sb ) {
	} // fstream<Ch, Tr>::fstream

	template< typename Ch, typename Tr >
	basic_fstream<Ch, Tr>::basic_fstream( const char *name, std::ios_base::openmode mode ) : std::basic_iostream<Ch, Tr>( &sb ) {
		rdbuf()->open( name, mode );
	} // fstream<Ch, Tr>::fstream

	template< typename Ch, typename Tr >
	bool basic_fstream<Ch, Tr>::is_open() {
		return rdbuf()->is_open();
	} // fstream<Ch, Tr>::is_open

	template< typename Ch, typename Tr >
	void basic_fstream<Ch, Tr>::open( const char *name, std::ios_base::openmode mode ) {
		rdbuf()->open( name, mode );
	} // fstream<Ch, Tr>::fstream

	template< typename Ch, typename Tr >
	void basic_fstream<Ch, Tr>::close() {
		rdbuf()->close();
	} // fstream<Ch, Tr>::close

	template< typename Ch, typename Tr >
	basic_filebuf<Ch,Tr> *basic_fstream<Ch, Tr>::rdbuf() {
		return (basic_filebuf<Ch,Tr> *)std::basic_iostream<Ch, Tr>::rdbuf();
	} // fstream<Ch, Tr>::rdbuf
} // namespace std


// Local Variables: //
// compile-command: "make install" //
// End: //
