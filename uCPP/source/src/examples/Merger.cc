//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 1994
// 
// Merger.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sun May 16 22:40:36 1993
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Apr 26 08:19:52 2022
// Update Count     : 78
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

#include <iostream>
using std::cout;
using std::endl;

struct PassInfo {
	int Max;
	int Posn;
};

const int HighValue = 99999999;

_Coroutine merger {
	merger **Partner;
	int *SortValues, NoOfSortValues, *MergedValues;
	PassInfo Next;

	void main() {
		int Posn = 0;
		SortValues[NoOfSortValues] = HighValue;			// mark the end of the list
		for ( ;; ) {
			for ( ;; ) {
			  if ( SortValues[Posn] == HighValue ) break;
			  if ( SortValues[Posn] >  Next.Max  ) break;
				MergedValues[Next.Posn] = SortValues[Posn];
				Posn += 1;
				Next.Posn += 1;
			} // for
		  if ( SortValues[Posn] == HighValue ) break;
			Next.Max = SortValues[Posn];
			Next = (*Partner)->upto( Next );
		} // for

		if ( Next.Max != HighValue ) {					// partner has already finished
			Next.Max = SortValues[Posn];
		} // if
		(*Partner)->upto( Next );
	} // merger::main
  public:
	merger( merger **Partner, int SortValues[], int NoOfSortValues, int MergedValues[] ) {
		merger::Partner = Partner;
		merger::SortValues = SortValues;
		merger::NoOfSortValues = NoOfSortValues;
		merger::MergedValues = MergedValues;
	} // merger::merger

	PassInfo upto( PassInfo Next ) {
		merger::Next = Next;
		resume();
		return merger::Next;
	} // merger::upto
}; // merger


int main() {
	const int MaxList1 = 10;
	const int MaxList2 = 11;
	const int MaxMergeList = MaxList1 + MaxList2;

	int list1[MaxList1 + 1] = { 2, 3, 3, 10, 14, 16, 20, 24, 28, 34 };
	int list2[MaxList2 + 1] = { 1, 3, 3, 7, 9, 11, 21, 25, 33, 37, 39 };
	int Mlist[MaxMergeList];
	PassInfo start;
	int i;

	cout << "list 1:" << endl;
	for ( i = 0; i < MaxList1; i += 1 ) {
		cout << list1[i] << " ";
	} // for
	cout << endl;

	cout << "list 2:" << endl;
	for ( i = 0; i < MaxList2; i += 1 ) {
		cout << list2[i] << " ";
	} // for
	cout << endl;

	merger *m1p, *m2p;
	merger m1( &m2p, list1, MaxList1, Mlist ), m2( &m1p, list2, MaxList2, Mlist );

	m1p = &m1;											// initialize coroutine partner names
	m2p = &m2;

	start.Posn = 0;
	if ( list1[0] < list2[0] ) {						// start the smaller of the two
		start.Max = list2[0];
		m1.upto( start );
	} else {
		start.Max = list1[0];
		m2.upto( start );
	} // if
	
	cout << "Merged Lists:" << endl;
	for ( i = 0; i < MaxMergeList; i += 1 ) {
		cout << Mlist[i] << " ";
	} // for
	cout << endl;

	cout << "successful completion" << endl;
} // uMain:main

// Local Variables: //
// compile-command: "u++ Merger.cc" //
// End: //
