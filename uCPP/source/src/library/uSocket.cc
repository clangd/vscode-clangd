//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uSocket.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Tue Mar 29 17:06:26 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Oct  7 07:55:10 2023
// Update Count     : 1102
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
#include <uSocket.h>

//#include <uDebug.h>

#include <arpa/inet.h>									// inet_aton
#include <cstring>										// strerror, memset
#include <unistd.h>										// read, write, close, etc.
#include <sys/sendfile.h>

#ifndef SUN_LEN
#define SUN_LEN(su) (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif


//######################### uSocket #########################


void uSocket::createSocket( int domain, int type, int protocol ) {
	for ( ;; ) {
		access.fd = ::socket( domain, type, protocol );
	  if ( access.fd != -1 || errno != EINTR ) break;	// timer interrupt ?
	} // for
	if ( access.fd == -1 ) {
	    // The exception should be caught by constructor in uSocketServer or uSocketClient and throw a different
		// exception indicating the failure to create a server or a client.  However, it cannot be implemented because
		// the initialization list in a constructor cannot be encapsulated in a try block.  Actually, there is a feature
		// in c++ allowing catch clause to handle exception propoagated out of the initialization list but g++ does not
		// have it yet.

	    _Throw OpenFailure( *this, errno, domain, type, protocol, "unable to open socket" );
	} // if

	// Sockets always treated as non-blocking.
	access.poll.setStatus( uPoll::AlwaysPoll );
	access.poll.setPollFlag( access.fd );

	// Sockets always have local-address reuse enable.
	const socklen_t enable = 1;							// 1 => enable option
	if ( setsockopt( access.fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable) ) == -1 ) {
	    _Throw OpenFailure( *this, errno, domain, type, protocol, "unable to set socket-option" );
	} // if
} // uSocket::createSocket


void uSocket::convertFailure( const char *kind, const char *text ) {
	_Throw IPConvertFailure( kind, text );
} // uSocket::convertFailure


uSocket::~uSocket() {
	int retcode;

	for ( ;; ) {
		retcode = ::close( access.fd );
	  if ( retcode != -1 || errno != EINTR ) break;		// timer interrupt ?
	} // for
	if ( retcode == -1 ) {
	    if ( ! std::__U_UNCAUGHT_EXCEPTION__() ) _Throw CloseFailure( *this, errno, "unable to close socket" );
	} // if
} // uSocket::~uSocket


in_addr uSocket::gethostbyname( const char *name ) {
	in_addr host;
	hostent hp;
	int herrno = 0;
	char buffer[8 * 1024];								// best guess at size
	hostent *rc;

	for ( int i = 0; i < 5; i += 1 ) {					// N tries
		::gethostbyname_r( name, &hp, buffer, sizeof(buffer), &rc, &herrno );
	  if ( rc != 0 ) break;								// success ?
	  if ( herrno != TRY_AGAIN ) break;					// failure ?
		sleep( 1 );										// TRY_AGAIN => delay and try again
	} // for
	if ( rc == 0 ) {
		convertFailure( "name", name );
	} // if
	memcpy( &host, hp.h_addr, hp.h_length );
	return host;
} // uSocket::gethostbyname


in_addr uSocket::gethostbyip( const char *ip ) {
	in_addr host;
	if ( inet_aton( ip, &host ) == 0 ) {
		convertFailure( "ip string", ip );
	} // if
	return host;
} // uSocket::gethostbyip


uSocket::IPConvertFailure::IPConvertFailure( const char *kind, const char *text ) : uIOFailure( EINVAL, "IP conversion failed" ), kind( kind ), text( text ) {
} // uSocket::IPConvertFailure::IPConvertFailure

void uSocket::IPConvertFailure::defaultTerminate() {
	abort( "uSocket::IPConvertFailure : %s from %s \"%s\" to ip value.", message(), kind, text );
} // uSocket::IPConvertFailure::defaultTerminate


uSocket::Failure::Failure( const uSocket &socket, int errno_, const char *const msg ) : uIOFailure( errno_, msg ), socket_( socket ) {}

const uSocket &uSocket::Failure::socket() const { return socket_; }

void uSocket::Failure::defaultTerminate() {
	abort( "(uSocket &)%p, %.256s.", &socket(), message() );
} // uSocket::Failure::defaultTerminate


uSocket::OpenFailure::OpenFailure( const uSocket &socket, int errno_, const int domain, const int type, const int protocol, const char *const msg ) :
	uSocket::Failure( socket, errno_, msg ), domain( domain ), type( type ), protocol( protocol ) {}

void uSocket::OpenFailure::defaultTerminate() {
	abort( "(uSocket &)%p.uSocket( domain:%d, type:%d, protocol:%d ), %.256s.\nError(%d) : %s.",
		   &socket(), domain, type, protocol, message(), errNo(), strerror( errNo() ) );
} // uSocket::OpenFailure::defaultTerminate


uSocket::CloseFailure::CloseFailure( const uSocket &socket, int errno_, const char *const msg ) : uSocket::Failure( socket, errno_, msg ) {}

void uSocket::CloseFailure::defaultTerminate() {
	abort( "(uSocket &)%p.~uSocket(), %.256s %d.\nError(%d) : %s.",
		   &socket(), message(), socket().access.fd, errNo(), strerror( errNo() ) );
} // uSocket::CloseFailure::defaultTerminate


//######################### uSocketIO #########################


int uSocketIO::send( char *buf, int len, int flags, uDuration *timeout ) {
	int slen;

	struct Send : public uIOClosure {
		char *buf;
		int len;
		int flags;

		int action() { return ::send( access.fd, buf, len, flags ); }
		Send( uIOaccess &access, int &slen, char *buf, int len, int flags ) : uIOClosure( access, slen ), buf( buf ), len( len ), flags( flags ) {}
	} sendClosure( access, slen, buf, len, flags );

	sendClosure.wrapper();
	if ( slen == -1 && sendClosure.errno_ == U_EWOULDBLOCK ) {
		if ( ! sendClosure.select( uCluster::WriteSelect, timeout ) ) {
			writeTimeout( buf, len, flags, nullptr, 0, timeout, "send" );
		} // if
	} // if
	if ( slen == -1 ) {
		writeFailure( sendClosure.errno_, buf, len, flags, nullptr, 0, timeout, "send" );
	} // if

	return slen;
} // uSocketIO::send


int uSocketIO::sendto( char *buf, int len, struct sockaddr *to, socklen_t tolen, int flags, uDuration *timeout ) {
	int slen;

	struct Sendto : public uIOClosure {
		char *buf;
		int len;
		int flags;
		struct sockaddr *to;
		socklen_t tolen;

		int action() { return ::sendto( access.fd, buf, len, flags, to, tolen ); }
		Sendto( uIOaccess &access, int &slen, char *buf, int len, int flags, struct sockaddr *to, socklen_t tolen ) :
			uIOClosure( access, slen ), buf( buf ), len( len ), flags( flags ), to( to ), tolen( tolen ) {}
	} sendtoClosure( access, slen, buf, len, flags, to, tolen );

	sendtoClosure.wrapper();
	if ( slen == -1 && sendtoClosure.errno_ == U_EWOULDBLOCK ) {
		if ( ! sendtoClosure.select( uCluster::WriteSelect, timeout ) ) {
			writeTimeout( buf, len, flags, nullptr, 0, timeout, "sendto" );
		} // if
	} // if
	if ( slen == -1 ) {
		writeFailure( sendtoClosure.errno_, buf, len, flags, nullptr, 0, timeout, "sendto" );
	} // if

	return slen;
} // uSocketIO::sendto


