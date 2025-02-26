#include "uFlexArray.h"
#include <iostream>
using namespace std;

int main() {
	uFlexArray<int> a( 1 ), b( 1 );
	int i;
	for ( i = 0; i < 6; i += 1 ) {
		a.add( i );
	} // for
	for ( i = 0; i < a.size(); i += 1 ) {
		cout << a[i] << endl;
	} // for
	b = a;
	for ( i = 0; i < b.size(); i += 1 ) {
		cout << b[i] << endl;
	} // for
	for ( i = 0; i < b.size(); i += 1 ) {
		b[i] += 1;
		cout << b[i] << endl;
	} // for
	b.remove(2);
	b.remove(4);
	for ( i = 0; i < b.size(); i += 1 ) {
		cout << b[i] << endl;
	} // for
}

// Local Variables: //
// compile-command: "u++ example_FlexArray.cc" //
// End: //
