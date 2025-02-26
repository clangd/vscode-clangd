//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uFile.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue Mar 29 16:42:36 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Wed Oct  6 13:35:46 2021
// Update Count     : 487
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
#include <uFile.h>

//#include <uDebug.h>

#include <algorithm>
using std::min;

#include <cstring>										// strerror
#include <unistd.h>										// read, write, close, etc.
#include <sys/uio.h>									// readv, writev


//######################### uFileIO #########################


int uFileIO::read( char *buf, int len, uDuration *timeout ) {
	int rlen;

	struct Read : public uIOClosure {
		char *buf;
		int len;

		int action() {
#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::read_syscalls, 1 );
#endif // __U_STATISTICS__
			return ::read( access.fd, buf, len );
		}
		Read( uIOaccess &access, int &rlen ) : uIOClosure( access, rlen ) {}
	} readClosure( access, rlen );

	if ( access.poll.getStatus() == uPoll::NeverPoll ) { // chunk blocking disk read
#ifdef __U_READ_CHUNGKING__
		static const int ChunkSize = 256 * 1024;
#endif // __U_READ_CHUNGKING__
		int count;
		for ( count = 0;; ) {							// ensure all data is read
			readClosure.buf = buf + count;
#ifdef __U_READ_CHUNGKING__
			readClosure.len = min( len - count, ChunkSize );
#else
			readClosure.len = len - count;
#endif // __U_READ_CHUNGKING__
			readClosure.wrapper();
			if ( rlen == -1 ) {
#ifdef __U_STATISTICS__
				uFetchAdd( UPP::Statistics::read_errors, 1 );
#endif // __U_STATISTICS__
				readFailure( readClosure.errno_, buf, len, timeout, "read" );
			} // if
			count += rlen;
		  if ( rlen < readClosure.len ) break;			// read less than request on specific read => no more data
		  if ( count == len ) break;					// transferred across all reads
#ifdef __U_READ_CHUNGKING__
#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::read_chunking, 1 );
#endif // __U_STATISTICS__
			uThisTask().yield();						// allow other tasks to make progress
#endif // __U_READ_CHUNGKING__
		} // for

#ifdef __U_STATISTICS__
		uFetchAdd( UPP::Statistics::read_bytes, count );
#endif // __U_STATISTICS__
		return count;
	} else {
		readClosure.buf = buf;
		readClosure.len = len;
		readClosure.wrapper();
		if ( rlen == -1 && readClosure.errno_ == U_EWOULDBLOCK ) {
#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::read_eagain, 1 );
#endif // __U_STATISTICS__
			if ( ! readClosure.select( uCluster::ReadSelect, timeout ) ) {
				readTimeout( buf, len, timeout, "read" );
			} // if
		} // if
		if ( rlen == -1 ) {
#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::read_errors, 1 );
#endif // __U_STATISTICS__
			readFailure( readClosure.errno_, buf, len, timeout, "read" );
		} // if

#ifdef __U_STATISTICS__
		uFetchAdd( UPP::Statistics::read_bytes, rlen );
#endif // __U_STATISTICS__
		return rlen;
	} // if
} // uFileIO::read


int uFileIO::readv( const struct iovec *iov, int iovcnt, uDuration *timeout ) {
	int rlen;

	struct Readv : public uIOClosure {
		const struct iovec *iov;
		int iovcnt;

		int action() { return ::readv( access.fd, iov, iovcnt ); }
		Readv( uIOaccess &access, int &rlen, const struct iovec *iov, int iovcnt ) : uIOClosure( access, rlen ), iov( iov ), iovcnt( iovcnt ) {}
	} readvClosure( access, rlen, iov, iovcnt );

	readvClosure.wrapper();
	if ( rlen == -1 && readvClosure.errno_ == U_EWOULDBLOCK ) {
		if ( ! readvClosure.select( uCluster::ReadSelect, timeout ) ) {
			readTimeout( (const char *)iov, iovcnt, timeout, "readv" );
		} // if
	} // if
	if ( rlen == -1 ) {
		readFailure( readvClosure.errno_, (const char *)iov, iovcnt, timeout, "readv" );
	} // if

#ifdef __U_STATISTICS__
	uFetchAdd( UPP::Statistics::read_bytes, rlen );
#endif // __U_STATISTICS__
	return rlen;
} // uFileIO::readv