int uSocketIO::recv( char *buf, int len, int flags, uDuration *timeout ) {
	int rlen;

	struct Recv : public uIOClosure {
		char *buf;
		int len;
		int flags;

		int action() { return ::recv( access.fd, buf, len, flags ); }
		Recv( uIOaccess &access, int &rlen, char *buf, int len, int flags ) : uIOClosure( access, rlen ), buf( buf ), len( len ), flags( flags ) {}
	} recvClosure( access, rlen, buf, len, flags );

	recvClosure.wrapper();
	if ( rlen == -1 && recvClosure.errno_ == U_EWOULDBLOCK ) {
		if ( ! recvClosure.select( uCluster::ReadSelect, timeout ) ) {
			readTimeout( buf, len, flags, nullptr, nullptr, timeout, "recv" );
		} // if
	} // if
	if ( rlen == -1 ) {
		readFailure( recvClosure.errno_, buf, len, flags, nullptr, nullptr, timeout, "recv" );
	} // if

	return rlen;
} // uSocketIO::recv


int uSocketIO::recvfrom( char *buf, int len, struct sockaddr *from, socklen_t *fromlen, int flags, uDuration *timeout ) {
	int rlen;

	struct Recvfrom : public uIOClosure {
		char *buf;
		int len;
		int flags;
		struct sockaddr *from;
		socklen_t *fromlen;

		int action() {
			int rlen;
			rlen = ::recvfrom( access.fd, buf, len, flags, from, fromlen );
			return rlen;
		} // action
		Recvfrom( uIOaccess &access, int &rlen, char *buf, int len, int flags, struct sockaddr *from, socklen_t *fromlen ) :
			uIOClosure( access, rlen ), buf( buf ), len( len ), flags( flags ), from( from ), fromlen( fromlen ) {}
	} recvfromClosure( access, rlen, buf, len, flags, from, fromlen );

	recvfromClosure.wrapper();
	if ( rlen == -1 && recvfromClosure.errno_ == U_EWOULDBLOCK ) {
		if ( ! recvfromClosure.select( uCluster::ReadSelect, timeout ) ) {
			readTimeout( buf, len, flags, from, fromlen, timeout, "recvfrom" );
		} // if
	} // if
	if ( rlen == -1 ) {
		readFailure( recvfromClosure.errno_, buf, len, flags, from, fromlen, timeout, "recvfrom" );
	} // if

	return rlen;
} // uSocketIO::recvfrom


int uSocketIO::recvmsg( struct msghdr *msg, int flags, uDuration *timeout ) {
	int rlen;

	struct Recvmsg : public uIOClosure {
		struct msghdr *msg;
		int flags;

		int action() { return ::recvmsg( access.fd, msg, flags ); }
		Recvmsg( uIOaccess &access, int &rlen, struct msghdr *msg, int flags ) : uIOClosure( access, rlen ), msg( msg ), flags( flags ) {}
	} recvmsgClosure( access, rlen, msg, flags );

	recvmsgClosure.wrapper();
	if ( rlen == -1 && recvmsgClosure.errno_ == U_EWOULDBLOCK ) {
		if ( ! recvmsgClosure.select( uCluster::ReadSelect, timeout ) ) {
			readTimeout( (const char *)msg, 0, flags, nullptr, nullptr, timeout, "recvmsg" );
		} // if
	} // if
	if ( rlen == -1 ) {
		readFailure( recvmsgClosure.errno_, (const char *)msg, 0, flags, nullptr, nullptr, timeout, "recvmsg" );
	} // if

	return rlen;
} // uSocketIO::recvmsg


ssize_t uSocketIO::sendfile( uFile::FileAccess &file, off_t *off, size_t len, uDuration *timeout ) {
	int ret;
	off_t wlen;

	struct Sendfile : public uIOClosure {
		int ret;										// sendfile return code 0/-1
		int in_fd;										// disk file
		off_t *off;										// offset in disk file
		size_t len;										// number of remaining bytes to be read from disk file
		off_t &wlen;									// number of bytes written from disk to the socket
		bool direct;									// do not perform select call in IOPoller

		int action() {
			if ( ! direct ) return 0;
			off_t ret;
			//access.poll.clearPollFlag( access.fd );	// blocking sendfile
#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::sendfile_syscalls, 1 );
#endif // __U_STATISTICS__
			//fprintf( stderr, "sfd:%d, ffd:%d, off:%ld, len:%d, wlen:%d, errno:%d\n", access.fd, in_fd, *off, len, wlen, errno );
			wlen = ret = ::sendfile( access.fd, in_fd, off, len );
			if ( ret == -1 ) wlen = 0;
			else ret = 0;
			//access.poll.setPollFlag( access.fd );	// blocking sendfile
			return ret;
		} // action
		Sendfile( uIOaccess &access, int &ret, int in_fd, off_t *off, int len, off_t &wlen ) :
			uIOClosure( access, ret ), in_fd( in_fd ), off( off ), len( len ), wlen( wlen ), direct( true ) {}
	} sendfileClosure( access, ret, file.fd(), off, len, wlen );

	// sendfile is unusual because it is both blocking and nonblocking, i.e., it is blocking on disk I/O and nonblocking
	// on socket I/O. To handle the nonblocking socket I/O requires using select in uNBIO; however, uNBIO should not
	// perform the sendfile on behalf of the user when the socket becomes available because it could block on file I/O
	// while holding the uNBIO lock. Instead, the "direct" flag is used to trick uNBIO into returning here, where the
	// sendfile is executed.

	for ( off_t count = 0;; ) {							// ensure all data is written
		sendfileClosure.len = len - count;
		sendfileClosure.wrapper();
#ifdef __U_STATISTICS__
		if ( count == 0 && wlen == (__typeof__(wlen))len ) { uFetchAdd( UPP::Statistics::first_sendfile, 1 ); };
#endif // __U_STATISTICS__
		if ( ret == -1 && sendfileClosure.errno_ == U_EWOULDBLOCK ) {
#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::sendfile_eagain, 1 );
#endif // __U_STATISTICS__
			sendfileClosure.direct = false;				// do not perform sendfile in uNBIO
			if ( ! sendfileClosure.select( uCluster::WriteSelect, timeout ) ) {
				sendfileTimeout( file.fd(), off, len, timeout );
			} // if
			sendfileClosure.direct = true;
		} // if
		if ( ret == -1 ) {
#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::sendfile_errors, 1 );
#endif // __U_STATISTICS__
			sendfileFailure( sendfileClosure.errno_, file.fd(), off, len, timeout );
		} // if
		count += wlen;
	  if ( (__typeof__(len))count == len ) break;		// transfer completed ? (cast prevents warning about signed/unsigned comparison)
#ifdef __U_STATISTICS__
//	sendfile_yields += 1;
#endif // __U_STATISTICS__
		uThisTask().yield();							// allow other tasks to make progress
	} // for

	return len;
} // uSocketIO::sendfile


//######################### uSocketServer #########################


