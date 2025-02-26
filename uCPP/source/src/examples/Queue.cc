#include <iostream>
using namespace std;
#include <uQueue.h>

int main() {
	// Fred test

	struct Fred : public uColable {
		int i;
		Fred() { abort(); }
		Fred( int p ) : i( p ) {}
	};

	uQueue<Fred> fred;
	uQueueIter<Fred> fredIter( fred );
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

	for ( uQueueIter<Fred> iter( fred ); iter >> f; ) {
		cout << f->i << ' ';
	}
	cout << endl;

	for ( i = 0; i < 9; i += 1 ) {
		delete fred.drop();
	}

	for ( fredIter.over( fred ); fredIter >> f; ) {
		cout << f->i << ' ';
	}
	cout << endl;
	
	for ( i = 0; i < 10; i += 1 ) {
		fred.add( new Fred( 2 * i + 1 ) );
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
		fred.add( new Fred( i ) );
		if ( i == 4 ) {
			middle = fred.tail();
		}
	}
	for ( uQueueIter<Fred> iter( fred ); iter >> f; ) {
		cout << f->i << ' ';
	}
	cout << endl;

	uQueue<Fred> fred2;

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

	uQueue<Mary> mary;
	uQueueIter<Mary> maryIter( mary );
	Mary * m;

	for ( ; maryIter >> m; ) {							// empty list
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << "empty" << endl;
	
	for ( i = 0; i < 10; i += 1 ) {
		mary.add( new Mary( 2 * i ) );
	}

	cout << mary.head()->i << ' ' << mary.tail()->i << endl;

	for ( uQueueIter<Mary> iter( mary ); iter >> m; ) {
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << endl;
	
	for ( i = 0; i < 9; i += 1 ) {
		delete mary.drop();
	}

	for ( maryIter.over( mary ); maryIter >> m; ) {
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << endl;
	
	for ( i = 0; i < 10; i += 1 ) {
		mary.add( new Mary( 2 * i + 1 ) );
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
	for ( uQueueIter<Mary> iter( mary ); iter >> m; ) {
		cout << m->i << ' ' << m->j << ' ';
	}
	cout << endl;

	uQueue<Mary> mary2;

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
