//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2003
// 
// Allocation.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Fri Oct  3 22:58:11 2003
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Jul 31 18:04:13 2022
// Update Count     : 209
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

#include <unistd.h>										// sbrk
#include <iostream>
#include <iomanip>
using namespace std;

// unsigned int uDefaultPreemption() {
// 	return 0;
// } // uDefaultPreemption

_Task Worker {
	void main();
  public:
	Worker() : uBaseTask( 120 * 1000 ) {}				// larger stack
};


void Worker::main() {
	enum { NoOfAllocs = 5000, NoOfMmaps = 10 };
	char * locns[NoOfAllocs];
	size_t amount;
	enum { limit = 64 * 1024 };							// check alignments up to here

	// check new/delete

	for ( int j = 0; j < 40; j += 1 ) {
		for ( int i = 0; i < NoOfAllocs; i += 1 ) {
			locns[i] = new char[i];
			//cout << setw(6) << (void *)locns[i] << endl;
			for ( int k = 0; k < i; k += 1 ) locns[i][k] = '\345';
		} // for
		//cout << (char *)sbrk(0) - start << " bytes" << endl;

		for ( int i = 0; i < NoOfAllocs; i += 1 ) {
			//cout << setw(6) << (void *)locns[i] << endl;
			for ( int k = 0; k < i; k += 1 ) if ( locns[i][k] != '\345' ) abort( "new/delete corrupt storage1" );
			delete [] locns[i];
		} // for
		//cout << (char *)sbrk(0) - start << " bytes" << endl;

		for ( int i = 0; i < NoOfAllocs; i += 1 ) {
			locns[i] = new char[i];
			//cout << setw(6) << (void *)locns[i] << endl;
			for ( int k = 0; k < i; k += 1 ) locns[i][k] = '\345';
		} // for
		for ( int i = NoOfAllocs - 1; i >=0 ; i -= 1 ) {
			//cout << setw(6) << (void *)locns[i] << endl;
			for ( int k = 0; k < i; k += 1 ) if ( locns[i][k] != '\345' ) abort( "new/delete corrupt storage2" );
			delete [] locns[i];
		} // for
	} // for

	// check malloc/free (sbrk)

	for ( int i = 0; i < NoOfAllocs; i += 1 ) {
		size_t s = (i + 1) * 20;
		char * area = (char *)malloc( s );
		area[0] = '\345'; area[s - 1] = '\345';			// fill first/last
		area[malloc_usable_size( area ) - 1] = '\345';	// fill ultimate byte
		free( area );
	} // for

	for ( int i = 0; i < NoOfAllocs; i += 1 ) {
		size_t s = i + 1;								// +1 to make initialization simpler
		locns[i] = (char *)malloc( s );
		locns[i][0] = '\345'; locns[i][s - 1] = '\345';	// fill first/last
		locns[i][malloc_usable_size( locns[i] ) - 1] = '\345'; // fill ultimate byte
	} // for
	for ( int i = 0; i < NoOfAllocs; i += 1 ) {
		size_t s = i + 1;
		if ( locns[i][0] != '\345' || locns[i][s - 1] != '\345' ||
			 locns[i][malloc_usable_size( locns[i] ) - 1] != '\345' ) abort( "malloc/free corrupt storage" );
		free( locns[i] );
	} // for

	// check malloc/free (mmap)

	for ( int i = 0; i < NoOfMmaps; i += 1 ) {
		size_t s = i + malloc_mmap_start();				// cross over point
		char * area = (char *)malloc( s );
		area[0] = '\345'; area[s - 1] = '\345';			// fill first/last
		area[malloc_usable_size( area ) - 1] = '\345';	// fill ultimate byte
		free( area );
	} // for

	for ( int i = 0; i < NoOfMmaps; i += 1 ) {
		size_t s = i + malloc_mmap_start();				// cross over point
		locns[i] = (char *)malloc( s );
		locns[i][0] = '\345'; locns[i][s - 1] = '\345';	// fill first/last
		locns[i][malloc_usable_size( locns[i] ) - 1] = '\345'; // fill ultimate byte
	} // for
	for ( int i = 0; i < NoOfMmaps; i += 1 ) {
		size_t s = i + malloc_mmap_start();				// cross over point
		if ( locns[i][0] != '\345' || locns[i][s - 1] != '\345' ||
			 locns[i][malloc_usable_size( locns[i] ) - 1] != '\345' ) abort( "malloc/free corrupt storage" );
		free( locns[i] );
	} // for

	// check calloc/free (sbrk)

	for ( int i = 0; i < NoOfAllocs; i += 1 ) {
		size_t s = (i + 1) * 20;
		char * area = (char *)calloc( 5, s );
		if ( area[0] != '\0' || area[s - 1] != '\0' ||
			 area[malloc_size( area ) - 1] != '\0' ||
			 ! malloc_zero_fill( area ) ) abort( "calloc/free corrupt storage1" );
		area[0] = '\345'; area[s - 1] = '\345';			// fill first/last
		area[malloc_usable_size( area ) - 1] = '\345';	// fill ultimate byte
		free( area );
	} // for

	for ( int i = 0; i < NoOfAllocs; i += 1 ) {
		size_t s = i + 1;
		locns[i] = (char *)calloc( 5, s );
		if ( locns[i][0] != '\0' || locns[i][s - 1] != '\0' ||
			 locns[i][malloc_size( locns[i] ) - 1] != '\0' ||
			 ! malloc_zero_fill( locns[i] ) ) abort( "calloc/free corrupt storage2" );
		locns[i][0] = '\345'; locns[i][s - 1] = '\345';	// fill first/last
		locns[i][malloc_usable_size( locns[i] ) - 1] = '\345'; // fill ultimate byte
	} // for
	for ( int i = 0; i < NoOfAllocs; i += 1 ) {
		size_t s = i + 1;
		if ( locns[i][0] != '\345' || locns[i][s - 1] != '\345' ||
			 locns[i][malloc_usable_size( locns[i] ) - 1] != '\345' ) abort( "calloc/free corrupt storage3" );
		free( locns[i] );
	} // for

	// check calloc/free (mmap)

	for ( int i = 0; i < NoOfMmaps; i += 1 ) {
		size_t s = i + malloc_mmap_start();				// cross over point
		char * area = (char *)calloc( 1, s );
		if ( area[0] != '\0' || area[s - 1] != '\0' ) abort( "calloc/free corrupt storage4.1" );
		if ( area[malloc_size( area ) - 1] != '\0' ) abort( "calloc/free corrupt storage4.2" );
		if ( ! malloc_zero_fill( area ) ) abort( "calloc/free corrupt storage4.3" );
		area[0] = '\345'; area[s - 1] = '\345';			// fill first/last
		area[malloc_usable_size( area ) - 1] = '\345';	// fill ultimate byte
		free( area );
	} // for

	for ( int i = 0; i < NoOfMmaps; i += 1 ) {
		size_t s = i + malloc_mmap_start();				// cross over point
		locns[i] = (char *)calloc( 1, s );
		if ( locns[i][0] != '\0' || locns[i][s - 1] != '\0' ||
			 locns[i][malloc_size( locns[i] ) - 1] != '\0' ||
			 ! malloc_zero_fill( locns[i] ) ) abort( "calloc/free corrupt storage5" );
		locns[i][0] = '\345'; locns[i][s - 1] = '\345';	// fill first/last
		locns[i][malloc_usable_size( locns[i] ) - 1] = '\345'; // fill ultimate byte
	} // for
	for ( int i = 0; i < NoOfMmaps; i += 1 ) {
		size_t s = i + malloc_mmap_start();				// cross over point
		if ( locns[i][0] != '\345' || locns[i][s - 1] != '\345' ||
			 locns[i][malloc_usable_size( locns[i] ) - 1] != '\345' ) abort( "calloc/free corrupt storage6" );
		free( locns[i] );
	} // for

	// check memalign/free (sbrk)

	for ( size_t a = uAlign(); a <= limit; a += a ) {	// generate powers of 2
		//cout << setw(6) << alignments[a] << endl;
		for ( int s = 1; s < NoOfAllocs; s += 1 ) {		// allocation of size 0 can return null
			char * area = (char *)memalign( a, s );
			//cout << setw(6) << i << " " << area << endl;
			if ( (size_t)area % a != 0 || malloc_alignment( area ) != a ) { // check for initial alignment
				abort( "memalign/free bad alignment : memalign(%d,%d) = %p", (int)a, s, area );
			} // if
			area[0] = '\345'; area[s - 1] = '\345';		// fill first/last byte
			area[malloc_usable_size( area ) - 1] = '\345'; // fill ultimate byte
			free( area );
		} // for
	} // for

	// check memalign/free (mmap)

	for ( size_t a = uAlign(); a <= limit; a += a ) {	// generate powers of 2
		//cout << setw(6) << alignments[a] << endl;
		for ( int i = 1; i < NoOfMmaps; i += 1 ) {
			size_t s = i + malloc_mmap_start();			// cross over point
			char * area = (char *)memalign( a, s );
			//cout << setw(6) << i << " " << area << endl;
			if ( (size_t)area % a != 0 || malloc_alignment( area ) != a ) { // check for initial alignment
				abort( "memalign/free bad alignment : memalign(%d,%d) = %p", (int)a, (int)s, area );
			} // if
			area[0] = '\345'; area[s - 1] = '\345';		// fill first/last byte
			area[malloc_usable_size( area ) - 1] = '\345'; // fill ultimate byte
			free( area );
		} // for
	} // for

#ifndef ALLOCATOR										// uC++ allocator only
	// check malloc/resize/free (sbrk)

	for ( int i = 2; i < NoOfAllocs; i += 12 ) {
		// initial N byte allocation
		char * area = (char *)malloc( i );
		area[0] = '\345'; area[i - 1] = '\345';			// fill first/penultimate byte

		// Do not start this loop index at 0 because resize of 0 bytes frees the storage.
		int prev = i;
		for ( int s = i; s < 256 * 1024; s += 26 ) {	// start at initial memory request
			if ( area[0] != '\345' || area[prev - 1] != '\345' ) abort( "malloc/resize/free corrupt storage" );
			area = (char *)resize( area, s );			// attempt to reuse storage
			area[0] = area[s - 1] = '\345';				// fill last byte
			prev = s;
		} // for
		free( area );
	} // for

	// check malloc/resize/free (mmap)

	for ( int i = 2; i < NoOfAllocs; i += 12 ) {
		// initial N byte allocation
		size_t s = i + malloc_mmap_start();				// cross over point
		char * area = (char *)malloc( s );
		area[0] = '\345'; area[s - 1] = '\345';			// fill first/penultimate byte

		// Do not start this loop index at 0 because resize of 0 bytes frees the storage.
		int prev = s;
		for ( int r = s; r < 256 * 1024; r += 26 ) {	// start at initial memory request
			if ( area[0] != '\345' || area[prev - 1] != '\345' ) abort( "malloc/resize/free corrupt storage" );
			area = (char *)resize( area, s );			// attempt to reuse storage
			area[0] = area[r - 1] = '\345';				// fill last byte
			prev = r;
		} // for
		free( area );
	} // for

	// check malloc/realloc/free (sbrk)

	for ( int i = 2; i < NoOfAllocs; i += 12 ) {
		// initial N byte allocation
		char * area = (char *)malloc( i );
		area[0] = '\345'; area[i - 1] = '\345';			// fill first/penultimate byte

		// Do not start this loop index at 0 because realloc of 0 bytes frees the storage.
		int prev = i;
		for ( int s = i; s < 256 * 1024; s += 26 ) {	// start at initial memory request
			if ( area[0] != '\345' || area[prev - 1] != '\345' ) abort( "malloc/realloc/free corrupt storage" );
			area = (char *)realloc( area, s );			// attempt to reuse storage
			area[s - 1] = '\345';						// fill last byte
			prev = s;
		} // for
		free( area );
	} // for

	// check malloc/realloc/free (mmap)

	for ( int i = 2; i < NoOfAllocs; i += 12 ) {
		// initial N byte allocation
		size_t s = i + malloc_mmap_start();				// cross over point
		char * area = (char *)malloc( s );
		area[0] = '\345'; area[s - 1] = '\345';			// fill first/penultimate byte

		// Do not start this loop index at 0 because realloc of 0 bytes frees the storage.
		int prev = s;
		for ( int r = s; r < 256 * 1024; r += 26 ) {	// start at initial memory request
			if ( area[0] != '\345' || area[prev - 1] != '\345' ) abort( "malloc/realloc/free corrupt storage" );
			area = (char *)realloc( area, s );			// attempt to reuse storage
			area[r - 1] = '\345';						// fill last byte
			prev = r;
		} // for
		free( area );
	} // for

	// check calloc/realloc/free (sbrk)

	for ( int i = 1; i < 10000; i += 12 ) {
		// initial N byte allocation
		char * area = (char *)calloc( 5, i );
		if ( area[0] != '\0' || area[i - 1] != '\0' ||
			 area[malloc_size( area ) - 1] != '\0' ||
			 ! malloc_zero_fill( area ) ) abort( "calloc/realloc/free corrupt storage1" );

		// Do not start this loop index at 0 because realloc of 0 bytes frees the storage.
		for ( int s = i; s < 256 * 1024; s += 26 ) {	// start at initial memory request
			area = (char *)realloc( area, s );			// attempt to reuse storage
			if ( area[0] != '\0' || area[s - 1] != '\0' ||
				 area[malloc_size( area ) - 1] != '\0' ||
				 ! malloc_zero_fill( area ) ) abort( "calloc/realloc/free corrupt storage2" );
		} // for
		free( area );
	} // for

	// check calloc/realloc/free (mmap)

	for ( int i = 1; i < 1000; i += 12 ) {
		// initial N byte allocation
		size_t s = i + malloc_mmap_start();				// cross over point
		char * area = (char *)calloc( 1, s );
		if ( area[0] != '\0' || area[s - 1] != '\0' ||
			 area[malloc_size( area ) - 1] != '\0' ||
			 ! malloc_zero_fill( area ) ) abort( "calloc/realloc/free corrupt storage3" );

		// Do not start this loop index at 0 because realloc of 0 bytes frees the storage.
		for ( int r = i; r < 256 * 1024; r += 26 ) {	// start at initial memory request
			area = (char *)realloc( area, r );			// attempt to reuse storage
			if ( area[0] != '\0' || area[r - 1] != '\0' ||
				 area[malloc_size( area ) - 1] != '\0' ||
				 ! malloc_zero_fill( area ) ) abort( "calloc/realloc/free corrupt storage4" );
		} // for
		free( area );
	} // for

	// check memalign/resize with align/free

	amount = 2;
	for ( size_t a = uAlign(); a <= limit; a += a ) {	// generate powers of 2
		// initial N byte allocation
		char * area = (char *)memalign( a, amount );	// aligned N-byte allocation
		//sout | alignments[a] | area | endl;
		if ( (size_t)area % a != 0 || malloc_alignment( area ) != a ) { // check for initial alignment
			abort( "memalign/resize with align/free bad alignment : memalign(%d,%d) = %p", (int)a, (int)amount, area );
		} // if
		area[0] = '\345'; area[amount - 2] = '\345';	// fill first/penultimate byte

		// Do not start this loop index at 0 because resize of 0 bytes frees the storage.
		for ( int s = amount; s < 256 * 1024; s += 1 ) { // start at initial memory request
			area = (char *)resize( area, a * 2, s );	// attempt to reuse storage
			//sout | i | area | endl;
			if ( (size_t)area % a * 2 != 0 ) {			// check for initial alignment
				abort( "memalign/resize with align/free bad alignment %p", area );
			} // if
			area[s - 1] = '\345';						// fill last byte
		} // for
		free( area );
	} // for

	// check memalign/realloc/free

	amount = 2;
	for ( size_t a = uAlign(); a <= limit; a += a ) {	// generate powers of 2
		// initial N byte allocation
		char * area = (char *)memalign( a, amount );	// aligned N-byte allocation
		//cout << setw(6) << alignments[a] << " " << area << endl;
		if ( (size_t)area % a != 0 || malloc_alignment( area ) != a ) { // check for initial alignment
			abort( "memalign/realloc/free bad alignment : memalign(%d,%d) = %p", (int)a, (int)amount, area );
		} // if
		area[0] = '\345'; area[amount - 2] = '\345';	// fill first/penultimate byte

		// Do not start this loop index at 0 because realloc of 0 bytes frees the storage.
		for ( int s = amount; s < 256 * 1024; s += 1 ) { // start at initial memory request
			if ( area[0] != '\345' || area[s - 2] != '\345' ) abort( "memalign/realloc/free corrupt storage" );
			area = (char *)realloc( area, s );			// attempt to reuse storage
			//cout << setw(6) << i << " " << area << endl;
			if ( (size_t)area % a != 0 ) {				// check for initial alignment
				abort( "memalign/realloc/free bad alignment %p", area );
			} // if
			area[s - 1] = '\345';						// fill last byte
		} // for
		free( area );
	} // for

	// check cmemalign/free

	for ( size_t a = uAlign(); a <= limit; a += a ) {	// generate powers of 2
		//cout << setw(6) << alignments[a] << endl;
		for ( int s = 1; s < limit; s += 1 ) {			// allocation of size 0 can return null
			char * area = (char *)cmemalign( a, 1, s );
			//cout << setw(6) << i << " " << area << endl;
			if ( (size_t)area % a != 0 || malloc_alignment( area ) != a ) { // check for initial alignment
				abort( "cmemalign/free bad alignment : cmemalign(%d,%d) = %p", (int)a, s, area );
			} // if
			if ( area[0] != '\0' || area[s - 1] != '\0' ||
				 area[malloc_size( area ) - 1] != '\0' ||
				 ! malloc_zero_fill( area ) ) abort( "cmemalign/free corrupt storage" );
			area[0] = '\345'; area[s - 1] = '\345';		// fill first/last byte
			free( area );
		} // for
	} // for

	// check cmemalign/realloc/free

	amount = 2;
	for ( size_t a = uAlign() + uAlign(); a <= limit; a += a ) { // generate powers of 2
		// initial N byte allocation
		char * area = (char *)cmemalign( a, 1, amount ); // aligned N-byte allocation
		//cout << setw(6) << alignments[a] << " " << area << endl;
		if ( (size_t)area % a != 0 || malloc_alignment( area ) != a ) { // check for initial alignment
			abort( "cmemalign/realloc/free bad alignment : cmemalign(%d,%d) = %p", (int)a, (int)amount, area );
		} // if
		if ( area[0] != '\0' || area[amount - 1] != '\0' ||
			 area[malloc_size( area ) - 1] != '\0' ||
			 ! malloc_zero_fill( area ) ) abort( "cmemalign/realloc/free corrupt storage1" );
		area[0] = '\345'; area[amount - 2] = '\345';	// fill first/penultimate byte

		// Do not start this loop index at 0 because realloc of 0 bytes frees the storage.
		for ( int s = amount; s < 256 * 1024; s += 1 ) { // start at initial memory request
			if ( area[0] != '\345' || area[s - 2] != '\345' ) abort( "cmemalign/realloc/free corrupt storage2" );
			area = (char *)realloc( area, s );			// attempt to reuse storage
			//cout << setw(6) << i << " " << area << endl;
			if ( (size_t)area % a != 0 || malloc_alignment( area ) != a ) { // check for initial alignment
				abort( "cmemalign/realloc/free bad alignment %p", area );
			} // if
			if ( area[0] != '\345' || area[s - 1] != '\0' ||
				 area[malloc_size( area ) - 1] != '\0' ||
				 ! malloc_zero_fill( area ) ) abort( "cmemalign/realloc/free corrupt storage3" );
			area[s - 1] = '\345';						// fill last byte
		} // for
		free( area );
	} // for

	// check memalign/realloc with align/free

	amount = 2;
	for ( size_t a = uAlign(); a <= limit; a += a ) {	// generate powers of 2
		// initial N byte allocation
		char * area = (char *)memalign( a, amount );	// aligned N-byte allocation
		//cout << setw(6) << alignments[a] << " " << area << endl;
		if ( (size_t)area % a != 0 || malloc_alignment( area ) != a ) { // check for initial alignment
			abort( "memalign/realloc with align/free bad alignment : memalign(%d,%d) = %p", (int)a, (int)amount, area );
		} // if
		area[0] = '\345'; area[amount - 2] = '\345';	// fill first/penultimate byte

		// Do not start this loop index at 0 because realloc of 0 bytes frees the storage.
		for ( int s = amount; s < 256 * 1024; s += 1 ) { // start at initial memory request
			if ( area[0] != '\345' || area[s - 2] != '\345' ) abort( "memalign/realloc/free corrupt storage" );
			area = (char *)realloc( area, a * 2, s );	// attempt to reuse storage
			//cout << setw(6) << i << " " << area << endl;
			if ( (size_t)area % a * 2 != 0 ) {			// check for initial alignment
				abort( "memalign/realloc with align/free bad alignment %p", area );
			} // if
			area[s - 1] = '\345';						// fill last byte
		} // for
		free( area );
	} // for

	// check cmemalign/realloc with align/free

	amount = 2;
	for ( size_t a = uAlign() + uAlign(); a <= limit; a += a ) { // generate powers of 2
		// initial N byte allocation
		char * area = (char *)cmemalign( a, 1, amount ); // aligned N-byte allocation
		//cout << setw(6) << alignments[a] << " " << area << endl;
		if ( (size_t)area % a != 0 || malloc_alignment( area ) != a ) { // check for initial alignment
			abort( "cmemalign/realloc with align/free bad alignment : cmemalign(%d,%d) = %p", (int)a, (int)amount, area );
		} // if
		if ( area[0] != '\0' || area[amount - 1] != '\0' ||
			 area[malloc_size( area ) - 1] != '\0' ||
			 ! malloc_zero_fill( area ) ) abort( "cmemalign/realloc with align/free corrupt storage1" );
		area[0] = '\345'; area[amount - 2] = '\345';	// fill first/penultimate byte

		// Do not start this loop index at 0 because realloc of 0 bytes frees the storage.
		for ( int s = amount; s < 256 * 1024; s += 1 ) { // start at initial memory request
			if ( area[0] != '\345' || area[s - 2] != '\345' ) abort( "cmemalign/realloc with align/free corrupt storage2" );
			area = (char *)realloc( area, a * 2, s );	// attempt to reuse storage
			//cout << setw(6) << i << " " << area << endl;
			if ( (size_t)area % a * 2 != 0 || malloc_alignment( area ) != a * 2 ) { // check for initial alignment
				abort( "cmemalign/realloc with align/free bad alignment %p %zd %zd", area, malloc_alignment( area ), a * 2 );
			} // if
			if ( area[s - 1] != '\0' || area[s - 1] != '\0' ||
				 area[malloc_size( area ) - 1] != '\0' ||
				 ! malloc_zero_fill( area ) ) abort( "cmemalign/realloc/free corrupt storage3" );
			area[s - 1] = '\345';						// fill last byte
		} // for
		free( area );
	} // for
#endif // ALLOCATION
	osacquire( cout ) << "worker " << &uThisTask() << " successful completion" << endl;
} // Worker::main

int main() {
	const unsigned int NoOfWorkers = 4;
	{
		uProcessor processors[NoOfWorkers - 1] __attribute__(( unused )); // more than one processor
		Worker workers[NoOfWorkers] __attribute__(( unused ));
	}
	malloc_stats();
} // main

// Local Variables: //
// tab-width: 4 //
// End: //