void uSocketServer::createSocketServer1( const char *name, int type, int protocol, int backlog ) {
	int retcode;

	uDEBUGPRT( uDebugPrt( "(uSocketServer &)%p.createSocketServer1 attempting binding to name:%s\n", this, name ); );

	if ( strlen( name ) >= sizeof(((sockaddr_un *)saddr)->sun_path) ) {
		openFailure( ENAMETOOLONG, name, 0, uSocket::itoip( 0 ), AF_UNIX, type, protocol, backlog, "socket name too long" );
	} // if

	((sockaddr_un *)saddr)->sun_family = AF_UNIX;
	strcpy( ((sockaddr_un *)saddr)->sun_path, name );
	saddrlen = SUN_LEN( (sockaddr_un *)saddr );
	baddrlen = sizeof(sockaddr_un);

	for ( ;; ) {
		retcode = ::bind( socket.access.fd, saddr, saddrlen );
	  if ( retcode != -1 || errno != EINTR ) break;		// timer interrupt ?
	} // for
	if ( retcode == -1 ) {
	    openFailure( errno, name, 0, uSocket::itoip( 0 ), AF_UNIX, type, protocol, backlog, "unable to bind name to socket" );
	} // if

	if ( type != SOCK_DGRAM ) {							// connection-oriented
		for ( ;; ) {
			retcode = ::listen( socket.access.fd, backlog );
		  if ( retcode != -1 || errno != EINTR ) break; // timer interrupt ?
		} // for
		if ( retcode == -1 ) {
			openFailure( errno, name, 0, uSocket::itoip( 0 ), AF_UNIX, type, protocol, backlog, "unable to listen on socket" );
		} // if
	} // if

	acceptorCnt = 0;

	uDEBUGPRT( uDebugPrt( "(uSocketServer &)%p.createSocketServer1 binding to name:%s\n", this, name ); );
} // uSockeServer::createSocketServer1


void uSocketServer::createSocketServer2( unsigned short port, int type, int protocol, int backlog ) {
	int retcode;

	baddrlen = saddrlen = sizeof(sockaddr_in);

	uDEBUGPRT( uDebugPrt( "(uSocketServer &)%p.createSocketServer2 attempting binding to port:%d, ip:0x%08x\n", this, port, ((inetAddr *)saddr)->sin_addr.s_addr ); );

	for ( ;; ) {
		retcode = ::bind( socket.access.fd, saddr, saddrlen );
	  if ( retcode != -1 || errno != EINTR ) break;		// timer interrupt ?
	} // for
	if ( retcode == -1 ) {
		openFailure( errno, "", port, ((inetAddr *)saddr)->sin_addr, AF_INET, type, protocol, backlog, "unable to bind name to socket" );
	} // if

	if ( type != SOCK_DGRAM ) {							// connection-oriented
		for ( ;; ) {
			retcode = ::listen( socket.access.fd, backlog );
		  if ( retcode != -1 || errno != EINTR ) break; // timer interrupt ?
		} // for
		if ( retcode == -1 ) {
			openFailure( errno, "", port, ((inetAddr *)saddr)->sin_addr, AF_INET, type, protocol, backlog, "unable to listen on socket" );
		} // if
	} // if

	acceptorCnt = 0;

	uDEBUGPRT( uDebugPrt( "(uSocketServer &)%p.createSocketServer2 binding to port:%d, ip:0x%08x\n", this, port, ((inetAddr *)saddr)->sin_addr.s_addr ); );
} // uSocketServer::createSocketServer2


void uSocketServer::createSocketServer3( unsigned short *port, int type, int protocol, int backlog ) {
	createSocketServer2( 0, type, protocol, backlog );	// 0 port number => select an used port

	getsockname( saddr, &saddrlen );					// insert unsed port number into address ("bind" does not do it)
	uDEBUGPRT( uDebugPrt( "(uSocketServer &)%p.createSocketServer3 binding to port:%d\n", this, ntohs( ((sockaddr_in *)saddr)->sin_port ) ); );
	*port = ntohs( ((sockaddr_in *)saddr)->sin_port );
} // uSocketServer::createSocketServer3


void uSocketServer::readFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcpy( msg, "socket " ); strcat( msg, op ); strcat( msg, " fails" );
	_Throw uSocketServer::ReadFailure( *this, errno_, buf, len, 0, nullptr, 0, timeout, msg );
} // uSocketServer::readFailure


void uSocketServer::readTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcpy( msg, "timeout during socket " ), op );
	_Throw uSocketServer::ReadTimeout( *this, buf, len, 0, nullptr, 0, timeout, msg );
} // uSocketServer::readTimeout


void uSocketServer::writeFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcat( strcpy( msg, "socket " ), op ), " fails" );
	_Throw uSocketServer::WriteFailure( *this, errno_, buf, len, 0, nullptr, 0, timeout, msg );
} // uSocketServer::writeFailure


void uSocketServer::writeTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcpy( msg, "timeout during socket " ), op );
	_Throw uSocketServer::WriteTimeout( *this, buf, len, 0, nullptr, 0, timeout, msg );
} // uSocketServer::writeTimeout


void uSocketServer::readFailure( int errno_, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcat( strcpy( msg, "socket " ), op ), " fails" );
	_Throw uSocketServer::ReadFailure( *this, errno_, buf, len, flags, from, fromlen, timeout, msg );
} // uSocketServer::readFailure


void uSocketServer::readTimeout( const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcpy( msg, "timeout during socket " ), op );
	_Throw uSocketServer::ReadTimeout( *this, buf, len, flags, from, fromlen, timeout, msg );
} // uSocketServer::readTimeout


void uSocketServer::writeFailure( int errno_, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcat( strcpy( msg, "socket " ), op ), " fails" );
	_Throw uSocketServer::WriteFailure( *this, errno_, buf, len, flags, to, tolen, timeout, msg );
} // uSocketServer::writeFailure


void uSocketServer::writeTimeout( const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcpy( msg, "timeout during socket " ), op );
	_Throw uSocketServer::WriteTimeout( *this, buf, len, flags, to, tolen, timeout, msg );
} // uSocketServer::writeTimeout


void uSocketServer::sendfileFailure( int errno_, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout ) {
	_Throw uSocketServer::SendfileFailure( *this, errno_, in_fd, off, len, timeout, "socket sendfile fails" );
} // uSocketServer::sendfileFailure


void uSocketServer::sendfileTimeout( const int in_fd, const off_t *off, const size_t len, const uDuration *timeout ) {
	_Throw uSocketServer::SendfileTimeout( *this, in_fd, off, len, timeout, "timeout during socket sendfile" );
} // uSocketServer::sendfileTimeout


void uSocketServer::openFailure( int errno_, const char *const name, const unsigned short port, const in_addr ip, int domain, int type, int protocol, int backlog, const char *const msg ) { // prevent expanding _Exception at call site to reduce stack size
	_Throw OpenFailure( *this, errno_, name, port, ip, domain, type, protocol, backlog, msg );
} // uSocketServer::openFailure


//######################### uSocketAccept #########################


void uSocketAccept::createSocketAcceptor( uDuration *timeout, struct sockaddr *adr, socklen_t *len ) {
	struct Accept : public uIOClosure {
		struct sockaddr *adr;
		socklen_t *len;

		int action() {
			int fd, tmp = 0;
#ifdef __U_STATISTICS__
			uFetchAdd( UPP::Statistics::accept_syscalls, 1 );
#endif // __U_STATISTICS__
			if ( len != nullptr ) tmp = *len;			// save *len, as it may be set to 0 after each attempt
			fd = ::accept( access.fd, adr, len );
			if ( len != nullptr && *len == 0 ) *len = tmp; // reset *len after each attempt
			return fd;
		} // action
		Accept( uIOaccess &access, int &fd, struct sockaddr *adr, socklen_t *len ) : uIOClosure( access, fd ), adr( adr ), len( len ) {}
	} acceptClosure( socketserver.access, access.fd, adr, len );

	baddrlen = saddrlen = socketserver.saddrlen;

	uDEBUGPRT( uDebugPrt( "(uSocketAccept &)%p.uSocketAccept before accept\n", this ); );

	acceptClosure.wrapper();
	if ( access.fd == -1 && acceptClosure.errno_ == U_EWOULDBLOCK ) {
		if ( ! acceptClosure.select( uCluster::ReadSelect, timeout ) ) {
			openTimeout( timeout, adr, len );
		} // if
	} // if
	if ( access.fd == -1 ) {
#ifdef __U_STATISTICS__
		uFetchAdd( UPP::Statistics::accept_errors, 1 );
#endif // __U_STATISTICS__
		openFailure( acceptClosure.errno_, timeout, adr, len );
	} // if

	uDEBUGPRT( uDebugPrt( "(uSocketAccept &)%p.uSocketAccept after accept fd:%d\n", this, access.fd ); );

	// On some UNIX systems the file descriptor created by accept inherits the non-blocking characteristic from the base
	// socket; on other system this does not seem to occur, so explicitly set the file descriptor to non-blocking.

	access.poll.setStatus( uPoll::AlwaysPoll );
	access.poll.setPollFlag( access.fd );
	openAccept = true;
} // uSocketAccept::createSocketAcceptor