int uFileIO::write( const char *buf, int len, uDuration *timeout ) {
	int wlen;

	struct Write : public uIOClosure {
		const char *buf;
		int len;

		int action() {
#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::write_syscalls, 1 );
#endif // __U_STATISTICS__
			return ::write( access.fd, buf, len );
		}
		Write( uIOaccess &access, int &wlen ) : uIOClosure( access, wlen ) {}
	} writeClosure( access, wlen );

	for ( int count = 0;; ) {							// ensure all data is written
		writeClosure.buf = buf + count;
		writeClosure.len = len - count;
		writeClosure.wrapper();
		if ( wlen == -1 && writeClosure.errno_ == U_EWOULDBLOCK ) {
#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::write_eagain, 1 );
#endif // __U_STATISTICS__
			if ( ! writeClosure.select( uCluster::WriteSelect, timeout ) ) {
				writeTimeout( buf, len, timeout, "write" );
			} // if
		} // if
		if ( wlen == -1 ) {
			// EIO means the write is to stdout but the shell has terminated (I think). Normally, people want this to
			// work as if stdout is magically redirected to /dev/null, instead of aborting the program.
			if ( writeClosure.errno_ == EIO ) break;
#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::write_errors, 1 );
#endif // __U_STATISTICS__
			writeFailure( writeClosure.errno_, buf, len, timeout, "write" );
		} // if
		count += wlen;
	  if ( count == len ) break;						// transferred across all writes
	} // for

#ifdef __U_STATISTICS__
	uFetchAdd( UPP::Statistics::write_bytes, len );
#endif // __U_STATISTICS__
	return len;											// always return the specified length
} // uFileIO::write


int uFileIO::writev( const struct iovec *iov, int iovcnt, uDuration *timeout ) {
	int wlen;

	struct Writev : public uIOClosure {
		const struct iovec *iov;
		int iovcnt;

		int action() { return ::writev( access.fd, iov, iovcnt ); }
		Writev( uIOaccess &access, int &wlen, const struct iovec *iov, int iovcnt ) : uIOClosure( access, wlen ), iov( iov ), iovcnt( iovcnt ) {}
	} writevClosure( access, wlen, iov, iovcnt );

	writevClosure.wrapper();
	if ( wlen == -1 && writevClosure.errno_ == U_EWOULDBLOCK ) {
		if ( ! writevClosure.select( uCluster::WriteSelect, timeout ) ) {
			writeTimeout( (const char *)iov, iovcnt, timeout, "writev" );
		} // if
	} // if
	if ( wlen == -1 && writevClosure.errno_ != EIO ) {
		// EIO means the write is to stdout but the shell has terminated (I think). Normally, people want this to work
		// as if stdout is magically redirected to /dev/null, instead of aborting the program.
		writeFailure( writevClosure.errno_, (const char *)iov, iovcnt, timeout, "writev" );
	} // if

#ifdef __U_STATISTICS__
	uFetchAdd( UPP::Statistics::write_bytes, wlen );
#endif // __U_STATISTICS__
	return wlen;
} // uFileIO::writev


//######################### FileAccess #########################


uFile::FileAccess::Failure::Failure( const FileAccess &fa, int errno_, const char *const msg ) : uFile::Failure( *fa.file, errno_, msg ), fa( fa ) {
	fd = fa.access.fd;
} // uFile::FileAccess::Failure::Failure

void uFile::FileAccess::Failure::defaultTerminate() {
	abort( "(FileAccess &)%p( file:%p ), %.256s file \"%.256s\".",
		   &fileAccess(), &file(), message(), getName() );
} // uFile::FileAccess::Failure::defaultTerminate


uFile::FileAccess::OpenFailure::OpenFailure( FileAccess &fa, int errno_, int flags, int mode, const char *const msg ) :
	uFile::FileAccess::Failure( fa, errno_, msg ), flags( flags ), mode( mode ) {}

void uFile::FileAccess::OpenFailure::defaultTerminate() {
	abort( "(FileAccess &)%p.FileAccess( file:%p, flags:0x%x, mode:0x%x ), %.256s \"%.256s\".\nError(%d) : %s.",
		   &fileAccess(), &file(), flags, mode, message(), getName(), errNo(), strerror( errNo() ) );
} // uFile::OpenFailure::defaultTerminate


