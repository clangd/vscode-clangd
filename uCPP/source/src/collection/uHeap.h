//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2000
// 
// uHeap.h -- generic heap data structure
// 
// Author           : Ashif S. Harji
// Created On       : Tue Jun 20 11:12:58 2000
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Nov 16 15:10:44 2021
// Update Count     : 68
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


//#include <uDebug.h>


template<typename KeyType, typename DataType> struct uHeapable {
	KeyType key;
	DataType data;
	uHeapable() {}
	uHeapable( KeyType key, DataType data ) : key( key ), data( data ) {}
}; // uHeapable


// array size a compile time constant => allocate space within the object
template<int MaxSize> struct uStaticHeapArray {
	template<typename Elt> class ArrayType {
		Elt A[ MaxSize + 1 ];
	  public:
		ArrayType() {}
	
		const Elt & operator[]( int index ) const {
			#ifdef __U_DEBUG__
			assert( 0 < index && index <= MaxSize );
			#endif // __U_DEBUG__
			return A[ index ];
		} // ArrayType::operator[]
	
		Elt & operator[]( int index ) {
			#ifdef __U_DEBUG__
			assert( 0 < index && index <= MaxSize );
			#endif // __U_DEBUG__
			return A[ index ];
		} // ArrayType::operator[]
	}; // uStaticHeapArray::ArrayType
}; // uStaticHeapArray


template<typename T> class uFlexArray;					// forward declaration

// array size not a compile time constant => use flexible array
template<typename Elt> class uDynamicHeapArray {
	uFlexArray<Elt> A;
  public:
	uDynamicHeapArray() {}
	uDynamicHeapArray( int size ) : A( size + 1 ) {}

	const Elt & operator[]( int index ) const {
		return A[ index ];
	} // uDynamicHeapArray::operator[]

	Elt & operator[]( int index ) {
		A.reserve( index + 1 );							// ensure sufficient space allocated
		return A[ index ];
	} // uDynamicHeapArray::operator[]
}; // uDynamicHeapArray


// Heap is generic in the type of elements stored in the heap, the maximum number of elements that can be stored in the
// heap, the array-like data structure used to represent the heap, and the comparison and exchange functions

