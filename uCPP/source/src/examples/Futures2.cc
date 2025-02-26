// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2015
// 
// Futures2.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Fri Dec  2 16:20:24 2022
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Dec  2 16:21:24 2022
// Update Count     : 1
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
using namespace std;
#include <uFuture.h>

Future_ISM<int> fi;
Future_ISM<int *> fip;
Future_ISM<double> fd;
struct Msg { int i, j; };
Future_ISM<Msg> fm;
Future_ISM<Msg *> fmp;
struct Stop {};
Future_ISM<Stop> fs;
struct Cont {};
Future_ISM<Cont> fc;

int x = 7;
Msg fred{ 12, 15 };

_Task Worker {
	void main() {
		for ( ;; ) {
			_Select( fi ) { cout << fi() << endl; fi.reset(); }
			and _Select( fip ) { cout << fip() << " " << *fip() << endl; fip.reset(); }
			and _Select( fd ) { cout << fd() << endl; fd.reset(); }
			and _Select( fm ) { cout << fm().i << " " << fm().j << endl; fm.reset(); }
			and _Select( fmp ) { cout << fmp() << " " << fmp()->i << " " << fmp()->j << endl; fmp.reset(); }
			or _Select( fs ) { cout << "stop" << endl; break; }
			fc.delivery( (Cont){} );					// synchronize
		} // for
	} // Worker::main
}; // Worker

int main() {
	Worker worker;
	uProcessor p;
	for ( int i = 0; i < 10; i += 1 ) {
		fi.delivery( i );
		fip.delivery( &x );
		fd.delivery( i + 2.5 );
		fm.delivery( (Msg){ i, 2 } );
		fmp.delivery( &fred );
		fc(); fc.reset();								// wait for 3 futures to be processed
		cout << endl;
	}
	fs.delivery( (Stop){} );
} // wait for worker to terminate