uFile::FileAccess::CloseFailure::CloseFailure( FileAccess &fa, int errno_, const char *const msg ) : uFile::FileAccess::Failure( fa, errno_, msg ) {}

void uFile::FileAccess::CloseFailure::defaultTerminate() {
	abort( "(FileAccess &)%p.~FileAccess(), %.256s \"%.256s\".\nError(%d) : %s.",
		   &fileAccess(), message(), getName(), errNo(), strerror( errNo() ) );
} // uFile::CloseFailure::defaultTerminate


uFile::FileAccess::SeekFailure::SeekFailure( const FileAccess &fa, int errno_, const off_t offset, const int whence, const char *const msg ) :
	uFile::FileAccess::Failure( fa, errno_, msg ), offset( offset ), whence( whence ) {}

void uFile::FileAccess::SeekFailure::defaultTerminate() {
	abort( "(uFile &)%p.lseek( offset:%ld, whence:%d ), %.256s file \"%.256s\".\nError(%d) : %s.",
		   &file(), (long int)offset, whence, message(), getName(), errNo(), strerror( errNo() ) );
} // uFile::FileAccess::SeekFailure::defaultTerminate


uFile::FileAccess::SyncFailure::SyncFailure( const FileAccess &fa, int errno_, const char *const msg ) : uFile::FileAccess::Failure( fa, errno_, msg ) {}

void uFile::FileAccess::SyncFailure::defaultTerminate() {
	abort( "(FileAccess &)%p.fsync(), %.256s \"%.256s\".\nError(%d) : %s.",
		   &file(), message(), getName(), errNo(), strerror( errNo() ) );
} // uFile::FileAccess::SyncFailure::defaultTerminate


// void uFile::FileAccess::WriteFailure::defaultResume() {
//	 if ( errNo() != EIO ) {
// 	_Throw *this;
//	 } // if
// } // uFile::FileAccess::WriteFailure::defaultResume


uFile::FileAccess::ReadFailure::ReadFailure( const FileAccess &fa, int errno_, const char *buf, const int len, const uDuration *timeout, const char *const msg ) :
	uFile::FileAccess::Failure( fa, errno_, msg ), buf( buf ), len( len ), timeout( timeout ) {}

void uFile::FileAccess::ReadFailure::defaultTerminate() {
	abort( "(FileAccess &)%p.read( buf:%p, len:%d, timeout:%p ) : %.256s for file descriptor %d.\nError(%d) : %s.",
		   &fileAccess(), buf, len, timeout, message(), fileDescriptor(), errNo(), strerror( errNo() ) );
} // uFile::FileAccess::ReadFailure::defaultTerminate

uFile::FileAccess::ReadTimeout::ReadTimeout( const FileAccess &fa, const char *buf, const int len, const uDuration *timeout, const char *const msg ) :
	uFile::FileAccess::ReadFailure( fa, ETIMEDOUT, buf, len, timeout, msg ) {}


uFile::FileAccess::WriteFailure::WriteFailure( const FileAccess &fa, int errno_, const char *buf, const int len, const uDuration *timeout, const char *const msg ) :
	uFile::FileAccess::Failure( fa, errno_, msg ), buf( buf ), len( len ), timeout( timeout ) {}

void uFile::FileAccess::WriteFailure::defaultTerminate() {
	abort( "(FileAccess &)%p.write( buf:%p, len:%d, timeout:%p ) : %.256s for file descriptor %d.\nError(%d) : %s.",
		   &fileAccess(), buf, len, timeout, message(), fileDescriptor(), errNo(), strerror( errNo() ) );
} // uFile::FileAccess::WriteFailure::defaultTerminate

uFile::FileAccess::WriteTimeout::WriteTimeout( const FileAccess &fa, const char *buf, const int len, const uDuration *timeout, const char *const msg ) :
	uFile::FileAccess::WriteFailure( fa, ETIMEDOUT, buf, len, timeout, msg ) {}


void uFile::FileAccess::readFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcat( strcpy( msg, "file " ), op ), " fails" );
	_Throw uFile::FileAccess::ReadFailure( *this, errno_, buf, len, timeout, msg );
} // uFile::FileAccess::readFailure


