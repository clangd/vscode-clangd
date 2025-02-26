//#include <uDebug.h>
#include <fstream>										// filebuf, ios_base
#include <sstream>										// ostringstream
#include <iostream>										// cerr
#include <cstdlib>										// srand, rand
#include <vector>										// vector
#include <set>                                          // multiset
#include <unistd.h>										// unlink


#define verify( x ) ( ( x ) ? true : ( ( std::cerr << "Error: assertion " #x " failed at " __FILE__ ":" << __LINE__ << "\n" ), false ) )


const char *tempFile = "xxx";


bool equalBuffers( std::filebuf &ibuf, std::filebuf &obuf ) {
	std::ostringstream input, output;
	ibuf.pubseekoff( 0, std::ios_base::beg );
	obuf.pubseekoff( 0, std::ios_base::beg );
	input << &ibuf;
	output << &obuf;
	return input.str() == output.str();
} // equalBuffers


bool overflow_test( std::filebuf &ibuf, std::filebuf &obuf, char *testFile ) {
	// overflow/underflow: copy input to output through various intermediate buffers
	const int bufsizes[] = { 1, 193, 256 };				// single-byte, prime, power-of-two
	bool success = true;
	for ( unsigned i = 0; i < sizeof( bufsizes ) / sizeof( int ); i += 1 ) {
		std::cerr << "copy with intermediate " << bufsizes[ i ] << "-byte buffer: ";
		char *intbuf = new char[ bufsizes[ i ] ];
		ibuf.open( testFile, std::ios_base::in );
		success &= verify( ibuf.is_open() );
		obuf.open( tempFile, std::ios_base::in | std::ios_base::out | std::ios_base::trunc );
		success &= verify( obuf.is_open() );
		for ( ;; ) {
			int nread = ibuf.sgetn( intbuf, bufsizes[ i ] );
			if ( nread == 0 ) break;
			int nwrote = obuf.sputn( intbuf, nread );
			success &= verify( nwrote == nread );
		} // for
		if ( equalBuffers( ibuf, obuf ) ) {
			std::cerr << "success\n";
		} else {
			std::cerr << "failure\n";
			success = false;
		} // if
		delete [] intbuf;
		ibuf.close();
		success &= verify( ! ibuf.is_open() );
		obuf.close();
		success &= verify( ! obuf.is_open() );
	} // for
	return success;
} // overflow_test


bool seek_test( std::filebuf &ibuf, std::filebuf &obuf, char *testFile, std::ios_base::seekdir way ) {
	// seekoff: divide the file into 10 randomly-sized fragments
	std::cerr << "seekoff mode " << way << ": ";

	bool success = true;
	const int nfrags = 10;
	std::multiset< int > fileOrder;
	std::vector< char > intbuf;

	struct list_t {
		int pos;
		std::set< int >::iterator order;
	} list[ nfrags ];

	ibuf.open( testFile, std::ios_base::in );
	success &= verify( ibuf.is_open() );
	obuf.open( "xxx", std::ios_base::in | std::ios_base::out | std::ios_base::trunc );
	success &= verify( obuf.is_open() );

	int lastpos = ibuf.pubseekoff( 0, std::ios_base::end );
	int opos = obuf.pubseekoff( lastpos, std::ios_base::beg );
	success &= verify( opos = lastpos );

	list[ 0 ].pos = 0;
	list[ 0 ].order = fileOrder.insert( 0 );
	for ( int i = 1; i < nfrags; i += 1 ) {
		list[ i ].pos = rand() % (lastpos + 1);
		list[ i ].order = fileOrder.insert( list[ i ].pos );
	} // for

	int curpos = lastpos;
	for ( int i = 0; i < nfrags; i += 1 ) {
		std::set< int >::iterator next = list[ i ].order;
		int len;
		if ( ++next == fileOrder.end() ) {
			len = lastpos - list[ i ].pos;
		} else {
			len = *next - list[ i ].pos;
		} // if
		int newipos = 0, newopos = 0;			// stop uninitialized warning
		switch( way ) {
		  case std::ios_base::beg:
			newipos = ibuf.pubseekoff( list[ i ].pos, way );
			newopos = obuf.pubseekoff( list[ i ].pos, way );
			break;

		  case std::ios_base::end:
			newipos = ibuf.pubseekoff( list[ i ].pos - lastpos, way );
			newopos = obuf.pubseekoff( list[ i ].pos - lastpos, way );
			break;

		  case std::ios_base::cur:
			newipos = ibuf.pubseekoff( list[ i ].pos - curpos, way );
			newopos = obuf.pubseekoff( list[ i ].pos - curpos, way );
			curpos = list[ i ].pos + len;
			break;
		  default: ;
			// stop warning about unhandled enumerators
		} // switch
		success &= verify( newipos == list[ i ].pos );
		success &= verify( newopos == list[ i ].pos );
		intbuf.reserve( len );
		int nread = ibuf.sgetn( &intbuf[ 0 ], len );
		success &= verify( nread == len );
		int nwrote = obuf.sputn( &intbuf[ 0 ], len );
		success &= verify( nwrote == len );
	} // for

	if ( equalBuffers( ibuf, obuf ) ) {
		std::cerr << "success\n";
	} else {
		std::cerr << "failure\n";
		success = false;
	} // if

	return success;
} // seek_test

	
int main( int argc, char *argv[] ) {
	const int ibufsizes[] = { 0, 1, 512 };
	const int obufsizes[] = { 0, 1, 512 };
	bool success = true;
	srand( time( nullptr ) ) ;
	for ( unsigned i = 0; i < sizeof( ibufsizes ) / sizeof( int ); i += 1 ) {
		for ( unsigned j = 0; j < sizeof( obufsizes ) / sizeof( int ); j += 1 ) {
			for ( int arg = 1; arg < argc; arg += 1 ) {
				std::cerr << argv[ arg ] << ": " << ibufsizes[ i ] << "-byte input buffer, " << obufsizes[ j ] << "-byte output buffer\n";
				std::filebuf ibuf, obuf;
				char *inarray = new char[ ibufsizes[ i ] ], *outarray = new char[ obufsizes[ j ] ];
				ibuf.pubsetbuf( inarray, ibufsizes[ i ] );
				obuf.pubsetbuf( outarray, obufsizes[ j ] );
				success &= overflow_test( ibuf, obuf, argv[ arg ] );
				success &= seek_test( ibuf, obuf, argv[ arg ], std::ios::beg );
				success &= seek_test( ibuf, obuf, argv[ arg ], std::ios::cur );
				success &= seek_test( ibuf, obuf, argv[ arg ], std::ios::end );
				delete [] inarray;
				delete [] outarray;
			} // for
		} // for
	} // for

	unlink( tempFile );
	if ( success ) {
		std::cerr << "All tests succeeded.\n";
		return 0;
	} else {
		std::cerr << "Error: some tests failed.\n";
		return 1;
	} // if
} // main