void uSocketAccept::readFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcat( strcpy( msg, "socket " ), op ), " fails" );
	_Throw uSocketAccept::ReadFailure( *this, errno_, buf, len, 0, nullptr, 0, timeout, msg );
} // uSocketAccept::readFailure


void uSocketAccept::readTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcpy( msg, "timeout during socket " ), op );
	_Throw uSocketAccept::ReadTimeout( *this, buf, len, 0, nullptr, 0, timeout, msg );
} // uSocketAccept::readTimeout


void uSocketAccept::writeFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcat( strcpy( msg, "socket " ), op ), " fails" );
	_Throw uSocketAccept::WriteFailure( *this, errno_, buf, len, 0, nullptr, 0, timeout, msg );
} // uSocketAccept::writeFailure


void uSocketAccept::writeTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcpy( msg, "timeout during socket " ), op );
	_Throw uSocketAccept::WriteTimeout( *this, buf, len, 0, nullptr, 0, timeout, msg );
} // uSocketAccept::writeTimeout


void uSocketAccept::readFailure( int errno_, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcat( strcpy( msg, "socket " ), op ), " fails" );
	_Throw uSocketAccept::ReadFailure( *this, errno_, buf, len, flags, from, fromlen, timeout, msg );
} // uSocketAccept::readFailure


void uSocketAccept::readTimeout( const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcpy( msg, "timeout during socket " ), op );
	_Throw uSocketAccept::ReadTimeout( *this, buf, len, flags, from, fromlen, timeout, msg );
} // uSocketAccept::readTimeout


void uSocketAccept::writeFailure( int errno_, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcat( strcpy( msg, "socket " ), op ), " fails" );
	_Throw uSocketAccept::WriteFailure( *this, errno_, buf, len, flags, to, tolen, timeout, msg );
} // uSocketAccept::writeFailure


void uSocketAccept::writeTimeout( const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcpy( msg, "timeout during socket " ), op );
	_Throw uSocketAccept::WriteTimeout( *this, buf, len, flags, to, tolen, timeout, msg );
} // uSocketAccept::writeTimeout


void uSocketAccept::sendfileFailure( int errno_, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout ) {
	_Throw uSocketAccept::SendfileFailure( *this, errno_, in_fd, off, len, timeout, "socket sendfile fails" );
} // uSocketAccept::sendfileFailure


void uSocketAccept::sendfileTimeout( const int in_fd, const off_t *off, const size_t len, const uDuration *timeout ) {
	_Throw uSocketAccept::SendfileTimeout( *this, in_fd, off, len, timeout, "timeout during socket sendfile" );
} // uSocketAccept::sendfileTimeout


void uSocketAccept::openFailure( int errno_, const uDuration *timeout, const struct sockaddr *adr, const socklen_t *len ) { // prevent expanding _Exception at call site to reduce stack size
	_Throw OpenFailure( *this, errno_, timeout, adr, len, "unable to accept connection on socket server" );
} // uSocketAccept::openFailure


void uSocketAccept::openTimeout( const uDuration *timeout, const struct sockaddr *adr, const socklen_t *len ) { // prevent expanding _Exception at call site to reduce stack size
	_Throw OpenTimeout( *this, timeout, adr, len, "timeout during accept" );
} // uSocketAccept::openTimeout


void uSocketAccept::close() {
	uDEBUGPRT( uDebugPrt( "(uSocketAccept &)%p.close, enter fd:%d\n", this, access.fd ); );

  if ( ! openAccept ) return;							// closed?

	int retcode;

	for ( ;; ) {
		retcode = ::close( access.fd );
	  if ( retcode != -1 || errno != EINTR ) break;		// timer interrupt ?
	} // for

	openAccept = false;
	if ( retcode == -1 ) {
		// called from ~uSocketAccept()
		if ( ! std::__U_UNCAUGHT_EXCEPTION__() ) _Throw CloseFailure( *this, errno, "unable to close socket acceptor" );
	} // if

	uDEBUGPRT( uDebugPrt( "(uSocketAccept &)%p.close, exit fd:%d\n", this, access.fd ); );
} // uSocketAccept::close


//######################### uSocketClient #########################


void uSocketClient::connectionOriented( const char *name, unsigned short port, const int domain, uDuration *timeout, int type, int protocol ) {
	int retcode;

	struct Connect : public uIOClosure {
		struct sockaddr *adr;
		socklen_t len;
		bool doConnect;

		int action() { return doConnect ? ::connect( access.fd, adr, len ) : 0; }
		Connect( uIOaccess &access, int &retcode, struct sockaddr *adr, socklen_t len ) : uIOClosure( access, retcode ), adr( adr ), len( len ), doConnect( true ) {}
	} connectClosure( socket.access, retcode, saddr, saddrlen );

	connectClosure.wrapper();
	if ( retcode == -1 && connectClosure.errno_ == U_EWOULDBLOCK ) {
		if ( ! connectClosure.select( uCluster::ReadSelect, timeout ) ) {
			openTimeout( name, port, uSocket::itoip( 0 ), timeout, domain, type, protocol, "timeout during connect" );
		} // if
	} // if
	if ( retcode == -1 && connectClosure.errno_ == EINPROGRESS ) {
		// wait for a non-blocking connect to complete without issuing the connect again
		connectClosure.doConnect = false;
		if ( ! connectClosure.select( uCluster::WriteSelect, timeout ) ) {
			openTimeout( name, port, uSocket::itoip( 0 ), timeout, domain, type, protocol, "timeout during connect" );
		} // if
		// check if connection completed
		socklen_t retcodeLen = sizeof(retcode);
		if ( ::getsockopt( socket.access.fd, SOL_SOCKET, SO_ERROR, &retcode, &retcodeLen ) == -1 ) {
			openFailure( connectClosure.errno_, name, port, uSocket::itoip( 0 ), timeout, domain, type, protocol, "unable to connect to socket" );
		} // if
	} // if
	if ( retcode == -1 ) {
		openFailure( connectClosure.errno_, name, port, uSocket::itoip( 0 ), timeout, domain, type, protocol, "unable to connect to socket" );
	} // if
} // uSocketClient::connectionOriented