void uFile::FileAccess::readTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcpy( msg, "timeout during file " ), op );
	_Throw uFile::FileAccess::ReadTimeout( *this, buf, len, timeout, msg );
} // uFile::FileAccess::readTimeout


void uFile::FileAccess::writeFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcat( strcpy( msg, "file " ), op ), " fails" );
	_Throw uFile::FileAccess::WriteFailure( *this, errno_, buf, len, timeout, msg );
} // uFile::FileAccess::writeFailure


void uFile::FileAccess::writeTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcpy( msg, "timeout during file " ), op );
	_Throw uFile::FileAccess::WriteTimeout( *this, buf, len, timeout, msg );
} // uFile::FileAccess::writeTimeout


void uFile::FileAccess::createAccess( int flags, int mode ) {
	for ( ;; ) {
		access.fd = ::open( file->name, flags, mode );
	  if ( access.fd != -1 || errno != EINTR ) break;	// timer interrupt ?
	} // for
	if ( access.fd == -1 ) {
		_Throw uFile::FileAccess::OpenFailure( *this, errno, flags, mode, "unable to access file" );
	} // if
	access.poll.computeStatus( access.fd );
	if ( access.poll.getStatus() == uPoll::AlwaysPoll ) access.poll.setPollFlag( access.fd );
	file->access();
} // uFile::FileAccess::createAccess


uFile::FileAccess::FileAccess() : uFileIO( access ), own( false ) {
} // uFile::FileAccess::FileAccess


uFile::FileAccess::FileAccess( uFile &f, int flags, int mode ) : uFileIO( access ), file( &f ), own( false ) {
	createAccess( flags, mode );
} // uFile::FileAccess::FileAccess


uFile::FileAccess::FileAccess( const char *name, int flags, int mode ) : uFileIO( access ), own( true ) {
	file = new uFile( name );
	createAccess( flags, mode );
} // uFile::FileAccess::FileAccess


uFile::FileAccess::~FileAccess() {
	file->unaccess();
	if ( access.poll.getStatus() == uPoll::AlwaysPoll ) access.poll.clearPollFlag( access.fd );
	if ( access.fd >= 3 ) {								// don't close the standard file descriptors
		int retcode;

		for ( ;; ) {
			retcode = ::close( access.fd );
		  if ( retcode != -1 || errno != EINTR ) break;	// timer interrupt ?
		} // for
		if ( retcode == -1 ) {
			if ( ! std::__U_UNCAUGHT_EXCEPTION__() ) _Throw uFile::FileAccess::CloseFailure( *this, errno, "unable to terminate access to file" );
		} // if
	} // if
	if ( own ) delete file;
} // uFile::FileAccess::~FileAccess


void uFile::FileAccess::open( uFile &f, int flags, int mode ) {
	file = &f;
	createAccess( flags, mode );
} // uFile::FileAccess::FileAccess


off_t uFile::FileAccess::lseek( off_t offset, int whence ) {
	off_t retcode;

	for ( ;; ) {
		retcode = ::lseek( access.fd, offset, whence );
	  if ( retcode != -1 || errno != EINTR ) break;		// timer interrupt ?
	} // for
	if ( retcode == -1 ) {
		_Throw uFile::FileAccess::SeekFailure( *this, errno, offset, whence, "could not seek file" );
	} // if
	return retcode;
} // uFile::FileAccess::lseek


int uFile::FileAccess::fsync() {
	int retcode;

	for ( ;; ) {
		retcode = ::fsync( access.fd );
	  if ( retcode != -1 || errno != EINTR ) break;		// timer interrupt ?
	} // for
	if ( retcode == -1 ) {
		_Throw uFile::FileAccess::SyncFailure( *this, errno, "could not fsync file" );
	} // if
	return retcode;
} // uFile::FileAccess::fsync


//######################### uFile #########################


uFile::Failure::Failure( const uFile &f, int errno_, const char *const msg ) : uIOFailure( errno_, msg ), f( f ) {
	// file name is copied because its storage can be freed and scrubbed before handler starts
	uEHM::strncpy( name, f.getName(), uEHMMaxName );
} // uFile::Failure::Failure

