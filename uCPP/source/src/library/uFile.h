//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uFile.h -- Nonblocking UNIX I/O library
// 
// Author           : Peter A. Buhr
// Created On       : Tue Mar 29 16:38:54 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Oct  7 07:57:05 2023
// Update Count     : 213
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


#include <uIOcntl.h>


#include <fcntl.h>										// open, mode flags
#include <sys/stat.h>									// stat


//######################### uFileIO #########################


class uFileIO {											// monitor
  protected:
	uIOaccess &access;

	// These routines provide an indirection to _Throw because the exception object is large and placed on the stack if
	// it appears in situ.
	virtual void readFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) = 0;
	virtual void readTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) = 0;
	virtual void writeFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) = 0;
	virtual void writeTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) = 0;

	uFileIO( uIOaccess &acc ) : access( acc ) {
	} // uFileIO::uFileIO

	virtual ~uFileIO() {
	} // uFileIO::~uFileIO
  public:
	int read( char *buf, int len, uDuration *timeout = nullptr );
	int readv( const struct iovec *iov, int iovcnt, uDuration *timeout = nullptr );
	_Mutex int write( const char *buf, int len, uDuration *timeout = nullptr );
	int writev( const struct iovec *iov, int iovcnt, uDuration *timeout = nullptr );

	int fd() {
		return access.fd;
	} // uFileIO::fd
}; // uFileIO


//######################### uFile #########################


class uFile {
	friend class uFileWrapper;
	friend class FileAccess;

	char *name;
	int accessCnt;

	void access() {
		uFetchAdd( accessCnt, 1 );
	} // uFile::access

	void unaccess() {
		uFetchAdd( accessCnt, -1 );
	} // uFile::unaccess
  public:
	_Exception Failure : public uIOFailure {
		const uFile &f;
		char name[uEHMMaxName];
	  protected:
		Failure( const uFile &f, int errno_, const char *const msg );
	  public:
		const uFile &file() const;
		const char *getName() const;
		virtual void defaultTerminate() override;
	}; // uFile::Failure

	_Exception TerminateFailure : public Failure {
		const int accessCnt;
	  public:
		TerminateFailure( const uFile &f, int errno_, const int accessCnt, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uFile::TerminateFailure

	_Exception StatusFailure : public Failure {
		const struct stat &buf;
	  public:
		StatusFailure( const uFile &f, int errno_, const struct stat &buf, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uFile::StatusFailure

	class FileAccess : public uFileIO {					// monitor
		template< typename char_t, typename traits > friend class std::basic_filebuf; // access: constructor
		friend class uSocketIO;							// access: access

		uFile *file;
		const bool own;
		uIOaccess access;

		void createAccess( int flags, int mode );

		FileAccess( int fd, uFile &f ) : uFileIO( access ), file( &f ), own( false ) {
			access.fd = fd;
			access.poll.setStatus( uPoll::PollOnDemand );
			file->access();
		} // FileAccess::FileAccess
	  protected:
		void readFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op );
		void readTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op );
		void writeFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op );
		void writeTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op );
	  public:
		_Exception Failure : public uFile::Failure {
			const FileAccess &fa;
			int fd;
		  protected:
			Failure( const FileAccess &fa, int errno_, const char *const msg );
		  public:
			const FileAccess &fileAccess() const { return fa; }
			int fileDescriptor() const { return fd; }
			virtual void defaultTerminate() override;
		}; // FileAccess::Failure

		friend _Exception Failure;

		_Exception OpenFailure : public Failure {
			const int flags;
			const int mode;
		  public:
			OpenFailure( FileAccess &fa, int errno_, int flags, int mode, const char *const msg );
			virtual void defaultTerminate() override;
		}; // FileAccess::OpenFailure

		_Exception CloseFailure : public Failure {
		  public:
			CloseFailure( FileAccess &fa, int errno_, const char *const msg );
			virtual void defaultTerminate() override;
		}; // FileAccess::CloseFailure

		_Exception SeekFailure : public Failure {
			const off_t offset;
			const int whence;
		  public:
			SeekFailure( const FileAccess &fa, int errno_, const off_t offset, const int whence, const char *const msg );
			virtual void defaultTerminate() override;
		}; // FileAccess::SeekFailure

		_Exception SyncFailure : public Failure {
		  public:
			SyncFailure( const FileAccess &fa, int errno_, const char *const msg );
			virtual void defaultTerminate() override;
		}; // FileAccess::SyncFailure

		_Exception ReadFailure : public Failure {
		  protected:
			const char *buf;
			const int len;
			const uDuration *timeout;
		  public:
			ReadFailure( const FileAccess &fa, int errno_, const char *buf, const int len, const uDuration *timeout, const char *const msg );
			virtual void defaultTerminate() override;
		}; // FileAccess::ReadFailure

		_Exception ReadTimeout : public ReadFailure {
		  public:
			ReadTimeout( const FileAccess &fa, const char *buf, const int len, const uDuration *timeout, const char *const msg );
		}; // FileAccess::ReadTimeout

		_Exception WriteFailure : public Failure {
		  protected:
			const char *buf;
			const int len;
			const uDuration *timeout;
		  public:
			WriteFailure( const FileAccess &fa, int errno_, const char *buf, const int len, const uDuration *timeout, const char *const msg );
			//virtual void defaultResume();		// handle special case when errno == EIO
			virtual void defaultTerminate() override;
		}; // FileAccess::WriteFailure

		_Exception WriteTimeout : public WriteFailure {
		  public:
			WriteTimeout( const FileAccess &fa, const char *buf, const int len, const uDuration *timeout, const char *const msg );
		}; // FileAccess::WriteTimeout


		FileAccess();
		FileAccess( uFile &f, int flags, int mode = 0644 );
		FileAccess( const char *name, int flags, int mode = 0644 );
		_Mutex virtual ~FileAccess();

		void open( uFile &f, int flags, int mode = 0644 );
		off_t lseek( off_t offset, int whence );
		int fsync();

		void status( struct stat &buf ) {
			file->status( buf );
		} // FileAccess::status
	}; // FileAccess