void uSocketClient::createSocketClient1( const char *name, uDuration *timeout, int type, int protocol ) {
	uDEBUGPRT( uDebugPrt( "(uSocketClient &)%p.createSocketClient1 attempting connection to name:%s\n", this, name ); );

	if ( strlen( name ) >= sizeof(((sockaddr_un *)saddr)->sun_path) ) {
		openFailure( ENAMETOOLONG, name, 0, uSocket::itoip( 0 ), timeout, AF_UNIX, type, protocol, "socket name too long" );
	} // if

	if ( type != SOCK_DGRAM ) {							// connection-oriented
		((sockaddr_un *)saddr)->sun_family = AF_UNIX;
		strcpy( ((sockaddr_un *)saddr)->sun_path, name );
		saddrlen = SUN_LEN( (sockaddr_un *)saddr );
		baddrlen = sizeof(sockaddr_un);

		connectionOriented( name, 0, AF_UNIX, timeout, type, protocol );
	} else {
		int retcode;

		// Cannot atomically create a temporary pipe. Therefore, atomically create a directory and put the pipe in the
		// unique directory. 8-(

		// Create temporary directory and pipe for the client to communicate through.
#define DIRNAME P_tmpdir "/uC++XXXXXX"
#define PIPENAME "/uC++Pipe"
#define TMPNAME DIRNAME PIPENAME
		static_assert( sizeof(TMPNAME) <= sizeof(((sockaddr_un *)saddr)->sun_path), "" ); // verify name fits

		tmpnm = new char[sizeof(TMPNAME)];				// enough storage for full name
		strcpy( tmpnm, DIRNAME );						// copy in directory name
		if ( mkdtemp( tmpnm ) == nullptr ) {			// make unique directory
			openFailure( errno, tmpnm, 0, uSocket::itoip( 0 ), timeout, AF_UNIX, type, protocol, "mkdtemp failed to create socket temporary directory" );
		} // 
		strcpy( tmpnm + sizeof(DIRNAME) - 1, PIPENAME ); // add pipe name to directory name

		((sockaddr_un *)saddr)->sun_family = AF_UNIX;
		strcpy( ((sockaddr_un *)saddr)->sun_path, tmpnm ); // path to pipe
		saddrlen = SUN_LEN( (sockaddr_un *)saddr );

		for ( ;; ) {
			retcode = ::bind( socket.access.fd, saddr, saddrlen );
		  if ( retcode != -1 || errno != EINTR ) break; // timer interrupt ?
		} // for
		if ( retcode == -1 ) {
			openFailure( errno, name, 0, uSocket::itoip( 0 ), timeout, AF_UNIX, type, protocol, "unable to bind name to socket" );
		} // if

		// Now set default address specified by user.
		strcpy( ((sockaddr_un *)saddr)->sun_path, name );
		saddrlen = SUN_LEN( (sockaddr_un *)saddr );
		baddrlen = sizeof(sockaddr_un);
	} // if

	uDEBUGPRT( uDebugPrt( "(uSocketClient &)%p.createSocketClient1 connection to name:%s\n", this, name ); );
} // uSocketClient::createSocketClient1


void uSocketClient::createSocketClient2( unsigned short port, const char *name, uDuration *timeout, int type, int protocol ) {
	uDEBUGPRT( uDebugPrt( "(uSocketClient &)%p.createSocketClient2 attempting connection to port:%d, name:%s\n", this, port, name ); );

	baddrlen = saddrlen = sizeof(sockaddr_in);

	if ( type != SOCK_DGRAM ) {							// connection-oriented
		connectionOriented( name, port, AF_INET, timeout, type, protocol );
	} // if

	uDEBUGPRT( uDebugPrt( "(uSocketClient &)%p.createSocketClient2 connection to port:%d, name:%s\n", this, port, name ); );
} // uSocketClient::createSocketClient2


void uSocketClient::readFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcat( strcpy( msg, "socket " ), op ), " fails" );
	_Throw uSocketClient::ReadFailure( *this, errno_, buf, len, 0, nullptr, 0, timeout, msg );
} // uSocketClient::readFailure


void uSocketClient::readTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcpy( msg, "timeout during socket " ), op );
	_Throw uSocketClient::ReadTimeout( *this, buf, len, 0, nullptr, 0, timeout, msg );
} // uSocketClient::readTimeout


void uSocketClient::writeFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcat( strcpy( msg, "socket " ), op ), " fails" );
	_Throw uSocketClient::WriteFailure( *this, errno_, buf, len, 0, nullptr, 0, timeout, msg );
} // uSocketClient::writeFailure


void uSocketClient::writeTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcpy( msg, "timeout during socket " ), op );
	_Throw uSocketClient::WriteTimeout( *this, buf, len, 0, nullptr, 0, timeout, msg );
} // uSocketClient::writeTimeout


void uSocketClient::readFailure( int errno_, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcat( strcpy( msg, "socket " ), op ), " fails" );
	_Throw uSocketClient::ReadFailure( *this, errno_, buf, len, flags, from, fromlen, timeout, msg );
} // uSocketClient::readFailure


void uSocketClient::readTimeout( const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcpy( msg, "timeout during socket " ), op );
	_Throw uSocketClient::ReadTimeout( *this, buf, len, flags, from, fromlen, timeout, msg );
} // uSocketClient::readTimeout


void uSocketClient::writeFailure( int errno_, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcat( strcpy( msg, "socket " ), op ), " fails" );
	_Throw uSocketClient::WriteFailure( *this, errno_, buf, len, flags, to, tolen, timeout, msg );
} // uSocketClient::writeFailure


void uSocketClient::writeTimeout( const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const op ) {
	char msg[32];
	strcat( strcpy( msg, "timeout during socket " ), op );
	_Throw uSocketClient::WriteTimeout( *this, buf, len, flags, to, tolen, timeout, msg );
} // uSocketClient::writeTimeout


void uSocketClient::sendfileFailure( int errno_, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout ) {
	_Throw uSocketClient::SendfileFailure( *this, errno_, in_fd, off, len, timeout, "socket sendfile fails" );
} // uSocketClient::sendfileFailure


void uSocketClient::sendfileTimeout( const int in_fd, const off_t *off, const size_t len, const uDuration *timeout ) {
	_Throw uSocketClient::SendfileTimeout( *this, in_fd, off, len, timeout, "timeout during socket sendfile" );
} // uSocketClient::sendfileTimeout


void uSocketClient::openFailure( int errno_, const char *const name, const unsigned short port, const in_addr ip, uDuration *timeout, const int domain, const int type, const int protocol, const char *const msg ) { // prevent expanding _Exception at call site to reduce stack size
	_Throw OpenFailure( *this, errno_, name, port, ip, timeout, domain, type, protocol, msg );
} // uSocketClient::openFailure


void uSocketClient::openTimeout( const char *const name, const unsigned short port, const in_addr ip, uDuration *timeout, const int domain, const int type, const int protocol, const char *const msg ) { // prevent expanding _Exception at call site to reduce stack size
	_Throw OpenTimeout( *this, name, port, ip, timeout, domain, type, protocol, msg );
} // uSocketClient::openTimeout


void uSocketClient::closeTempFile( const char * msg ) {
	delete tmpnm;
	delete saddr;
	if ( ! std::__U_UNCAUGHT_EXCEPTION__() ) _Throw CloseFailure( *this, errno, msg );
} // uSocketClient::closeTempFile


uSocketClient::~uSocketClient() {
	if ( saddr->sa_family == AF_UNIX && socket.type == SOCK_DGRAM ) {
		if ( unlink( tmpnm ) == -1 ) {					// remove temporary communication pipe
			closeTempFile( "unlink failed for temporary pipe" );
		} // if
		tmpnm[sizeof(DIRNAME) - 1] = '\0';				// remove pipe name => just directory name
		if ( rmdir( tmpnm ) == -1 ) {					// remove temporary unique directory
			closeTempFile( "rmdir failed for temporary pipe" );
		} // if
		delete tmpnm;
	} // if
	delete saddr;
} // uSocketClient::~uSocketClient


//######################### uSocketServer (cont) #########################