const uFile &uFile::Failure::file() const { return f; }

const char *uFile::Failure::getName() const { return name; }

void uFile::Failure::defaultTerminate() {
	abort( "(uFile &)%p, %.256s \"%.256s\".", &file(), message(), getName() );
} // uFile::Failure::defaultTerminate


uFile::TerminateFailure::TerminateFailure( const uFile &f, int errno_, const int accessCnt, const char *const msg ) :
	uFile::Failure( f, errno_, msg ), accessCnt( accessCnt ) {}

void uFile::TerminateFailure::defaultTerminate() {
	abort( "(uFile &)%p.~uFile(), %.256s, %d accessor(s) outstanding.", &file(), message(), accessCnt );
} // uFile::TerminateFailure::defaultTerminate


uFile::StatusFailure::StatusFailure( const uFile &f, int errno_, const struct stat &buf, const char *const msg ) : uFile::Failure( f, errno_, msg ), buf( buf ) {}

void uFile::StatusFailure::defaultTerminate() {
	abort( "(uFile &)%p.status( buf:%p ), %.256s \"%.256s\".\nError(%d) : %s.",
		   &file(), &buf, message(), getName(), errNo(), strerror( errNo() ) );
} // uFile::StatusFailure::defaultTerminate


uFile::~uFile() {
	if ( accessCnt != 0 && ! std::__U_UNCAUGHT_EXCEPTION__() ) {
		TerminateFailure temp( *this, EINVAL, accessCnt, "terminating access with outstanding accessor(s)" );
		if ( name != 0 ) delete name;					// safe to delete name as the name is copied in the exception object
		_Throw temp;
	} // if
	if ( name != 0 ) delete name;						// safe to delete name as the name is copied in the exception object
} // uFile::~uFile


const char *uFile::setName( char *name ) {
	// File name is NOT copied => name must exist for the duration of the uFile object.
	char *temp = uFile::name;							// copy previous file-name address
	uFile::name = name;
	return temp;										// return previous file-name address
} // uFile::setName


const char *uFile::getName() const {
	return
#ifdef __U_DEBUG__
		( name == nullptr || name == (const char *)-1 ) ? "*unknown*" : // storage might be scrubbed
#endif // __U_DEBUG__
		name;
} // uFile::getName


void uFile::status( struct stat &buf ) {
	int retcode;

	for ( ;; ) {
		retcode = ::stat( name, &buf );
	  if ( retcode != -1 || errno != EINTR ) break;		// timer interrupt ?
	} // for
	if ( retcode == -1 ) {
		_Throw uFile::StatusFailure( *this, errno, buf, "could not obtain statistical information for file" );
	} // if
} // uFile::status


//######################### End #########################


uPipe::End::Failure::Failure( const End &end, int errno_, const char *const msg ) : uPipe::Failure( *end.pipe, errno_, msg ), end( end ) {
} // uPipe::End::Failure::Failure

void uPipe::End::Failure::defaultTerminate() {
	abort( "(uPipe::End &)%p(), pipe:%p %.256s.", &pipeend(), &pipeend().pipe, message() );
} // uPipe::End::Failure::defaultTerminate


uPipe::End::ReadFailure::ReadFailure( const End &pe, int errno_, const char *buf, const int len, const uDuration *timeout, const char *const msg ) :
	Failure( pe, errno_, msg ), buf( buf ), len( len ), timeout( timeout ) {}

void uPipe::End::ReadFailure::defaultTerminate() {
	abort( "(uPipe::End &)%p.read( buf:%p, len:%d, timeout:%p ) : %.256s for file descriptor %d.\nError(%d) : %s.",
		   &pipeend(), buf, len, timeout, message(), fileDescriptor(), errNo(), strerror( errNo() ) );
} // uPipe::End::ReadFailure::defaultTerminate

uPipe::End::ReadTimeout::ReadTimeout( const End &pe, const char *buf, const int len, const uDuration *timeout, const char *const msg ) :
	ReadFailure( pe, ETIMEDOUT, buf, len, timeout, msg ) {}


uPipe::End::WriteFailure::WriteFailure( const End &pe, int errno_, const char *buf, const int len, const uDuration *timeout, const char *const msg ) :
	Failure( pe, errno_, msg ), buf( buf ), len( len ), timeout( timeout ) {}

