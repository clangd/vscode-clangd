#include "uBitSet.h"
#include <iostream>
using namespace std;

#define NBITS 128

template< unsigned int n > void printBitSet( uBitSet< n > set ) {
	for ( unsigned int i = 0; i < n; i += 1 ) {
		if ( i % 8 == 0 && i != 0 ) cout << "_";
		cout << (set.isSet( i ) ? "T" : "F");
	} // for
	cout << endl;
}

#define SANITY_CHECK(set) if ( !set.isSet( set.ffs() ) ) abort()

int main() {
	cout << ffs(0) << " " << ffs(3) << " " << ffs(4) << " " << ffs( 8 ) << endl;
	uBitSet< NBITS > t;
	t.clrAll();
	printBitSet( t );
	cout << "first set is " << t.ffs() << endl;
	t.clrAll();
	t.set( 0 ); t.set( 1 );
	printBitSet( t );
	cout << "first set is " << t.ffs() << endl;
	t.clrAll();
	t.set( 2 );
	printBitSet( t );
	cout << "first set is " << t.ffs() << endl;
	t.clrAll();
	t.set( 3 );
	printBitSet( t );
	cout << "first set is " << t.ffs() << endl;
	
	uBitSet< NBITS > a, b;
	a.setAll();
	cout << "a: ";
	printBitSet( a );
	cout << "first set is " << a.ffs() << endl;
	a.clr( 0 );
	a.clr( 4 );
	a.clr( a.size() - 1 );
	a.clr( 31 );
	a.clr( a.size() / 2 );
	cout << "a: ";
	printBitSet( a );
	cout << "first set is " << a.ffs() << endl;

	b.clrAll();
	cout << "b: ";
	printBitSet( b );
	cout << "first set is " << b.ffs() << endl;
	b.set( b.size() - 1 );
	b.set( b.size() / 2 );
	cout << "b: ";
	printBitSet( b );
	cout << "first set is " << b.ffs() << endl;
	b.clr( b.size() / 2 );
	cout << "b: ";
	printBitSet( b );
	cout << "first set is " << b.ffs() << endl;
}

// Local Variables: //
// compile-command: "g++ -g -Wall example_BitSet.cc" //
// End: //
