//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// Paging.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Wed May 11 17:03:26 1994
// Last Modified By : Peter A. Buhr
// Last Modified On : Sat Apr 23 18:24:55 2022
// Update Count     : 16
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

#include <fstream>
using std::ifstream;
using std::ofstream;
#include <iostream>
using std::istream;
using std::ostream;
using std::cout;
using std::cerr;
using std::cin;
using std::endl;

#define Kb *1024

unsigned int uDefaultStackSize() {
	return 64 Kb;
} // uDefaultStackSize

int searchUp( int Array[], int Start, int End,  int Key ) { // linear search forward through a section of an array
	int i;
	
	for ( i = Start;; i += 1 ) {
	  if ( i >= End ) break;
	  if ( Array[i] == Key ) break;
	} // for
	return i;
} // searchUp

int searchDown( int Array[], int Start, int End, int Key ) { // linear search backward through a section of an array
	int i;
	
	for ( i = Start;; i -= 1 ) {
	  if ( i < End ) break;
	  if ( Array[i] == Key ) break;
	} // for
	return i;
} // searchDown

#define MaxPageRequests 100

istream *infile = &cin;
ostream *outfile = &cout;

_Coroutine Replacement {
  protected:
  int *Memory, MemorySize, *PageRequests, NoOfPageRequests;
  int CurrRequest, repage;
  public:
  int replace( int CurrRequest ) {
	  Replacement::CurrRequest = CurrRequest;
	  resume();
	  return repage;
  } // replace
}; // Replacement

_Coroutine Optimal : public Replacement {
  protected:
	void main() {
		int Max, i, posn;

		for ( ; CurrRequest != -1; ) {
			Max = -1;
			for ( i = 0; i < MemorySize; i += 1 ) {		// for each page in the memory
				posn = searchUp( PageRequests, CurrRequest + 1, NoOfPageRequests, Memory[i] ); // look into the future
				if ( posn > Max ) {
					Max = posn;
					repage = i;
				} // if
			} // for
			suspend();
		} // for
	} // Optimal::main
  public:
	Optimal( int Memory[], int MemorySize, int PageRequests[], int NoOfPageRequests ) {
		Optimal::Memory = Memory;
		Optimal::MemorySize = MemorySize;
		Optimal::PageRequests = PageRequests;
		Optimal::NoOfPageRequests = NoOfPageRequests;
	} // Optimal::Optimal
}; // Optimal

_Coroutine FIFO : public Replacement {
  protected:
	void main() {
		repage = -1;
	
		for ( ; CurrRequest != -1; ) {
			repage = ( repage + 1 ) % MemorySize;
			suspend();
		} // for
	} // FIFO::main
  public:
	FIFO( int Memory[], int MemorySize, int PageRequests[], int NoOfPageRequests ) {
		FIFO::Memory = Memory;
		FIFO::MemorySize = MemorySize;
		FIFO::PageRequests = PageRequests;
		FIFO::NoOfPageRequests = NoOfPageRequests;
	} // FIFO::FIFO
}; // FIFO

_Coroutine LRU : public Replacement {
  protected:
	void main() {
		int Min, i, posn;
	
		for ( ; CurrRequest != -1; ) {
			Min = CurrRequest;
			for ( i = 0; i < MemorySize; i += 1 ) {		// for each page in the memory
				posn = searchDown(PageRequests, CurrRequest - 1, 0, Memory[i]); // look into the past
				if ( posn < Min ) {
					Min = posn;
					repage = i;
				} // if
			} // for
			suspend();
		} // for
	} // LRU::main
  public:
	LRU( int Memory[], int MemorySize, int PageRequests[], int NoOfPageRequests ) {
		LRU::Memory = Memory;
		LRU::MemorySize = MemorySize;
		LRU::PageRequests = PageRequests;
		LRU::NoOfPageRequests = NoOfPageRequests;
	} // LRU::LRU
}; // LRU