void uPipe::End::WriteFailure::defaultTerminate() {
	abort( "(uPipe::End &)%p.write( buf:%p, len:%d, timeout:%p ) : %.256s for file descriptor %d.\nError(%d) : %s.",
		   &pipeend(), buf, len, timeout, message(), fileDescriptor(), errNo(), strerror( errNo() ) );
} // uPipe::End::WriteFailure::defaultTerminate

uPipe::End::WriteTimeout::WriteTimeout( const End &pe, const char *buf, const int len, const uDuration *timeout, const char *const msg ) :
	WriteFailure( pe, ETIMEDOUT, buf, len, timeout, msg ) {}


void uPipe::End::readFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcat( strcpy( msg, "pipe " ), op ), " fails" );
	_Throw End::ReadFailure( *this, errno_, buf, len, timeout, msg );
} // uPipe::End::readFailure


void uPipe::End::readTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcpy( msg, "timeout during pipe " ), op );
	_Throw End::ReadTimeout( *this, buf, len, timeout, msg );
} // uPipe::End::readTimeout


void uPipe::End::writeFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcat( strcpy( msg, "pipe " ), op ), " fails" );
	_Throw End::WriteFailure( *this, errno_, buf, len, timeout, msg );
} // uPipe::End::writeFailure


void uPipe::End::writeTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcpy( msg, "timeout during pipe " ), op );
	_Throw End::WriteTimeout( *this, buf, len, timeout, msg );
} // uPipe::End::writeTimeout


//######################### uPipe #########################


uPipe::Failure::Failure( const uPipe &pipe, int errno_, const char *const msg ) : uIOFailure( errno_, msg ), p( pipe ) {}

void uPipe::Failure::defaultTerminate() {
	abort( "(uPipe &)%p, %.256s.", &pipe(), message() );
} // uPipe::Failure::defaultTerminate


uPipe::OpenFailure::OpenFailure( const uPipe &pipe, int errno_, const char *const msg ) : Failure( pipe, errno_, msg ) {}

void uPipe::OpenFailure::defaultTerminate() {
	abort( "(uPipe &)%p.uPipe(), %.256s.\nError(%d) : %s.", &pipe(), message(), errNo(), strerror( errNo() ) );
} // uPipe::OpenFailure::defaultTerminate


uPipe::CloseFailure::CloseFailure( const uPipe &pipe, int errno_, const char *const msg ) : Failure( pipe, errno_, msg ) {}

void uPipe::CloseFailure::defaultTerminate() {
	abort( "(uPipe &)%p.~uPipe(), %.256s.\nError(%d) : %s.", &pipe(), message(), errNo(), strerror( errNo() ) );
} // uPipe::CloseFailure::defaultTerminate


uPipe::uPipe() {
	int fds[2];
	int retcode;

	for ( ;; ) {
		retcode = ::pipe( fds );
	  if ( retcode != -1 || errno != EINTR ) break;		// timer interrupt ?
	} // for
	if ( retcode == -1 ) {
		_Throw uPipe::OpenFailure( *this, errno, "unable to create pipe" );
	} // if
	ends[0].pipe = this;
	ends[0].access.fd = fds[0];
	ends[1].pipe = this;
	ends[1].access.fd = fds[1];

	ends[0].access.poll.setStatus( uPoll::AlwaysPoll );
	ends[0].access.poll.setPollFlag( ends[0].access.fd );
	ends[1].access.poll.setStatus( uPoll::AlwaysPoll );
	ends[1].access.poll.setPollFlag( ends[1].access.fd );
} // uPipe::uPipe


uPipe::~uPipe() {
	int retcode;
	for ( unsigned int i = 0; i < 2; i += 1 ) {
		for ( ;; ) {
			retcode = ::close( ends[i].access.fd );
		  if ( retcode != -1 || errno != EINTR ) break;	// timer interrupt ?
		} // for
		if ( retcode == -1 ) {
			if ( ! std::__U_UNCAUGHT_EXCEPTION__() ) _Throw uPipe::CloseFailure( *this, errno, "unable to close pipe" );
		} // if
	} // for
} // uPipe::~uPipe


// Local Variables: //
// compile-command: "make install" //
// End: //