	uFile() {
		name = nullptr;
		accessCnt = 0;
	} // uFile::uFile

	uFile( const char *name ) {
		uFile::name = new char[strlen( name ) + 1];
		strcpy( uFile::name, name );
		accessCnt = 0;
	} // uFile::uFile

	virtual ~uFile();

	const char *setName( char *name );
	const char *getName() const;
	void status( struct stat &buf );
}; // uFile


//######################### uPipe #########################


class uPipe {
  public:
	_Exception Failure : public uIOFailure {
		const uPipe &p;
	  protected:
		Failure( const uPipe &pipe, int errno_, const char *const msg );
	  public:
		const uPipe &pipe() const { return p; }
		virtual void defaultTerminate() override;
	}; // uPipe::Failure

	_Exception OpenFailure : public Failure {
	  public:
		OpenFailure( const uPipe &pipe, int errno_, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uPipe::OpenFailure

	_Exception CloseFailure : public Failure {
	  public:
		CloseFailure( const uPipe &pipe, int errno_, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uPipe::CloseFailure

	class End : public uFileIO {
		friend class uPipe;

		uPipe *pipe;
		uIOaccess access;

		void readFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op );
		void readTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op );
		void writeFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op );
		void writeTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op );

		End() : uFileIO( access ) {}
		_Mutex virtual ~End() {}
	  public:
		_Exception Failure : public uPipe::Failure {
			const End &end;
			int fd;
		  protected:
			Failure( const End &end, int errno_, const char *const msg );
		  public:
			const End &pipeend() const { return end; }
			int fileDescriptor() const { return end.access.fd; }
			virtual void defaultTerminate() override;
		}; // End::Failure

		_Exception ReadFailure : public Failure {
		  protected:
			const char *buf;
			const int len;
			const uDuration *timeout;
		  public:
			ReadFailure( const End &end, int errno_, const char *buf, const int len, const uDuration *timeout, const char *const msg );
			virtual void defaultTerminate() override;
		}; // End::ReadFailure

		_Exception ReadTimeout : public ReadFailure {
		  public:
			ReadTimeout( const End &end, const char *buf, const int len, const uDuration *timeout, const char *const msg );
		}; // End::ReadTimeout

		_Exception WriteFailure : public Failure {
		  protected:
			const char *buf;
			const int len;
			const uDuration *timeout;
		  public:
			WriteFailure( const End &end, int errno_, const char *buf, const int len, const uDuration *timeout, const char *const msg );
			//virtual void defaultResume();				// handle special case when errno == EIO
			virtual void defaultTerminate() override;
		}; // End::WriteFailure

		_Exception WriteTimeout : public WriteFailure {
		  public:
			WriteTimeout( const End &end, const char *buf, const int len, const uDuration *timeout, const char *const msg );
		}; // End::WriteTimeout
	}; // End

	End ends[2];
	End &left() { return ends[0]; }
	End &right() { return ends[1]; }

	uPipe();
	~uPipe();
}; // uPipe


// Local Variables: //
// compile-command: "make install" //
// End: //
