// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// uHeapLmmm.h -- 
// 
// Author           : Peter A. Buhr
// Created On       : Wed Jul 20 00:07:05 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Jul  2 22:39:16 2024
// Update Count     : 536
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

#include <malloc.h>

extern "C" {
	// New allocation operations.
	void * aalloc( size_t dim, size_t elemSize ) __THROW __attribute__ ((malloc));
	void * resize( void * oaddr, size_t size ) __THROW __attribute__ ((malloc));
	void * amemalign( size_t align, size_t dim, size_t elemSize ) __THROW __attribute__ ((malloc));
	void * cmemalign( size_t align, size_t dim, size_t elemSize ) __THROW __attribute__ ((malloc));
	size_t malloc_alignment( void * addr ) __THROW;
	bool malloc_zero_fill( void * addr ) __THROW;
	size_t malloc_size( void * addr ) __THROW;
	int malloc_stats_fd( int fd ) __THROW;
	size_t malloc_expansion();							// heap expansion size (bytes)
	size_t malloc_mmap_start();							// crossover allocation size from sbrk to mmap
	size_t malloc_unfreed();							// heap unfreed size (bytes)
	void malloc_stats_clear();							// clear heap statistics
	void heap_stats();									// print thread-heap statistics
} // extern "C"

// New allocation operations.
void * resize( void * oaddr, size_t nalign, size_t size ) __THROW;
void * realloc( void * oaddr, size_t nalign, size_t size ) __THROW;
void * reallocarray( void * oaddr, size_t nalign, size_t dim, size_t elemSize ) __THROW;


// Local Variables: //
// mode: c++ //
// tab-width: 4 //
// compile-command: "make install" //
// End: //