uSocketServer::Failure::Failure( const uSocketServer &server, int errno_, const char *const msg ) : uSocket::Failure( server.socket, errno_, msg ), server_( server ), fd( server.access.fd ) {
} // uSocketServer::Failure::Failure

const uSocketServer &uSocketServer::Failure::server() const {
	return server_;
} // uSocketServer::Failure::server

int uSocketServer::Failure::fileDescriptor() const {
	return fd;
} // uSocketServer::Failure::fileDescriptor

void uSocketServer::Failure::defaultTerminate() {
	abort( "(uSocketServer &)%p, %.256s.\nError(%d) : %s.",
		   &server(), message(), errNo(), strerror( errNo() ) );
} // uSocketServer::Failure::defaultTerminate


uSocketServer::OpenFailure::OpenFailure( const uSocketServer &server, int errno_, const char *const name, const unsigned short port, const in_addr ip, const int domain, const int type, const int protocol, int backlog, const char *const msg ) :
	uSocketServer::Failure( server, errno_, msg ), port( port ), ip( ip ), domain( domain ), type( type ), protocol( protocol ), backlog( backlog ) {
	uEHM::strncpy( name_, name, uEHMMaxName );			// copy file name
} // uSocketServer::OpenFailure::OpenFailure

const char *uSocketServer::OpenFailure::name() const { return name_; }

void uSocketServer::OpenFailure::defaultTerminate() {
	if ( domain == AF_UNIX ) {
		abort( "(uSocketServer &)%p.uSocketServer( name:\"%s\", cluster:%p, domain:%d, type:%d, protocol:%d, backlog:%d ) : %.256s.\nError(%d) : %s.",
			   &server(), name(), &uThisCluster(), domain, type, protocol, backlog, message(), errNo(), strerror( errNo() ) );
	} else { // AF_INET
		abort( "(uSocketServer &)%p.uSocketServer( port:%d, ip:0x%08x, cluster:%p, domain:%d, type:%d, protocol:%d, backlog:%d ) : %.256s.\nError(%d) : %s.",
			   &server(), port, ip.s_addr, &uThisCluster(), domain, type, protocol, backlog, message(), errNo(), strerror( errNo() ) );
	} // if
} // uSocketServer::OpenFailure::defaultTerminate


uSocketServer::CloseFailure::CloseFailure( const uSocketServer &server, int errno_, int acceptorCnt, const char *const msg ) :
	uSocketServer::Failure( server, errno_, msg ), acceptorCnt( acceptorCnt ) {
} // uSocketServer::CloseFailure::CloseFailure

void uSocketServer::CloseFailure::defaultTerminate() {
	abort( "(uSocketServer &)%p.~uSocketServer(), %.256s, %d acceptor(s) outstanding.",
		   &server(), message(), acceptorCnt );
} // uSocketServer::CloseFailure::defaultTerminate


uSocketServer::ReadFailure::ReadFailure( const uSocketServer &sa, int errno_, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const msg ) :
	uSocketServer::Failure( sa, errno_, msg ), buf( buf ), len( len ), flags( flags ), from( from ), fromlen( fromlen ), timeout( timeout ) {
} // uSocketServer::ReadFailure::ReadFailure

void uSocketServer::ReadFailure::defaultTerminate() {
	abort( "(uSocketServer &)%p.read/recv/recvfrom( buf:%p, len:%d, flags:%d, timeout:%p ) : %.256s for file descriptor %d.\nError(%d) : %s.",
		   &server(), buf, len, flags, timeout, message(), fileDescriptor(), errNo(), strerror( errNo() ) );
} // uSocketServer::ReadFailure::defaultTerminate


uSocketServer::ReadTimeout::ReadTimeout( const uSocketServer &sa, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const msg ) :
	uSocketServer::ReadFailure( sa, ETIMEDOUT, buf, len, flags, from, fromlen, timeout, msg ) {
} // uSocketServer::ReadTimeout::ReadTimeout

void uSocketServer::ReadTimeout::defaultTerminate() {
	abort( "(uSocketServer &)%p.read/recv/recvfrom( buf:%p, len:%d, flags:%d, timeout:%p ) : %.256s for file descriptor %d.",
		   &server(), buf, len, flags, timeout, message(), fileDescriptor() );
} // uSocketServer::ReadTimeout::defaultTerminate


uSocketServer::WriteFailure::WriteFailure( const uSocketServer &sa, int errno_, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const msg ) :
	uSocketServer::Failure( sa, errno_, msg ), buf( buf ), len( len ), flags( flags ), to( to ), tolen( tolen ), timeout( timeout ) {
} // uSocketServer::WriteFailure::WriteFailure

void uSocketServer::WriteFailure::defaultTerminate() {
	abort( "(uSocketServer &)%p.write/send/sendto( buf:%p, len:%d, flags:%d, timeout:%p ) : %.256s for file descriptor %d.\nError(%d) : %s.",
		   &server(), buf, len, flags, timeout, message(), fileDescriptor(), errNo(), strerror( errNo() ) );
} // uSocketServer::WriteFailure::defaultTerminate


uSocketServer::WriteTimeout::WriteTimeout( const uSocketServer &sa, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const msg ) :
	uSocketServer::WriteFailure( sa, ETIMEDOUT, buf, len, flags, to, tolen, timeout, msg ) {
} // uSocketServer::WriteTimeout::WriteTimeout

void uSocketServer::WriteTimeout::defaultTerminate() {
	abort( "(uSocketServer &)%p.write/send/sendto( buf:%p, len:%d, flags:%d, timeout:%p ) : %.256s for file descriptor %d.",
		   &server(), buf, len, flags, timeout, message(), fileDescriptor() );
} // uSocketServer::WriteTimeout::defaultTerminate


uSocketServer::SendfileFailure::SendfileFailure( const uSocketServer &sa, int errno_, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout, const char *const msg ) :
	uSocketServer::Failure( sa, errno_, msg ), in_fd( in_fd ), off( *off ), len( len ), timeout( timeout ) {
} // uSocketServer::SendfileFailure::SendfileFailure

void uSocketServer::SendfileFailure::defaultTerminate() {
	abort( "(uSocketServer &)%p.sendfile( in_fd:%d, off:%ld, len:%lu, timeout:%p ) : %.256s for file descriptor %d.\nError(%d) : %s.",
		   &server(), in_fd, (long int)off, (unsigned long int)len, timeout, message(), fileDescriptor(), errNo(), strerror( errNo() ) );
} // uSocketServer::SendfileFailure::defaultTerminate


uSocketServer::SendfileTimeout::SendfileTimeout( const uSocketServer &sa, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout, const char *const msg ) :
	uSocketServer::SendfileFailure( sa, ETIMEDOUT, in_fd, off, len, timeout, msg ) {
} // uSocketServer::SendfileTimeout::SendfileTimeout

void uSocketServer::SendfileTimeout::defaultTerminate() {
	abort( "(uSocketServer &)%p.sendfile( in_fd:%d, off:%ld, len:%lu, timeout:%p ) : %.256s for file descriptor %d.",
		   &server(), in_fd, (long int)off, (unsigned long int)len, timeout, message(), fileDescriptor() );
} // uSocketServer::SendfileTimeout::defaultTerminate


//######################### uSocketAccept (cont) #########################


uSocketAccept::Failure::Failure( const uSocketAccept &acceptor, int errno_, const char *const msg ) :
	uSocket::Failure( acceptor.sock(), errno_, msg ), acceptor_( acceptor ), socketserver( acceptor.socketserver ), fd( acceptor.access.fd ) {}

const uSocketAccept &uSocketAccept::Failure::acceptor() const {
	return acceptor_;
} // uSocketAccept::Failure::acceptor

