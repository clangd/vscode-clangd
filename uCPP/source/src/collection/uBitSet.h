//                              -*- Mode: C++ -*- 
// 
// Copyright (C) Peter A. Buhr 2003
// 
// uBitSet.h -- Fast bit-set operations
// 
// Author           : Peter A. Buhr and Thierry Delisle
// Created On       : Mon Dec 15 14:05:51 2003
// Last Modified By : Peter A. Buhr
// Last Modified On : Tue Dec 27 08:31:16 2022
// Update Count     : 282
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


#include <cassert>										// assert
#include <cstring>										// ffs, memset
#include <cstdint>										// uint32_t, uint64_t, __int128
#include <climits>										// CHAR_BIT

namespace UPP {
	inline int FFS( uint32_t v ) {
		return ::ffs( v );
	} // FFS

	inline int FFS( uint64_t v ) {
		return ::ffsll( v );
	} // FFS

	#if _GLIBCXX_USE_INT128 == 1
	inline int FFS( unsigned __int128 v ) {
		if ( (uint64_t)v != 0 ) {						// most significant bits
			return ::ffsll( (uint64_t)v );
		} else if ( (uint64_t)(v >> 64) != 0 ) {		// least significant bits
			return ::ffsll( (uint64_t)(v >> 64) ) + 64;
		} else return 0;
	} // FFS
	#endif // _GLIBCXX_USE_INT128 == 1


	template<unsigned int nbits, unsigned int size> class uBitSetImpl {
		static_assert( nbits == size, "nbits != size" ); // size unnecessary
		#if __WORDSIZE == 64
		typedef uint64_t BaseType;
		static_assert( sizeof( BaseType ) == 8, "sizeof( BaseType ) != 8" );
		enum { idxshift = 6, idxmask = 0x3f, nbase = ( ( nbits - 1 ) >> idxshift ) + 1 };
		#else
		typedef uint32_t BaseType;
		static_assert( sizeof( BaseType ) == 4, "sizeof( BaseType ) != 4" );
		enum { idxshift = 5, idxmask = 0x1f, nbase = ( ( nbits - 1 ) >> idxshift ) + 1 };
		#endif // __WORDSIZE == 64
		BaseType bits[ nbase ];
	  public:
		void set( unsigned int idx ) {
			assert( idx < nbits );
			bits[ idx >> idxshift ] |= (BaseType)1 << ( idx & idxmask );
		} // uBitSetImpl::set

		void clr( unsigned int idx ) {
			assert( idx < nbits );
			bits[ idx >> idxshift ] &= ~((BaseType)1 << ( idx & idxmask ));
		} // uBitSetImpl::clr

		void setAll() {
			memset( bits, -1, nbase * sizeof( BaseType ) ); // assumes 2's complement
		} // uBitSetImpl::setAll

		void clrAll() {
			memset( bits, 0, nbase * sizeof( BaseType ) );
		} // uBitSetImpl::clrAll

		bool isSet( unsigned int idx ) const {
			assert( idx < nbits );
			return bits[ idx >> idxshift ] & ( (BaseType)1 << ( idx & idxmask ) );
		} // uBitSetImpl::isSet

		bool isAllClr() const {
			int elt;
			for ( elt = 0; elt < nbase; elt += 1 ) {
			  if ( bits[ elt ] != 0 ) break;
			} // for
			return elt == nbase;
		} // uBitSetImpl::isAllClr

		int ffs() const {
			int elt;
			for ( elt = 0;; elt += 1 ) {
			  if ( elt >= nbase ) return nbits;			// no bits set
			  if ( bits[ elt ] != 0 ) break;
			} // for
			return (UPP::FFS( bits[ elt ] ) - 1) + ( elt << idxshift );	// range 0..N-1
		} // uBitSetImpl::ffs

		BaseType operator[]( unsigned int i ) const {
			assert( i < nbase );
			return bits[ i ];
		} // uBitSetImpl::operator[]
	}; // uBitSetImpl


	// optimizations for hardware supported integral types

	template<unsigned int nbits> struct uBitSetType;

	template<> struct uBitSetType< sizeof( uint32_t ) * CHAR_BIT > {
		typedef uint32_t type;
	}; // uBitSetType

	template<> struct uBitSetType< sizeof( uint64_t ) * CHAR_BIT > {
		typedef uint64_t type;
	}; // uBitSetType

	#if _GLIBCXX_USE_INT128 == 1
	template<> struct uBitSetType< sizeof( unsigned __int128 ) * CHAR_BIT > {
		typedef unsigned __int128 type;
	}; // uBitSetType
	#endif // _GLIBCXX_USE_INT128 == 1

	template<unsigned int nbits, unsigned int size> class uSpecBitSet {
		typename uBitSetType<nbits>::type bits;
	  public:
		void set( unsigned int idx ) {
			assert( idx < size );
			bits |= (decltype( bits ))1 << idx;
		} // uSpecBitSet::set

		void clr( unsigned int idx ) {
			assert( idx < size );
			bits &= ~((decltype( bits ))1 << idx);
		} // uSpecBitSet::clr

		void setAll() {
			bits = (decltype( bits ))-1;				// assumes 2's complement
		} // uSpecBitSet::setAll

		void clrAll() {
			bits = 0;
		} // uSpecBitSet::clrAll

		bool isSet( unsigned int idx ) const {
			assert( idx < size );
			return bits & ((decltype( bits ))1 << idx);
		} // uSpecBitSet::is Set

		bool isAllClr() const {
			return bits == 0;
		} // uSpecBitSet::isAllClr

		int ffs() const {
			int temp = UPP::FFS( bits );
			return temp == 0 ? size						// no bits set
				: temp - 1;								// range 0..N-1
		} // uSpecBitSet::ffs

		decltype( bits ) operator[]( unsigned int i ) const {
			assert( i == 0 );
			return bits;
		} // uSpecBitSet::operator[]
	}; // uSpecBitSet

	template<unsigned int Size> class uBitSetImpl< sizeof( uint32_t ) * CHAR_BIT, Size > :
		public uSpecBitSet< sizeof( uint32_t ) * CHAR_BIT, Size > {};

	template<unsigned int Size> class uBitSetImpl< sizeof( uint64_t ) * CHAR_BIT, Size > :
		public uSpecBitSet< sizeof( uint64_t ) * CHAR_BIT, Size > {};

	#if _GLIBCXX_USE_INT128 == 1
	template<unsigned int Size> class uBitSetImpl< sizeof( unsigned __int128 ) * CHAR_BIT, Size > :
		public uSpecBitSet< sizeof( unsigned __int128 ) * CHAR_BIT, Size > {};
	#endif // _GLIBCXX_USE_INT128 == 1
} // UPP

template<unsigned int N> class uBitSet : public UPP::uBitSetImpl<
	N <= sizeof( uint32_t ) * CHAR_BIT ? sizeof( uint32_t ) * CHAR_BIT :
	N <= sizeof( uint64_t ) * CHAR_BIT ? sizeof( uint64_t ) * CHAR_BIT :
	#if _GLIBCXX_USE_INT128 == 1
	N <= sizeof( unsigned __int128 ) * CHAR_BIT ? sizeof( unsigned __int128 ) * CHAR_BIT :
	#endif // _GLIBCXX_USE_INT128 == 1
	N, N> {
  public:
	unsigned int size() const { return N; };
};


// Local Variables: //
// compile-command: "make install" //
// End: //