template<typename KeyType, typename DataType, template<typename Elt> class Array> class uGenericHeap {
  protected:
	typedef int (* CompareFn)( KeyType x, KeyType y );
	typedef void (* ExchangeFn)( uHeapable<KeyType,DataType> & x, uHeapable<KeyType,DataType> & y );
	static void defaultExchange( uHeapable<KeyType,DataType> & x, uHeapable<KeyType,DataType> & y ) {
		uHeapable<KeyType,DataType> temp = x;
		x = y;   
		y = temp;
	} // defaultExchange
	
	#ifdef DEBUG_SHOW_COUNT
	int NumCompares, NumExchg, NumTransfers, NumHeapifies;
	#endif
	int heapSize, cursor;
	CompareFn compare;
	ExchangeFn exchange;
  public:
	Array<uHeapable<KeyType,DataType> > A;				// do not use A[0], valid subscripts are 1..MaxSize
  private:
	void createGenericHeap( CompareFn compare, ExchangeFn exchange ) {
		uGenericHeap::compare = compare;
		uGenericHeap::exchange = exchange;
		heapSize = 0;
		cursor = 1;
	} // uGenericHeap::createGenericHeap

	int parent( int i ) {
		return i >> 1;
	} // uGenericHeap::parent
	
	int left( int i ) {
		return i << 1;
	} // uGenericHeap::left
	
	int right( int i ) {
		return i << 1 | 1;
	} // uGenericHeap::right
  public:
	uGenericHeap( CompareFn compare, ExchangeFn exchange = defaultExchange ) {
		createGenericHeap( compare, exchange );
		#ifdef DEBUG_SHOW_COUNT
		NumCompares = NumExchg = NumTransfers = NumHeapifies = 0;
		#endif
	} // uGenericHeap::uGenericHeap

	uGenericHeap( int size, CompareFn compare, ExchangeFn exchange = defaultExchange ) : A( size ) {
		createGenericHeap( compare, exchange );
		#ifdef DEBUG_SHOW_COUNT
		NumCompares = NumExchg = NumTransfers = NumHeapifies = 0;
		#endif
	} // uGenericHeap::uGenericHeap

	#ifdef DEBUG_SHOW_COUNT
	void display_counts() {
		osacquire acq( cerr );
		cerr << "Heap::~Heap() - Total Number of Compare Operations = " << NumCompares << "\n";
		cerr << "Heap::~Heap() - Total Number of Exchange Operations = " << NumExchg << "\n";
		cerr << "Heap::~Heap() - Total Number of Transfer Operations = " << NumTransfers << "\n";
		cerr << "Heap::~Heap() - Total Number of Heapify Operations = " << NumHeapifies << "\n";
	} // uGenericHeap::display_counts
	
	void accumulate_counts( int & C, int & E, int & T, int & H ) {
		C += NumCompares;
		E += NumExchg;
		T += NumTransfers;
		H += NumHeapifies;
	} // uGenericHeap::accumulate_counts
	#endif

	int size() const {									// heap size
		return heapSize;
	} // uGenericHeap::size

	void getRoot( uHeapable<KeyType,DataType> & root ) const {
		root = A[1];
		#ifdef DEBUG_SHOW_COUNT
		NumTransfers += 1;
		#endif
	} // uGenericHeap::getRoot
	
	void deleteInsert( KeyType new_key, DataType new_data ) { // remove root element
		#ifdef __U_DEBUG__
		assert( heapSize > 0 );
		#endif // __U_DEBUG__
	
		A[1].key = new_key;
		A[1].data = new_data;
		#ifdef DEBUG_SHOW_COUNT
		NumTransfers += 1;
		#endif
		heapify( 1 );
	} // uGenericHeap::deleteInsert

	void deleteRoot() {									// remove root element
		#ifdef __U_DEBUG__
		assert( heapSize > 0 );
		#endif // __U_DEBUG__
	
		exchange( A[1], A[size()] );
		#ifdef DEBUG_SHOW_COUNT
		NumTransfers += 1;
		#endif
		heapSize -= 1;
		heapify( 1 );
	} // uGenericHeap::extractRoot

	void deletenode( uHeapable<KeyType,DataType> & node ) {
		exchange( node, A[size()] );
		heapSize -= 1;
		heapify( 1 );
	} // uGenericHeap::deletenode

	void insert( KeyType key, DataType data ) {			// insert element into heap
		heapSize += 1;
		A[heapSize].key = key;
		A[heapSize].data = data;
		for ( int i = heapSize; i > 1 && compare( A[parent( i )].key, key ) == -1; exchange( A[i], A[parent( i )] ), i = parent( i ) ) {
		#ifdef DEBUG_SHOW_COUNT
			NumTransfers += 1;
			NumCompares += 1;
		#endif
		} // for
		#ifdef DEBUG_SHOW_COUNT
		NumCompares += 1;
		NumTransfers += 1;
		#endif
	} // uGenericHeap::insert

	void insert_last( KeyType key, DataType data ) {
		heapSize += 1;
		#ifdef DEBUG_SHOW_COUNT
		NumTransfers += 1;
		#endif
		A[heapSize].key = key;
		A[heapSize].data = data;
	} // uGenericHeap::insert
	
	DataType find( KeyType key ) {						// find if element is in heap
		for ( int i = 1; i <= heapSize; i += 1 ) {
			if ( compare( A[i].key, key ) == 0 ) return A[i].data;
		} // for

		return( (DataType)nullptr );
	} // uGenericHeap::find

	void sort() {
		int last, saveHeapSize = heapSize;
	
		for ( last = heapSize; last >= 2; last -= 1 ) {
			uHeapable<KeyType,DataType> saveA1 = A[1];	// save A[1]

			int h = pullDown( 1 );						// let the hole at A[1] fall to the bottom
			// put A[last] in the hole at the bottom and bubble up
			if ( h < last ) {
				#ifdef DEBUG_SHOW_COUNT
				NumCompares += 1;
				NumTransfers += 1;
				#endif
				for ( exchange( A[h], A[last] );
					  h > 1 && compare( A[parent(h)].key, A[h].key) == -1;
					  exchange( A[h],A[parent( h)]), h=parent( h)) {
					#ifdef DEBUG_SHOW_COUNT
					NumCompares += 1;
					#endif
				} // for
			} // if
			A[last] = saveA1;							// put the earlier saved A[1] at vacated last
			heapSize -= 1;
			#ifdef DEBUG_SHOW_COUNT
			NumTransfers += 2;
			#endif
		} // for
		heapSize = saveHeapSize;
	} // uGenericHeap::sort
  private:
	int pullDown( int j ) {
		// There is a hole at the top of the heap rooted at j make the hole move down to the last level by moving the
		// children up

		int  larger;
		for ( int i = j; ; i = larger ) {
			int l, r;

			l = left( i );
			if ( l > size() ) return i;					// i has no children; return
			r = right( i );
		
			// find the position of the largee of the 2 (or 1) children
			larger = l;
			if ( r <= size() ) {
				if ( compare( A[r].key, A[larger].key ) != -1 )
					larger = r;
				#ifdef DEBUG_SHOW_COUNT
				NumCompares += 1;
				#endif
			} // if
		
			exchange( A[i], A[larger] ); 
		
			#ifdef DEBUG_SHOW_COUNT
			NumTransfers += 1;
			NumHeapifies += 1;
			#endif
		} // for
	} // uGenericHeap::pullDown
	
	void heapify( int j ) {
		int  largest;
		for ( int i = j; ; i = largest ) {
			int l, r;

			l = left( i );
		  if ( l > size() ) break;
			r = right( i );

			// find the position of the largest of the 3 values

			#ifdef DEBUG_SHOW_COUNT
			NumCompares += 1;
			#endif
			if ( compare( A[l].key, A[i].key ) == 1) {
				largest = l;
			} else {
				largest = i;
			} // if

			if ( r <= size() ) {
				if ( compare( A[r].key, A[largest].key ) == 1)
					largest = r;
				#ifdef DEBUG_SHOW_COUNT
				NumCompares += 1;
				#endif
			} // if
			#ifdef DEBUG_SHOW_COUNT
			NumHeapifies += 1;
			#endif
		  if ( largest == i ) break;
			exchange( A[i], A[largest] );
		} // for
	} // uGenericHeap::heapify
  public:
	void buildHeap() {
		for ( int i = heapSize >> 1; i >= 1; i -= 1 ) {
			heapify( i );
		} // for
	} // uGenericHeap::buildHeap
}; // uHeap