_Coroutine Pager {
	int *Memory, MemorySize, *PageRequests, NoOfPageRequests;
	Replacement *replacement;
	int pagefaults;

	void main() {
		int RepPage, PagesUsed;
		int Results[MemorySize+1][MaxPageRequests];
		int i, j;

		for (i = 0; i < MemorySize; i += 1) Memory[i] = -1; // initialize memory to null
		PagesUsed = 0;
	
		for ( i = 0; i < NoOfPageRequests; i += 1 ) {	// for each page request
			if ( searchUp( Memory, 0, PagesUsed, PageRequests[i] ) == PagesUsed ) { // page not in memory ?
				if ( PagesUsed == MemorySize ) {		// are all pages being used ?
					RepPage = replacement->replace( i );
				} else {
					RepPage = PagesUsed;
					PagesUsed += 1;
				} // if
				Memory[RepPage] = PageRequests[i];		// insert page into memory
				Results[MemorySize][i] = -1;
			} else {
				Results[MemorySize][i] = 0;
			} // if
	    
			for (j = 0; j < MemorySize; j += 1) {		// store memory over time
				Results[j][i] = Memory[j];
			} // for
		} // for
		i = -1;
		replacement->replace( i );						// terminate replacement scheme

		for ( i = 0; i < MemorySize; i += 1 ) {
			for ( j = 0; j < NoOfPageRequests; j += 1 ) {
				if (Results[i][j] != -1) {
					*outfile << Results[i][j] << "|";
				} else {
					*outfile << " |";
				} // if
			} // for
			*outfile << endl;
		} // for

		pagefaults = 0;
		for ( i = 0; i < NoOfPageRequests; i += 1 ) {
			if ( Results[MemorySize][i] == -1 ) {
				*outfile << "*|";
				pagefaults += 1;
			} else {
				*outfile << " |";
			}
		} // for
		*outfile << endl;

		for ( i = 0; i < NoOfPageRequests; i += 1 ) {
			*outfile << PageRequests[i] << "|";
		} // for
		*outfile << endl;
	} // Pager::main
  public:
	Pager( int Memory[], int MemorySize, int PageRequests[], int NoOfPageRequests, Replacement *replacement ) {
		Pager::Memory = Memory;
		Pager::MemorySize = MemorySize;
		Pager::PageRequests = PageRequests;
		Pager::NoOfPageRequests = NoOfPageRequests;
		Pager::replacement = replacement;
	} // Pager::Pager

	int faults() {
		resume();
		return pagefaults;
	} // Pager::faults
}; // Pager

int main( int argc, char *argv[] ) {
	switch ( argc ) {									// deal with input and output file parameters
	  case 3:
		outfile = new ofstream( argv[2] );
		if ( outfile->fail() ) {
			cerr << "Open error, outfile file: " << argv[2] << endl;
			exit( EXIT_FAILURE );
		} // if
	  case 2:
		infile = new ifstream( argv[1] );				// 1st file is input file
		if ( infile->fail() ) {
			cerr << "Open error, infile file: " << argv[1] << endl;
			exit( EXIT_FAILURE );
		} // if
	  case 1:
		break;
	  default:
		cerr << "Usage: " << argv[0] << " [infile-file [outfile-file]]" << endl;
		exit( EXIT_FAILURE );
	} // switch

	int NoValues;
	int MemorySize;
	int PageRequests[MaxPageRequests], NoOfPageRequests;
	int i, faults;

	for ( ;; ) {
		for ( NoOfPageRequests = 0;; NoOfPageRequests += 1 ) { // read page requests
			*infile >> PageRequests[NoOfPageRequests];
			if ( infile->eof() ) goto Eof;
		  if ( PageRequests[NoOfPageRequests] == -1 ) break;
		} // for

		*outfile << "For page references: ";
		for ( i = 0; i < NoOfPageRequests; i += 1 ) {	// print page requests
			*outfile << PageRequests[i] << "  ";
		} // for
		*outfile << endl << endl; 
	
		for ( i = 0;; i += 1 ) {
			*infile >> MemorySize;
		  if ( infile->eof() ) goto Eof;
		  if ( MemorySize == -1 ) break;
			*outfile << "and a " << MemorySize << " page memory" << endl;

			int Memory[MemorySize];
			{
				Optimal optimal( Memory, MemorySize, PageRequests, NoOfPageRequests );
				Pager pager( Memory, MemorySize, PageRequests, NoOfPageRequests, &optimal );
				faults = pager.faults();
				*outfile << "Optimal paging strategy produced " << faults << " page faults" << endl << endl;
			}
			{
				FIFO fifo( Memory, MemorySize, PageRequests, NoOfPageRequests );
				Pager pager( Memory, MemorySize, PageRequests, NoOfPageRequests, &fifo );
				faults = pager.faults();
				*outfile << "FIFO paging strategy produced " << faults << " page faults" << endl << endl;
			}
			{
				LRU lru( Memory, MemorySize, PageRequests, NoOfPageRequests );
				Pager pager( Memory, MemorySize, PageRequests, NoOfPageRequests, &lru );
				faults = pager.faults();
				*outfile << "LRU paging strategy produced " << faults << " page faults" << endl << endl;
			}
			*outfile << endl;
		} // for
	} // for
  Eof:
	*outfile << "successful completion" << endl;
	if ( infile != &cin ) delete infile;				// close files
	if ( outfile != &cout ) delete outfile;
} // main
			 
// Local Variables: //
// compile-command: "u++ Paging.cc" //
// End: //