const uSocketServer &uSocketAccept::Failure::server() const {
	return socketserver;
} // uSocketAccept::Failure::server

int uSocketAccept::Failure::fileDescriptor() const {
	return fd;
} // uSocketAccept::Failure::fileDescriptor

void uSocketAccept::Failure::defaultTerminate() {
	abort( "(uSocketAccept &)%p( server:%p ), %.256s.", &acceptor(), &server(), message() );
} // uSocketAccept::Failure::defaultTerminate


uSocketAccept::OpenFailure::OpenFailure( const uSocketAccept &acceptor, int errno_, const class uDuration *timeout, const struct sockaddr *adr, const socklen_t *len, const char *const msg ) :
	uSocketAccept::Failure( acceptor, errno_, msg ), timeout( timeout ), adr( adr ), len( len ) {}

void uSocketAccept::OpenFailure::defaultTerminate() {
	abort( "(uSocketAccept &)%p.uSocketAccept( server:%p, timeout:%p, adr:%p, len:%p ), %.256s, error(%d) : %s",
		   &acceptor(), &server(), timeout, adr, len, message(), errNo(), strerror( errNo() ) );
} // uSocketAccept::OpenFailure::defaultTerminate


uSocketAccept::OpenTimeout::OpenTimeout( const uSocketAccept &acceptor, const class uDuration *timeout, const struct sockaddr *adr, const socklen_t *len, const char *const msg ) :
	uSocketAccept::OpenFailure( acceptor, ETIMEDOUT, timeout, adr, len, msg ) {}

void uSocketAccept::OpenTimeout::defaultTerminate() {
	abort( "(uSocketAccept &)%p.uSocketAccept( server:%p, timeout:%p, adr:%p, len:%p ), %.256s",
		   &acceptor(), &server(), timeout, adr, len, message() );
} // uSocketAccept::OpenTimeout::defaultTerminate


uSocketAccept::CloseFailure::CloseFailure( const uSocketAccept &acceptor, int errno_, const char *const msg ) :
	uSocketAccept::Failure( acceptor, errno_, msg ) {}

void uSocketAccept::CloseFailure::defaultTerminate() {
	abort( "(uSocketAccept &)%p.~uSocketAccept(), %.256s.\nError(%d) : %s.",
		   &acceptor(), message(), errNo(), strerror( errNo() ) );
} // uSocketAccept::CloseFailure::defaultTerminate


uSocketAccept::ReadFailure::ReadFailure( const uSocketAccept &sa, int errno_, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const msg ) :
	uSocketAccept::Failure( sa, errno_, msg ), buf( buf ), len( len ), flags( flags ), from( from ), fromlen( fromlen ), timeout( timeout ) {
} // uSocketAccept::ReadFailure::ReadFailure

void uSocketAccept::ReadFailure::defaultTerminate() {
	abort( "(uSocketAccept &)%p.read/recv/recvfrom( buf:%p, len:%d, flags:%d, timeout:%p ) : %.256s for file descriptor %d.\nError(%d) : %s.",
		   &acceptor(), buf, len, flags, timeout, message(), fileDescriptor(), errNo(), strerror( errNo() ) );
} // uSocketAccept::ReadFailure::defaultTerminate


uSocketAccept::ReadTimeout::ReadTimeout( const uSocketAccept &sa, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const msg ) :
	uSocketAccept::ReadFailure( sa, ETIMEDOUT, buf, len, flags, from, fromlen, timeout, msg ) {
} // uSocketAccept::ReadTimeout::ReadTimeout

void uSocketAccept::ReadTimeout::defaultTerminate() {
	abort( "(uSocketAccept &)%p.read/recv/recvfrom( buf:%p, len:%d, flags:%d, timeout:%p ) : %.256s for file descriptor %d.",
		   &acceptor(), buf, len, flags, timeout, message(), fileDescriptor() );
} // uSocketAccept::ReadTimeout::defaultTerminate


uSocketAccept::WriteFailure::WriteFailure( const uSocketAccept &sa, int errno_, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const msg ) :
	uSocketAccept::Failure( sa, errno_, msg ), buf( buf ), len( len ), flags( flags ), to( to ), tolen( tolen ), timeout( timeout ) {
} // uSocketAccept::WriteFailure::WriteFailure

void uSocketAccept::WriteFailure::defaultTerminate() {
	abort( "(uSocketAccept &)%p.write/send/sendto( buf:%p, len:%d, flags:%d, timeout:%p ) : %.256s for file descriptor %d.\nError(%d) : %s.",
		   &acceptor(), buf, len, flags, timeout, message(), fileDescriptor(), errNo(), strerror( errNo() ) );
} // uSocketAccept::WriteFailure::defaultTerminate


uSocketAccept::WriteTimeout::WriteTimeout( const uSocketAccept &sa, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const msg ) :
	uSocketAccept::WriteFailure( sa, ETIMEDOUT, buf, len, flags, to, tolen, timeout, msg ) {
} // uSocketAccept::WriteTimeout::WriteTimeout

void uSocketAccept::WriteTimeout::defaultTerminate() {
	abort( "(uSocketAccept &)%p.write/send/sendto( buf:%p, len:%d, flags:%d, timeout:%p ) : %.256s for file descriptor %d.",
		   &acceptor(), buf, len, flags, timeout, message(), fileDescriptor() );
} // uSocketAccept::WriteTimeout::defaultTerminate


uSocketAccept::SendfileFailure::SendfileFailure( const uSocketAccept &sa, int errno_, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout, const char *const msg ) :
	uSocketAccept::Failure( sa, errno_, msg ), in_fd( in_fd ), off( *off ), len( len ), timeout( timeout ) {
} // uSocketAccept::SendfileFailure::SendfileFailure

void uSocketAccept::SendfileFailure::defaultTerminate() {
	abort( "(uSocketAccept &)%p.sendfile( in_fd:%d, off:%ld, len:%lu, timeout:%p ) : %.256s for file descriptor %d.\nError(%d) : %s.",
		   &acceptor(), in_fd, (long int)off, (unsigned long int)len, timeout, message(), fileDescriptor(), errNo(), strerror( errNo() ) );
} // uSocketAccept::SendfileFailure::defaultTerminate


uSocketAccept::SendfileTimeout::SendfileTimeout( const uSocketAccept &sa, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout, const char *const msg ) :
	uSocketAccept::SendfileFailure( sa, ETIMEDOUT, in_fd, off, len, timeout, msg ) {
} // uSocketAccept::SendfileTimeout::SendfileTimeout

void uSocketAccept::SendfileTimeout::defaultTerminate() {
	abort( "(uSocketAccept &)%p.sendfile( in_fd:%d, off:%ld, len:%lu, timeout:%p ) : %.256s for file descriptor %d.",
		   &acceptor(), in_fd, (long int)off, (unsigned long int)len, timeout, message(), fileDescriptor() );
} // uSocketAccept::SendfileTimeout::defaultTerminate


//######################### uSocketClient (cont) #########################


uSocketClient::Failure::Failure( const uSocketClient &client, int errno_, const char *const msg ) : uSocket::Failure( client.socket, errno_, msg ), client_( client ), fd( client.access.fd ) {
} // uSocketClient::Failure::Failure

const uSocketClient &uSocketClient::Failure::client() const {
	return client_;
} // uSocketClient::Failure::client

int uSocketClient::Failure::fileDescriptor() const {
	return fd;
} // uSocketClient::Failure::fileDescriptor

void uSocketClient::Failure::defaultTerminate() {
	abort( "(uSocketClient &)%p, %.256s.",
		   &client(), message() );
} // uSocketClient::Failure::defaultTerminate