template<typename KeyType, typename DataType, int MaxSize>
class uHeap : public uGenericHeap<KeyType, DataType, uStaticHeapArray<MaxSize>::template ArrayType> {
	typedef uGenericHeap<KeyType, DataType, uStaticHeapArray<MaxSize>::template ArrayType> Base;
  public:
	uHeap( typename Base::CompareFn compare, typename Base::ExchangeFn exchange = Base::defaultExchange ) : Base( compare, exchange ) {}
	uHeap( int size, typename Base::CompareFn compare, typename Base::ExchangeFn exchange = Base::defaultExchange ) : Base( size, compare, exchange ) {}
}; // uHeap


template<typename KeyType, typename DataType>
class uDynamicHeap : public uGenericHeap<KeyType, DataType, uDynamicHeapArray> {
	typedef uGenericHeap<KeyType, DataType, uDynamicHeapArray> Base;
  public:
	uDynamicHeap( typename Base::CompareFn compare, typename Base::ExchangeFn exchange = Base::defaultExchange ) : Base( compare, exchange ) {}
	uDynamicHeap( int size, typename Base::CompareFn compare, typename Base::ExchangeFn exchange = Base::defaultExchange ) : Base( size, compare, exchange ) {}
}; // uHeap


template<typename KeyType, typename DataType, int MaxSize> class uHeapPtrSort {
  public:
	uHeapPtrSort( uHeap<KeyType,DataType,MaxSize> * h, DataType DataRecords, DataType tempRecPtr ) {
		sortRecs( h, DataRecords, tempRecPtr );
	} // uHeapPtrSort::uHeapPtrSort
	
	// assumes DataType is a pointer to records and that the heap is already sorted arranges the data records pointed to
	// by DataType in the same order as that in the heap...
	// IMPORTANT: this is destructive - wipes out the Data fields in the heap...  The records are assumed to be to
	// stored in array DataRecords.  Algorithm based on discussions with Anna Lubiw/Ian Munro - akg
	
	void sortRecs( uHeap<KeyType,DataType,MaxSize> * h, DataType DataRecords, DataType tempRecPtr ) {
		#ifdef DEBUG_SHOW_COUNT
		int recsMoved = 0;
		#endif
		// convert DataType pointers into indices into DataRecords for simplicity.
		for ( int i = 1; i <= h->heapSize; i += 1 ) {
			*((int *)&(h->A[i].data)) = h->A[i].data - DataRecords;
		} // for

		for ( int cycleStart = 1; cycleStart <= h->heapSize; cycleStart += 1 ) {
			if ( (int)h->A[cycleStart].data != 0) {
				if ( (int)h->A[cycleStart].data == cycleStart ) {
					h->A[cycleStart].data = 0;
				} else {
					int current = cycleStart;
					*tempRecPtr = DataRecords[current];
					#ifdef DEBUG_SHOW_COUNT
					recsMoved += 1;
					#endif
					for ( ;; ) {
						int next = (int)h->A[current].data;
						h->A[current].data = 0;
					  if ( next == cycleStart) break;

						DataRecords[current] = DataRecords[next];
						#ifdef DEBUG_SHOW_COUNT
						recsMoved += 1;
						#endif
						current = next;
					} // for
					DataRecords[current] = *tempRecPtr;
					#ifdef DEBUG_SHOW_COUNT
					recsMoved += 1;
					#endif
				} // if
			} // if
		} // for
		#ifdef DEBUG_SHOW_COUNT
		// osacquire( cerr ) << "Total Records Moved = " << recsMoved << endl;
		#endif
	} // uHeapPtrSort::sortRecs
};


// Local Variables: //
// compile-command: "make install" //
// End: //
