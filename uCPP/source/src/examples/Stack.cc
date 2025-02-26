#include <iostream>
using namespace std;
#include <uStack.h>

int main() {
	// Fred test

	struct Fred : public uColable {
		int i;
		Fred() { abort(); }
		Fred( int p ) : i( p ) {}
	};

	uStack<Fred> fred;
	uStackIter<Fred> fredIter( fred );
	Fred * f;
	int i;

	for ( ; fredIter >> f; ) {							// empty list
		cout << f->i << ' ';
	}
	cout << "empty" << endl;
	
	for ( i = 0; i < 10; i += 1 ) {
		fred.push( new Fred( 2 * i ) );
	}

	for ( fredIter.over( fred ); fredIter >> f; ) {
		cout << f->i << ' ';
	}
	cout << endl;

	cout << fred.head()->i << endl;
	
	for ( i = 0; i < 9; i += 1 ) {
		delete fred.pop();
	}

	for ( fredIter.over( fred ); fredIter >> f; ) {
		cout << f->i << ' ';
	}
	cout << endl;
	
	for ( i = 0; i < 10; i += 1 ) {
		fred.push( new Fred( 2 * i + 1 ) );
	}
	for ( fredIter.over( fred ); fredIter >> f; ) {
		cout << f->i << ' ';
	}
	cout << endl;

	for ( fredIter.over( fred ); fredIter >> f; ) {
		delete f;
	}

	// Mary test

	struct Mary: public Fred {
		int j;
		Mary() { abort(); }
		Mary( int p ) : Fred( p ), j( p ) {}
	};

	uStack<Mary> mary;
	uStackIter<Mary> maryIter( mary );
	Mary * m;

	for ( ; maryIter >> m; ) {							// empty list
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << "empty" << endl;
	
	for ( i = 0; i < 10; i += 1 ) {
		mary.push( new Mary( 2 * i ) );
	}

	for ( maryIter.over( mary ); maryIter >> m; ) {
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << endl;
	
	for ( i = 0; i < 9; i += 1 ) {
		delete mary.pop();
	}

	for ( maryIter.over( mary ); maryIter >> m; ) {
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << endl;
	
	for ( i = 0; i < 10; i += 1 ) {
		mary.push( new Mary( 2 * i + 1 ) );
	}
	for ( maryIter.over( mary ); maryIter >> m; ) {
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << endl;

	for ( maryIter.over( mary ); maryIter >> m; ) {
		delete m;
	}
}
