//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Richard A. Stroobosscher 1994
//
// main.h --
//
// Author           : Richard A. Stroobosscher
// Created On       : Tue Apr 28 16:06:46 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Dec 24 18:07:39 2021
// Update Count     : 32
//


#pragma once


#include <iostream>

using std::istream;
using std::ostream;

extern istream * yyin;
extern ostream * yyout;

extern bool error;
extern bool profile;
extern bool stdcpp11;
extern bool user;

int main( int argc, char * argv[] );



// Local Variables: //
// compile-command: "make install" //
// End: //
