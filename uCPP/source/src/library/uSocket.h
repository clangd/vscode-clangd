//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uSocket.h -- Nonblocking UNIX Socket I/O
// 
// Author           : Peter A. Buhr
// Created On       : Tue Mar 29 17:04:36 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Oct  7 07:56:14 2023
// Update Count     : 397
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


#include <uFile.h>
//#include <uDebug.h>


#include <cstdio>
#include <netdb.h>
#include <sys/param.h>									// MAXHOSTNAMELEN
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>


//######################### uSocket #########################


class uSocket {
	friend class uSocketServer;							// access: access, uSocket, ~uSocket
	friend class uSocketAccept;							// access: access
	friend class uSocketClient;							// access: access, uSocket, ~uSocket

	uIOaccess access;

	const int domain, type, protocol;					// copies for information purposes

	uSocket( const uSocket & ) = delete;				// no copy
	uSocket( uSocket && ) = delete;
	uSocket & operator=( const uSocket & ) = delete;	// no assignment
	uSocket & operator=( uSocket && ) = delete;

	static void convertFailure( const char *msg, const char *addr ) __attribute__ ((noreturn));

	void createSocket( int domain, int type, int protocol );

	uSocket( int domain, int type, int protocol ) : domain( domain ), type( type ), protocol( protocol ) {
		createSocket( domain, type, protocol );
	} // uSocket::uSocket

	virtual ~uSocket();
  public:
	static in_addr gethostbyname( const char *name );
	static in_addr gethostbyip( const char *ip );

	static in_addr itoip( in_addr_t ip ) {
		in_addr host;
		host.s_addr = ip;
		return host;
	} // uSocket::itoip

	_Exception IPConvertFailure : public uIOFailure {
		const char *kind, *text;
	  public:
		IPConvertFailure( const char *kind, const char *text );
		void defaultTerminate() override;
	}; // uSocket::IPConvertFailure

	_Exception Failure : public uIOFailure {
		const uSocket &socket_;
	  protected:
		Failure( const uSocket &s, int errno_, const char *const msg );
	  public:
		const uSocket &socket() const;
		virtual void defaultTerminate() override;
	}; // uSocket::Failure

	friend _Exception Failure;							// access: access

