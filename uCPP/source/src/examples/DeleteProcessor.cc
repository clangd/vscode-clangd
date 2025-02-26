//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2010
// 
// DeleteProcessor.cc -- 
// 
// Author           : Peter A. Buhr
// Created On       : Sun Jul 18 11:03:54 2010
// Last Modified By : Peter A. Buhr
// Last Modified On : Sun Apr 24 09:50:18 2022
// Update Count     : 7
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

_Task worker {
	void main() {
		migrate( clus );
		cout << "Task " << &uThisTask() << " deleting processor " << &uThisProcessor() << "\n";
		delete &uThisProcessor();
		cout << "Task " << &uThisTask() << " now on processor " << &uThisProcessor() << "\n";
		yield();
		cout << "Task " << &uThisTask() << " still on processor " << &uThisProcessor() << "\n";
	}

	uCluster &clus;
  public:
	worker( uCluster &cl ): clus( cl ) {}
}; // worker

int main(){
	uCluster cluster;
	uProcessor *processor = new uProcessor( cluster );
	cout << "Task main created processor " << processor << "\n";
	{
		worker f( cluster );
		sleep( 2 );
		processor = new uProcessor( cluster );
		cout << "Task main created processor " << processor << "\n";
	}
	delete processor;
	cout << "successful completion\n";
}
