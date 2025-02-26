#include <iostream>
using namespace std;
#include <uSequence.h>

int main() {
	// Fred test

	struct Fred : public uSeqable {
		int i;
		Fred() { abort(); }
		Fred( int p ) : i( p ) {}
	};

	uSequence<Fred> fred;
	uSeqIter<Fred> fredIter( fred );
	Fred * f;
	int i;

	for ( ; fredIter >> f; ) {							// empty list
		cout << f->i << ' ';
	}
	cout << "empty" << endl;
	
	for ( i = 0; i < 10; i += 1 ) {
		fred.add( new Fred( 2 * i ) );
	}

	cout << fred.head()->i << ' ' << fred.tail()->i << endl;

	for ( uSeqIter<Fred> iter( fred ); iter >> f; ) {
		cout << f->i << ' ';
	}
	cout << endl;

	for ( i = 0; i < 9; i += 1 ) {
		delete fred.dropHead();
	}

	for ( fredIter.over( fred ); fredIter >> f; ) {
		cout << f->i << ' ';
	}
	cout << endl;
	
	for ( i = 0; i < 10; i += 1 ) {
		fred.addTail( new Fred( 2 * i + 1 ) );
	}

	Fred * head = new Fred( -1 ), tail( -2 );
	fred.addHead( head );
	fred.addTail( &tail );

	cout << fred.head()->i << ' ' << fred.succ( head )->i << ' ' << fred.tail()->i << endl;

	for ( fredIter.over( fred ); fredIter >> f; ) {
		cout << f->i << ' ';
	}
	cout << endl;

	fred.remove( head );
	fred.remove( &tail );
	delete head;
	delete fred.dropTail();

	for ( fredIter.over( fred ); fredIter >> f; ) {
		cout << f->i << ' ';
	}
	cout << endl;

	for ( i = 0; i < 5; i += 1 ) {
		delete fred.dropTail();
	}
	for ( fredIter.over( fred ); fredIter >> f; ) {
		cout << f->i << ' ';
	}
	cout << endl;

	for ( fredIter.over( fred ); fredIter >> f; ) {
		delete fred.remove( f );
	}
	for ( fredIter.over( fred ); fredIter >> f; ) {
		cout << f->i << ' ';
	}
	cout << "empty" << endl;

	Fred * middle = nullptr;			// stop uninitialized warning

	for ( i = 0; i < 10; i += 1 ) {
		fred.addHead( new Fred( i ) );					// reverse order
		if ( i == 5 ) {
			middle = fred.head();
		}
	}
	for ( uSeqIterRev<Fred> iter( fred ); iter >> f; ) {
		cout << f->i << ' ';
	}
	cout << endl;

	head = new Fred( -1 );
	fred.insertBef( head, fred.head() );
	fred.insertAft( head, &tail );

	cout << fred.head()->i << ' ' << fred.succ( head )->i << ' ' << fred.tail()->i << endl;
	for ( fredIter.over( fred ); fredIter >> f; ) {
		cout << f->i << ' ';
	}
	cout << endl;

	fred.remove( head );
	fred.remove( &tail );
	delete head;
	delete fred.dropTail();

	for ( fredIter.over( fred ); fredIter >> f; ) {
		cout << f->i << ' ';
	}
	cout << endl;

	uSequence<Fred> fred2;

	fred2.split( fred, middle );

	for ( fredIter.over( fred ); fredIter >> f; ) {
		cout << f->i << ' ';
	}
	cout << endl;

	for ( fredIter.over( fred2 ); fredIter >> f; ) {
		cout << f->i << ' ';
	}
	cout << endl;

	fred.transfer( fred2 );

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

	uSequence<Mary> mary;
	uSeqIter<Mary> maryIter( mary );
	Mary * m;

	for ( ; maryIter >> m; ) {							// empty list
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << "empty" << endl;
	
	for ( i = 0; i < 10; i += 1 ) {
		mary.add( new Mary( 2 * i ) );
	}

	cout << mary.head()->i << ' ' << mary.tail()->i << endl;

	for ( uSeqIter<Mary> iter( mary ); iter >> m; ) {
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << endl;
	
	for ( i = 0; i < 9; i += 1 ) {
		delete mary.dropHead();
	}

	for ( maryIter.over( mary ); maryIter >> m; ) {
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << endl;
	
	for ( i = 0; i < 10; i += 1 ) {
		mary.addTail( new Mary( 2 * i + 1 ) );
	}

	Mary * headm = new Mary( -1 ), tailm( -2 );
	mary.addHead( headm );
	mary.addTail( &tailm );

	cout << mary.head()->i << ' ' << mary.succ( headm )->i << ' ' << mary.tail()->i << endl;

	for ( maryIter.over( mary ); maryIter >> m; ) {
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << endl;

	mary.remove( headm );
	mary.remove( &tailm );
	delete headm;
	delete mary.dropTail();

	for ( maryIter.over( mary ); maryIter >> m; ) {
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << endl;

	for ( i = 0; i < 5; i += 1 ) {
		delete mary.dropTail();
	}
	for ( maryIter.over( mary ); maryIter >> m; ) {
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << endl;

	for ( maryIter.over( mary ); maryIter >> m; ) {
		delete mary.remove( m );
	}
	for ( maryIter.over( mary ); maryIter >> m; ) {
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << "empty" << endl;

	Mary * middlem = nullptr;			// stop uninitialized warning

	for ( i = 0; i < 10; i += 1 ) {
		mary.add( new Mary( i ) );
		if ( i == 4 ) {
			middlem = mary.tail();
		}
	}
	for ( uSeqIter<Mary> iter( mary ); iter >> m; ) {
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << endl;

	uSequence<Mary> mary2;

	mary2.split( mary, middlem );

	for ( maryIter.over( mary ); maryIter >> m; ) {
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << endl;
	for ( maryIter.over( mary2 ); maryIter >> m; ) {
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << endl;

	mary.transfer( mary2 );

	for ( maryIter.over( mary ); maryIter >> m; ) {
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << endl;

	for ( maryIter.over( mary ); maryIter >> m; ) {
		delete m;
	}
}