	_Exception OpenFailure : public uSocket::Failure {
	  protected:
		const int domain;
		const int type;
		const int protocol;
	  public:
		OpenFailure( const uSocket &socket, int errno_, const int domain, const int type, const int protocol, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocket::OpenFailure

	_Exception CloseFailure : public uSocket::Failure {
	  public:
		CloseFailure( const uSocket &socket, int errno_, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocket::CloseFailure
	friend _Exception CloseFailure;						// access: access
}; // uSocket


//######################### uSocketIO #########################


class uSocketIO : public uFileIO {
  protected:
	struct inetAddr : sockaddr_in {
		inetAddr( unsigned short port, in_addr ip ) {
			sin_family = AF_INET;
			sin_port = htons( port );
			sin_addr = ip;
			memset( &sin_zero, '\0', sizeof(sin_zero) );
			uDEBUGPRT( uDebugPrt( "(uSocketIO &)%p::inetAddr::inetAddr, sin_family:%d sin_port:%d (0x%x), sin_addr:%8x\n", this, sin_family, sin_port, sin_port, sin_addr.s_addr ); )
				} // uSocketIO::inetAddr::inetAddr
	}; // inetAddr

	struct sockaddr *saddr;								// default send/receive address
	socklen_t saddrlen;									// size of send address
	socklen_t baddrlen;									// size of address buffer (UNIX/INET)

	using uFileIO::readFailure;
	using uFileIO::readTimeout;
	using uFileIO::writeFailure;
	using uFileIO::writeTimeout;

	virtual void readFailure( int errno_, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const op ) = 0;
	virtual void readTimeout( const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const op ) = 0;
	virtual void writeFailure( int errno_, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const op ) = 0;
	virtual void writeTimeout( const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const op ) = 0;
	virtual void sendfileFailure( int errno_, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout ) = 0;
	virtual void sendfileTimeout( const int in_fd, const off_t *off, const size_t len, const uDuration *timeout ) = 0;

	uSocketIO( uIOaccess &acc, struct sockaddr *saddr ) : uFileIO( acc ), saddr( saddr ) {
	} // uSocketIO::uSocketIO
  public:
	_Mutex const struct sockaddr *getsockaddr() {		// must cast result to sockaddr_in or sockaddr_un
		return saddr;
	} // uSocketIO::getsockname

	_Mutex int getsockname( struct sockaddr *name, socklen_t *len ) {
		return ::getsockname( access.fd, name, len );
	} // uSocketIO::getsockname

	_Mutex int getpeername( struct sockaddr *name, socklen_t *len ) {
		return ::getpeername( access.fd, name, len );
	} // uSocketIO::getpeername

	int send( char *buf, int len, int flags = 0, uDuration *timeout = nullptr );
	int sendto( char *buf, int len, struct sockaddr *to, socklen_t tolen, int flags = 0, uDuration *timeout = nullptr );

	int sendto( char *buf, int len, int flags = 0, uDuration *timeout = nullptr ) {
		return sendto( buf, len, saddr, saddrlen, flags, timeout );
	} // uSocketIO::sendto

	int sendmsg( const struct msghdr *msg, int flags = 0, uDuration *timeout = nullptr );
	int recv( char *buf, int len, int flags = 0, uDuration *timeout = nullptr );
	int recvfrom( char *buf, int len, struct sockaddr *from, socklen_t *fromlen, int flags = 0, uDuration *timeout = nullptr );

	int recvfrom( char *buf, int len, int flags = 0, uDuration *timeout = nullptr ) {
		saddrlen = baddrlen;				// set to receive buffer size
		return recvfrom( buf, len, saddr, &saddrlen, flags, timeout );
	} // uSocketIO::recvfrom

	int recvmsg( struct msghdr *msg, int flags = 0, uDuration *timeout = nullptr );

	ssize_t sendfile( uFile::FileAccess &file, off_t *off, size_t len, uDuration *timeout = nullptr );
}; // uSocketIO


//######################### uSocketServer #########################


_Monitor uSocketServer : public uSocketIO {
	friend class uSocketAccept;							// access: socket, acceptor, unacceptor, ~uSocketAccept

	int acceptorCnt;									// number of simultaneous acceptors using server
	uSocket socket;										// one-to-one correspondance between server and socket

	void acceptor() {
		uFetchAdd( acceptorCnt, 1 );
	} // uSocketServer::acceptor

	void unacceptor() {
		uFetchAdd( acceptorCnt, -1 );
	} // uSocketServer::unacceptor

	void createSocketServer1( const char *name, int type, int protocol, int backlog );
	void createSocketServer2( unsigned short port, int type, int protocol, int backlog );
	void createSocketServer3( unsigned short *port, int type, int protocol, int backlog );
  protected:
	void readFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void readTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void writeFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void writeTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));

	void readFailure( int errno_, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void readTimeout( const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void writeFailure( int errno_, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void writeTimeout( const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void sendfileFailure( int errno_, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout ) __attribute__ ((noreturn));
	void sendfileTimeout( const int in_fd, const off_t *off, const size_t len, const uDuration *timeout ) __attribute__ ((noreturn));
	void openFailure( int errno_, const char *const name, const unsigned short port, const in_addr ip, int domain, int type, int protocol, int backlog, const char *const msg ) __attribute__ ((noreturn));
  public:
	_Exception Failure : public uSocket::Failure {
		const uSocketServer &server_;					// do not dereference
		const int fd;
	  protected:
		Failure( const uSocketServer &s, int errno_, const char *const msg );
	  public:
		const uSocketServer &server() const;
		int fileDescriptor() const;
		virtual void defaultTerminate() override;
	}; // uSocketServer::Failure
	friend _Exception Failure;							// access: socket

	_Exception OpenFailure : public uSocketServer::Failure {
	  protected:
		char name_[uEHMMaxName];
		const unsigned short port;
		const in_addr ip;
		const int domain;
		const int type;
		const int protocol;
		const int backlog;
	  public:
		OpenFailure( const uSocketServer &server, int errno_, const char *const name, const unsigned short port, const in_addr ip, const int domain, const int type, const int protocol, int backlog, const char *const msg );
		const char *name() const;						// return socket name, or null when hostname is used
		virtual void defaultTerminate() override;
	}; // uSocketServer::OpenFailure

	_Exception CloseFailure : public uSocketServer::Failure {
		const int acceptorCnt;
	  public:
		CloseFailure( const uSocketServer &server, int errno_, const int acceptorCnt, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketServer::CloseFailure

	_Exception ReadFailure : public uSocketServer::Failure {
	  protected:
		const char *buf;
		const int len;
		const int flags;
		const struct sockaddr *from;
		const socklen_t *fromlen;
		const uDuration *timeout;
	  public:
		ReadFailure( const uSocketServer &sa, int errno_, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketServer::ReadFailure

	_Exception ReadTimeout : public uSocketServer::ReadFailure {
	  public:
		ReadTimeout( const uSocketServer &sa, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketServer::ReadTimeout

	_Exception WriteFailure : public uSocketServer::Failure {
	  protected:
		const char *buf;
		const int len;
		const int flags;
		const struct sockaddr *to;
		const int tolen;
		const uDuration *timeout;
	  public:
		WriteFailure( const uSocketServer &sa, int errno_, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketServer::WriteFailure

	_Exception WriteTimeout : public uSocketServer::WriteFailure {
	  public:
		WriteTimeout( const uSocketServer &sa, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketServer::WriteTimeout

	_Exception SendfileFailure : public Failure {
	  protected:
		const int in_fd;
		const off_t off;
	    const size_t len;
		const uDuration *timeout;
	  public:
		SendfileFailure( const uSocketServer &sa, int errno_, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketServer::SendfileFailure

	_Exception SendfileTimeout : public SendfileFailure {
	  public:
		SendfileTimeout( const uSocketServer &sa, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketServer::SendfileTimeout


	// AF_UNIX
	uSocketServer( const char *name, int type = SOCK_STREAM, int protocol = 0, int backlog = 10 ) :
	    uSocketIO( socket.access, (sockaddr *)new sockaddr_un ), socket( AF_UNIX, type, protocol ) {
		createSocketServer1( name, type, protocol, backlog );
	} // uSocketServer::uSocketServer

	// AF_INET, local host
	uSocketServer( unsigned short port, int type = SOCK_STREAM, int protocol = 0, int backlog = 10 ) :
	    uSocketIO( socket.access, (sockaddr *)new inetAddr( port, uSocket::itoip( INADDR_ANY ) ) ), socket( AF_INET, type, protocol ) {
		createSocketServer2( port, type, protocol, backlog );
	} // uSocketServer::uSocketServer

	uSocketServer( unsigned short port, in_addr ip, int type = SOCK_STREAM, int protocol = 0, int backlog = 10 ) :
	    uSocketIO( socket.access, (sockaddr *)new inetAddr( port, ip ) ), socket( AF_INET, type, protocol ) {
		createSocketServer2( port, type, protocol, backlog );
	} // uSocketServer::uSocketServer

	uSocketServer( unsigned short *port, int type = SOCK_STREAM, int protocol = 0, int backlog = 10 ) :
	    uSocketIO( socket.access, (sockaddr *)new inetAddr( 0, uSocket::itoip( INADDR_ANY ) ) ), socket( AF_INET, type, protocol ) {
		createSocketServer3( port, type, protocol, backlog );
	} // uSocketServer::uSocketServer

	uSocketServer( unsigned short *port, in_addr ip, int type = SOCK_STREAM, int protocol = 0, int backlog = 10 ) :
	    uSocketIO( socket.access, (sockaddr *)new inetAddr( 0, ip ) ), socket( AF_INET, type, protocol ) {
		createSocketServer3( port, type, protocol, backlog );
	} // uSocketServer::uSocketServer

	virtual ~uSocketServer() {
		if ( acceptorCnt != 0 ) {
			if ( ! std::__U_UNCAUGHT_EXCEPTION__() ) _Throw CloseFailure( *this, EINVAL, acceptorCnt, "closing socket server with outstanding acceptor(s)" );
		} // if
		delete saddr;
	} // uSocketServer::~uSocketServer

	void setClient( struct sockaddr *addr, socklen_t len ) {
#ifdef __U_DEBUG__
		if ( len > baddrlen ) {
			abort( "(uSocketServer &)%p.setClient( addr:%p, len:%d ) : New server address to large.", this, addr, len );
		} // if
#endif // __U_DEBUG__
		memcpy( saddr, addr, len );
	} // uSocketServer::setClient

	void getClient( struct sockaddr *addr, socklen_t *len ) {
#ifdef __U_DEBUG__
		if ( *len < baddrlen ) {
			abort( "(uSocketServer &)%p.getClient( addr:%p, len:%d ) : Area for server address to small.", this, addr, *len );
		} // if
#endif // __U_DEBUG__
		*len = baddrlen;
		memcpy( addr, saddr, baddrlen );
	} // uSocketServer::getServer
}; // uSocketServer


//######################### uSocketAccept #########################


_Monitor uSocketAccept : public uSocketIO {
	uSocketServer &socketserver;						// many-to-one correspondence among acceptors and server
	uIOaccess access;
	bool openAccept;									// current open accept for socket ?
	uDuration *timeout;
	struct sockaddr *adr;
	socklen_t *len;

	const uSocket &sock() const {
		return socketserver.socket;
	} // uSocketAccept::sock

	void createSocketAcceptor( uDuration *timeout, struct sockaddr *adr, socklen_t *len );
	int close_();
  protected:
	void readFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void readTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void writeFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void writeTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));

	void readFailure( int errno_, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void readTimeout( const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void writeFailure( int errno_, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void writeTimeout( const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void sendfileFailure( int errno_, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout ) __attribute__ ((noreturn));
	void sendfileTimeout( const int in_fd, const off_t *off, const size_t len, const uDuration *timeout ) __attribute__ ((noreturn));
	void openFailure( int errno_, const uDuration *timeout, const struct sockaddr *adr, const socklen_t *len ) __attribute__ ((noreturn));
	void openTimeout( const uDuration *timeout, const struct sockaddr *adr, const socklen_t *len ) __attribute__ ((noreturn));
  public:
	_Exception Failure : public uSocket::Failure {
		const uSocketAccept &acceptor_;					// do not dereference
		const uSocketServer &socketserver;
		const int fd;
	  protected:
		Failure( const uSocketAccept &acceptor, int errno_, const char *const msg );
	  public:
		const uSocketAccept &acceptor() const;
		const uSocketServer &server() const;
		int fileDescriptor() const;
		virtual void defaultTerminate() override;
	}; // uSocketAccept::Failure

	_Exception OpenFailure : public Failure {
	  protected:
		const uDuration *timeout;
		const struct sockaddr *adr;
		const socklen_t *len;
	  public:
		OpenFailure( const uSocketAccept &acceptor, int errno_, const uDuration *timeout, const struct sockaddr *adr, const socklen_t *len, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketAccept::OpenFailure

	_Exception OpenTimeout : public OpenFailure {
	  public:
		OpenTimeout( const uSocketAccept &acceptor, const uDuration *timeout, const struct sockaddr *adr, const socklen_t *len, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketAccept::OpenTimeout

	_Exception CloseFailure : public Failure {
	  public:
		CloseFailure( const uSocketAccept &acceptor, int errno_, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketAccept::CloseFailure

	_Exception ReadFailure : public Failure {
	  protected:
		const char *buf;
		const int len;
		const int flags;
		const struct sockaddr *from;
		const socklen_t *fromlen;
		const uDuration *timeout;
	  public:
		ReadFailure( const uSocketAccept &sa, int errno_, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketAccept::ReadFailure

	_Exception ReadTimeout : public ReadFailure {
	  public:
		ReadTimeout( const uSocketAccept &sa, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketAccept::ReadTimeout

	_Exception WriteFailure : public Failure {
	  protected:
		const char *buf;
		const int len;
		const int flags;
		const struct sockaddr *to;
		const int tolen;
		const uDuration *timeout;
	  public:
		WriteFailure( const uSocketAccept &sa, int errno_, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketAccept::WriteFailure

	_Exception WriteTimeout : public WriteFailure {
	  public:
		WriteTimeout( const uSocketAccept &sa, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketAccept::WriteTimeout

	_Exception SendfileFailure : public Failure {
	  protected:
		const int in_fd;
		const off_t off;
	    const size_t len;
		const uDuration *timeout;
	  public:
		SendfileFailure( const uSocketAccept &sa, int errno_, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketAccept::SendfileFailure

	_Exception SendfileTimeout : public SendfileFailure {
	  public:
		SendfileTimeout( const uSocketAccept &sa, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketAccept::SendfileTimeout


	uSocketAccept( uSocketServer &s, struct sockaddr *adr = nullptr, socklen_t *len = nullptr ) :
	    uSocketIO( access, s.saddr ), socketserver( s ), openAccept( false ), timeout( nullptr ), adr( adr ), len( len ) {
		createSocketAcceptor( nullptr, adr, len );
		socketserver.acceptor();
	} // uSocketAccept::uSocketAccept

	uSocketAccept( uSocketServer &s, uDuration *timeout, struct sockaddr *adr = nullptr, socklen_t *len = nullptr ) :
	    uSocketIO( access, s.saddr ), socketserver( s ), openAccept( false ), timeout( timeout ), adr( adr ), len( len ) {
		createSocketAcceptor( timeout, adr, len );
		socketserver.acceptor();
	} // uSocketAccept::uSocketAccept

	uSocketAccept( uSocketServer &s, bool doAccept, struct sockaddr *adr = nullptr, socklen_t *len = nullptr ) :
	    uSocketIO( access, s.saddr ), socketserver( s ), openAccept( false ), timeout( nullptr ), adr( adr ), len( len ) {
		if ( doAccept ) {
			createSocketAcceptor( nullptr, adr, len );
		} // if
		socketserver.acceptor();
	} // uSocketAccept::uSocketAccept

	uSocketAccept( uSocketServer &s, uDuration *timeout, bool doAccept, struct sockaddr *adr = nullptr, socklen_t *len = nullptr ) :
	    uSocketIO( access, s.saddr ), socketserver( s ), openAccept( false ), timeout( timeout ), adr( adr ), len( len ) {
		if ( doAccept ) {
			createSocketAcceptor( timeout, adr, len );
		} // if
		socketserver.acceptor();
	} // uSocketAccept::uSocketAccept

	virtual ~uSocketAccept() {
		socketserver.unacceptor();
		close();
	} // uSocketAccept::~uSocketAccept

	void accept( uDuration *timeout ) {
		close();
		uSocketAccept::timeout = timeout;
		createSocketAcceptor( timeout, adr, len );
	} // uSocketAccept::accept

	void accept() {
		accept( timeout );								// use global timeout
	} // uSocketAccept::accept

	void close();
}; // uSocketAccept


//######################### uSocketClient #########################


_Monitor uSocketClient : public uSocketIO {
	uSocket socket;										// one-to-one correspondence between client and socket
	char *tmpnm;										// temporary file for communicate with UNIX datagram

	void connectionOriented( const char *name, unsigned short port, const int domain, uDuration *timeout, int type, int protocol );
	void createSocketClient1( const char *name, uDuration *timeout, int type, int protocol );
	void createSocketClient2( unsigned short port, const char *name, uDuration *timeout, int type, int protocol );
  protected:
	void readFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void readTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void writeFailure( int errno_, const char *buf, const int len, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void writeTimeout( const char *buf, const int len, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));

	void readTimeout( const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void readFailure( int errno_, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void writeFailure( int errno_, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void writeTimeout( const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const op ) __attribute__ ((noreturn));
	void sendfileFailure( int errno_, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout ) __attribute__ ((noreturn));
	void sendfileTimeout( const int in_fd, const off_t *off, const size_t len, const uDuration *timeout ) __attribute__ ((noreturn));
	void openFailure( int errno_, const char *const name, const unsigned short port, const in_addr ip, uDuration *timeout, const int domain, const int type, const int protocol, const char *const msg ) __attribute__ ((noreturn));
	void openTimeout( const char *const name, const unsigned short port, const in_addr ip, uDuration *timeout, const int domain, const int type, const int protocol, const char *const msg ) __attribute__ ((noreturn));
	void closeTempFile( const char * msg );
  public:
	_Exception Failure : public uSocket::Failure {
		const uSocketClient &client_;					// do not dereference
		const int fd;
	  protected:
		Failure( const uSocketClient &client, int errno_, const char *const msg );
	  public:
		const uSocketClient &client() const;
		int fileDescriptor() const;
		virtual void defaultTerminate() override;
	}; // uSocketClient::Failure

	_Exception OpenFailure : public Failure {
	  protected:
		char name_[uEHMMaxName];
		unsigned short port;
		const in_addr ip;
		const uDuration *timeout;
		const int domain;
		const int type;
		const int protocol;
	  public:
		OpenFailure( const uSocketClient &client, int errno_, const char *name, const unsigned short port, const in_addr ip, uDuration *timeout, int domain, int type, int protocol, const char *const msg );
		const char *name() const;
		virtual void defaultTerminate() override;
	}; // uSocketClient::OpenFailure

	_Exception OpenTimeout : public OpenFailure {
	  public:
		OpenTimeout( const uSocketClient &client, const char *const name, const unsigned short port, const in_addr ip, uDuration *timeout, const int domain, const int type, const int protocol, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketClient::OpenTimeout

	_Exception CloseFailure : public Failure {
	  public:
		CloseFailure( const uSocketClient &acceptor, int errno_, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketClient::CloseFailure

	_Exception ReadFailure : public Failure {
	  protected:
		const char *buf;
		const int len;
		const int flags;
		const struct sockaddr *from;
		const socklen_t *fromlen;
		const uDuration *timeout;
	  public:
		ReadFailure( const uSocketClient &sa, int errno_, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketClient::ReadFailure

	_Exception ReadTimeout : public ReadFailure {
	  public:
		ReadTimeout( const uSocketClient &sa, const char *buf, const int len, const int flags, const struct sockaddr *from, const socklen_t *fromlen, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketClient::ReadTimeout

	_Exception WriteFailure : public Failure {
	  protected:
		const char *buf;
		const int len;
		const int flags;
		const struct sockaddr *to;
		const int tolen;
		const uDuration *timeout;
	  public:
		WriteFailure( const uSocketClient &sa, int errno_, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketClient::WriteFailure

	_Exception WriteTimeout : public WriteFailure {
	  public:
		WriteTimeout( const uSocketClient &sa, const char *buf, const int len, const int flags, const struct sockaddr *to, const int tolen, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketClient::WriteTimeout

	_Exception SendfileFailure : public Failure {
	  protected:
		const int in_fd;
		const off_t off;
	    const size_t len;
		const uDuration *timeout;
	  public:
		SendfileFailure( const uSocketClient &sa, int errno_, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketClient::SendfileFailure

	_Exception SendfileTimeout : public SendfileFailure {
	  public:
		SendfileTimeout( const uSocketClient &sa, const int in_fd, const off_t *off, const size_t len, const uDuration *timeout, const char *const msg );
		virtual void defaultTerminate() override;
	}; // uSocketClient::SendfileTimeout

	// AF_UNIX
	uSocketClient( const char *name, int type = SOCK_STREAM, int protocol = 0 ) :
	    uSocketIO( socket.access, (sockaddr *)new sockaddr_un ), socket( AF_UNIX, type, protocol ) {
		createSocketClient1( name, nullptr, type, protocol );
	} // uSocketClient::uSocketClient

	uSocketClient( const char *name, uDuration *timeout, int type = SOCK_STREAM, int protocol = 0 ) :
	    uSocketIO( socket.access, (sockaddr *)new sockaddr_un ), socket( AF_UNIX, type, protocol ) {
		createSocketClient1( name, timeout, type, protocol );
	} // uSocketClient::uSocketClient

	// AF_INET, local host
	uSocketClient( unsigned short port, int type = SOCK_STREAM, int protocol = 0 ) :
	    uSocketIO( socket.access, (sockaddr *)new inetAddr( port, uSocket::itoip( INADDR_ANY ) ) ), socket( AF_INET, type, protocol ) {
		createSocketClient2( port, "", nullptr, type, protocol );
	} // uSocketClient::uSocketClient

	uSocketClient( unsigned short port, uDuration *timeout, int type = SOCK_STREAM, int protocol = 0 ) :
	    uSocketIO( socket.access, (sockaddr *)new inetAddr( port, uSocket::itoip( INADDR_ANY ) ) ), socket( AF_INET, type, protocol ) {
		createSocketClient2( port, "", timeout, type, protocol );
	} // uSocketClient::uSocketClient

	// AF_INET, other host
	uSocketClient( unsigned short port, in_addr ip, int type = SOCK_STREAM, int protocol = 0 ) :
	    uSocketIO( socket.access, (sockaddr *)new inetAddr( port, ip ) ), socket( AF_INET, type, protocol ) {
		createSocketClient2( port, "", nullptr, type, protocol );
	} // uSocketClient::uSocketClient

	uSocketClient( unsigned short port, in_addr ip, uDuration *timeout, int type = SOCK_STREAM, int protocol = 0 ) :
	    uSocketIO( socket.access, (sockaddr *)new inetAddr( port, ip ) ), socket( AF_INET, type, protocol ) {
		createSocketClient2( port, "", timeout, type, protocol );
	} // uSocketClient::uSocketClient

	virtual ~uSocketClient();

	void setServer( struct sockaddr *addr, socklen_t len ) {
#ifdef __U_DEBUG__
		if ( len > baddrlen ) {
			abort( "(uSocketClient &)%p.setServer( addr:%p, len:%d ) : New server address to large.", this, addr, len );
		} // if
#endif // __U_DEBUG__
		memcpy( saddr, addr, len );
	} // uSocketClient::setServer

	void getServer( struct sockaddr *addr, socklen_t *len ) {
#ifdef __U_DEBUG__
		if ( *len < baddrlen ) {
			abort( "(uSocketClient &)%p.getServer( addr:%p, len:%d ) : Area for server address to small.", this, addr, *len );
		} // if
#endif // __U_DEBUG__
		*len = baddrlen;
		memcpy( addr, saddr, baddrlen );
	} // uSocketClient::getServer
}; // uSocketClient


// Local Variables: //
// compile-command: "make install" //
// End: //