uSocketClient::OpenFailure::OpenFailure( const uSocketClient &client, int errno_, const char *name, const unsigned short port, const in_addr ip, uDuration *timeout, int domain, int type, int protocol, const char *const msg ) :
	uSocketClient::Failure( client, errno_, msg ), port( port ), ip( ip ), timeout( timeout ), domain( domain ), type( type ), protocol( protocol ) {
	uEHM::strncpy( name_, name, uEHMMaxName );			// copy file name
} // uSocketClient::OpenFailure::OpenFailure

const char *uSocketClient::OpenFailure::name() const { return name_; }

void uSocketClient::OpenFailure::defaultTerminate() {
	if ( domain == AF_UNIX ) {
		abort( "(uSocketClient &)%p.uSocketClient( name:\"%s\", cluster:%p, timeout:%p, domain:%d, type:%d, protocol:%d ) : %.256s.\nError(%d) : %s.",
			   &client(), name(), &uThisCluster(), timeout, domain, type, protocol, message(), errNo(), strerror( errNo() ) );
	} else { // AF_INET
		abort( "(uSocketClient &)%p.uSocketClient( port:%d, ip:0x%08x, cluster:%p, timeout:%p, domain:%d, type:%d, protocol:%d ) : %.256s.\nError(%d) : %s.",
			   &client(), port, ip.s_addr, &uThisCluster(), timeout, domain, type, protocol, message(), errNo(), strerror( errNo() ) );
	} // if
} // uSocketClient::OpenFailure::defaultTerminate


uSocketClient::OpenTimeout::OpenTimeout( const uSocketClient &client, const char *const name, const unsigned short port, const in_addr ip, uDuration *timeout, const int domain, const int type, const int protocol, const char *const msg ) :
	uSocketClient::OpenFailure( client, ETIMEDOUT, name, port, ip, timeout, domain, type, protocol, msg ) {
} // uSocketClient::OpenTimeout::OpenTimeout

void uSocketClient::OpenTimeout::defaultTerminate() {
	if ( domain == AF_UNIX ) {
		abort( "(uSocketClient &)%p.uSocketClient( name:\"%s\", cluster:%p, domain:%d, type:%d, protocol:%d ) : %.256s.",
			   &client(), name(), &uThisCluster(), domain, type, protocol, message() );
	} else { // AF_INET
		abort( "(uSocketClient &)%p.uSocketClient( port:%d, name:\"%s\", cluster:%p, timeout:%p, domain:%d, type:%d, protocol:%d ) : %.256s.",
			   &client(), port, name(), &uThisCluster(), timeout, domain, type, protocol, message() );
	} // if
} // uSocketClient::OpenTimeout::defaultTerminate


uSocketClient::CloseFailure::CloseFailure( const uSocketClient &client, int errno_, const char *const msg ) :
	uSocketClient::Failure( client, errno_, msg ) {}

void uSocketClient::CloseFailure::defaultTerminate() {
	abort( "(uSocketClient &)%p.~uSocketClient(), %.256s.\nError(%d) : %s.",
		   &client(), message(), errNo(), strerror( errNo() ) );
} // uSocketClient::CloseFailure::defaultTerminate


uSocketClient::ReadFailure::ReadFailure( const uSocketClient &sa, int errno_, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const msg ) :
	uSocketClient::Failure( sa, errno_, msg ), buf( buf ), len( len ), flags( flags ), from( from ), fromlen( fromlen ), timeout( timeout ) {
} // uSocketClient::ReadFailure::ReadFailure

void uSocketClient::ReadFailure::defaultTerminate() {
	abort( "(uSocketClient &)%p.read/recv/recvfrom( buf:%p, len:%d, flags:%d, timeout:%p ) : %.256s for file descriptor %d.\nError(%d) : %s.",
		   &client(), buf, len, flags, timeout, message(), fileDescriptor(), errNo(), strerror( errNo() ) );
} // uSocketClient::ReadFailure::defaultTerminate


uSocketClient::ReadTimeout::ReadTimeout( const uSocketClient &sa, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const msg ) :
	uSocketClient::ReadFailure( sa, ETIMEDOUT, buf, len, flags, from, fromlen, timeout, msg ) {
} // uSocketClient::ReadTimeout::ReadTimeout

void uSocketClient::ReadTimeout::defaultTerminate() {
	abort( "(uSocketClient &)%p.read/recv/recvfrom( buf:%p, len:%d, flags:%d, timeout:%p ) : %.256s for file descriptor %d.",
		   &client(), buf, len, flags, timeout, message(), fileDescriptor() );
} // uSocketClient::ReadTimeout::defaultTerminate


uSocketClient::WriteFailure::WriteFailure( const uSocketClient &sa, int errno_, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const msg ) :
	uSocketClient::Failure( sa, errno_, msg ), buf( buf ), len( len ), flags( flags ), to( to ), tolen( tolen ), timeout( timeout ) {
} // uSocketClient::WriteFailure::WriteFailure

void uSocketClient::WriteFailure::defaultTerminate() {
	abort( "(uSocketClient &)%p.write/send/sendto( buf:%p, len:%d, flags:%d, timeout:%p ) : %.256s for file descriptor %d.\nError(%d) : %s.",
		   &client(), buf, len, flags, timeout, message(), fileDescriptor(), errNo(), strerror( errNo() ) );
} // uSocketClient::WriteFailure::defaultTerminate


uSocketClient::WriteTimeout::WriteTimeout( const uSocketClient &sa, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const msg ) :
	uSocketClient::WriteFailure( sa, ETIMEDOUT, buf, len, flags, to, tolen, timeout, msg ) {
} // uSocketClient::WriteTimeout::WriteTimeout

void uSocketClient::WriteTimeout::defaultTerminate() {
	abort( "(uSocketClient &)%p.write/send/sendto( buf:%p, len:%d, flags:%d, timeout:%p ) : %.256s for file descriptor %d.",
		   &client(), buf, len, flags, timeout, message(), fileDescriptor() );
} // uSocketClient::WriteTimeout::defaultTerminate


uSocketClient::SendfileFailure::SendfileFailure( const uSocketClient &sa, int errno_, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout, const char *const msg ) :
	uSocketClient::Failure( sa, errno_, msg ), in_fd( in_fd ), off( *off ), len( len ), timeout( timeout ) {
} // uSocketClient::SendfileFailure::SendfileFailure

void uSocketClient::SendfileFailure::defaultTerminate() {
	abort( "(uSocketClient &)%p.sendfile( in_fd:%d, off:%ld, len:%lu, timeout:%p ) : %.256s for file descriptor %d.\nError(%d) : %s.",
		   &client(), in_fd, (long int)off, (unsigned long int)len, timeout, message(), fileDescriptor(), errNo(), strerror( errNo() ) );
} // uSocketClient::SendfileFailure::defaultTerminate


uSocketClient::SendfileTimeout::SendfileTimeout( const uSocketClient &sa, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout, const char *const msg ) :
	uSocketClient::SendfileFailure( sa, ETIMEDOUT, in_fd, off, len, timeout, msg ) {
} // uSocketClient::SendfileTimeout::SendfileTimeout

void uSocketClient::SendfileTimeout::defaultTerminate() {
	abort( "(uSocketClient &)%p.sendfile( in_fd:%d, off:%ld, len:%lu, timeout:%p ) : %.256s for file descriptor %d.",
		   &client(), in_fd, (long int)off, (unsigned long int)len, timeout, message(), fileDescriptor() );
} // uSocketClient::SendfileTimeout::defaultTerminate


// Local Variables: //
// compile-command: "make install" //
// End: //
