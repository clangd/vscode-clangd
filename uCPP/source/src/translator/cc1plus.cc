//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr 2003
//
// cc1plus.cc --
//
// Author           : Peter A Buhr
// Created On       : Tue Feb 25 09:04:44 2003
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Sep 25 10:46:27 2023
// Update Count     : 350
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
#include <string>
#include <algorithm>									// find
#include <cstdio>										// stderr, stdout, perror, fprintf
#include <cstdlib>										// getenv, exit, mkstemp
using namespace std;
#include <unistd.h>										// execvp, fork, unlink
#include <sys/wait.h>									// wait
#include <fcntl.h>										// creat

//#define __U_DEBUG_H__
#include "debug.h"

//#define CLANG
static string compiler_path( CCAPP );					// path/name of C compiler
static bool UPP_flag = false;							// -U++ flag
static bool save_temps = false;							// -save-temps flag
static string o_file;
static string bprefix;
static string lang;										// -x flag


static bool prefix( const string & arg, const string & pre ) {
	return arg.substr( 0, pre.size() ) == pre;
} // prefix


static string __UPP_FLAGPREFIX__( "__UPP_FLAG" );		// "__UPP_FLAG__=" suffix

static void checkEnv1() {								// stage 1
	extern char ** environ;

	for ( int i = 0; environ[i]; i += 1 ) {
		string arg( environ[i] );
		uDEBUGPRT( cerr << "env arg[" << i << "]:\"" << arg << "\"" << endl; );

		if ( prefix( arg, __UPP_FLAGPREFIX__ ) ) {
			string val( arg.substr( arg.find_first_of( "=" ) + 1 ) );
			if ( prefix( val, "-compiler=" ) ) {
				compiler_path = val.substr( 10 );
			} else if ( prefix( val, "-x=" ) ) {
				lang = val.substr( 3 );
			} // if
		} // if
	} // for
} // checkEnv1


static void checkEnv2( const char * args[], int & nargs ) { // stage 2
	extern char ** environ;

	for ( int i = 0; environ[i]; i += 1 ) {
		string arg( environ[i] );
		uDEBUGPRT( cerr << "env arg[" << i << "]:\"" << arg << "\"" << endl; );

		if ( prefix( arg, __UPP_FLAGPREFIX__ ) ) {
			string val( arg.substr( arg.find_first_of( "=" ) + 1 ) );
			if ( prefix( val, "-compiler=" ) ) {
				compiler_path = val.substr( 10 );
			} else if ( val == "-UPP" ) {
				UPP_flag = true;
			} else if ( val == "-save-temps" ) {
				save_temps = true;
			} else if ( prefix( val, "-o=" ) ) {		// output file for -UPP
				o_file = val.substr( 3 );
			} else if ( prefix( val, "-B=" ) ) {		// location of u++-cpp
				bprefix = val.substr( 3 );
			} else if ( prefix( val, "-x=" ) ) {		// ignore
			} else {									// normal flag for u++-cpp
				args[nargs++] = ( *new string( arg.substr( arg.find_first_of( "=" ) + 1 ) ) ).c_str();
			} // if
		} // if
	} // for
} // checkEnv2

#define UPP_SUFFIX ".iupp"

static char tmpname[] = P_tmpdir "/uC++XXXXXX" UPP_SUFFIX;
static int tmpfilefd = -1;
static bool startrm = false;

static void rmtmpfile() {
	if ( tmpfilefd == -1 ) return;						// RACE, file created ?

	startrm = true;										// RACE with C-c C-c
	if ( unlink( tmpname ) == -1 ) {					// remove tmpname
		perror ( "CC1plus Translator error: failed, unlink" );
		exit( EXIT_FAILURE );
	} // if
	tmpfilefd = -1;										// mark removed
} // rmtmpfile


static void sigTermHandler( int ) {						// C-c C-c
	if ( startrm ) return;								// return and let rmtmpfile finish, and then program finishes

	if ( tmpfilefd != -1 ) {							// RACE, file created ?
		rmtmpfile();									// remove tmpname
	} // if
	exit( EXIT_FAILURE );								// terminate
} // sigTermHandler


static void Stage1( const int argc, const char * const argv[] ) {
	int code;
	string arg;

	const char * cpp_in = nullptr;
	const char * cpp_out = nullptr;

	bool cpp_flag = false;
	bool o_flag = false;
//	const char * o_name = nullptr;

	const char * args[argc + 100];						// leave space for 100 additional cpp command line values
	int nargs = 1;										// number of arguments in args list; 0 => command name

	uDEBUGPRT( cerr << "#########" << endl << "Stage1 " << string( 100, '#' ) << endl << "#########" << endl; );
	checkEnv1();										// arguments passed via environment variables
	uDEBUGPRT( cerr << string( 100, '*' ) << endl; );
	uDEBUGPRT(
		for ( int i = 1; i < argc; i += 1 ) {
			cerr << "argv[" << i << "]:\"" << argv[i] << "\"" << endl;
		} // for
	);
	uDEBUGPRT( cerr << string( 100, '*' ) << endl; );

	// process all the arguments

	for ( int i = 1; i < argc; i += 1 ) {
		arg = argv[i];
		if ( prefix( arg, "-" ) ) {
			// strip g++ flags that are inappropriate or cause duplicates in subsequent passes

			if ( arg == "-quiet" ) {
			} else if ( arg == "-imultilib" || arg == "-imultiarch" ) {
				i += 1;									// and argument
			} else if ( prefix( arg, "-A" ) ) {
			} else if ( prefix( arg, "-D__GNU" ) ) {
				//********
				// GCC 5.6.0 SEPARATED THE -D FROM THE ARGUMENT!
				//********
			} else if ( arg == "-D" && prefix( argv[i + 1], "__GNU" ) ) {
				i += 1;									// and argument

				// strip flags controlling cpp step

			} else if ( arg == "-D__U_CPP__" ) {
				cpp_flag = true;
			} else if ( arg == "-D" && string( argv[i + 1] ) == "__U_CPP__" ) {
				i += 1;									// and argument
				cpp_flag = true;

				// all other flags

			#ifdef CLANG
			} else if ( arg == "-fworking-directory" ) {
			#endif // CLANG
			} else if ( arg == "-o" ) {
				i += 1;
				o_flag = true;
				cpp_out = argv[i];
			} else {
				args[nargs++] = argv[i];				// pass flag along
				// CPP flags with an argument
				if ( arg == "-D" || arg == "-U" || arg == "-I" || arg == "-MF" || arg == "-MT" || arg == "-MQ" ||
					 arg == "-include" || arg == "-imacros" || arg == "-idirafter" || arg == "-iprefix" ||
					 arg == "-iwithprefix" || arg == "-iwithprefixbefore" || arg == "-isystem" || arg == "-isysroot" ||
					 arg == "-dumpbase-ext" || arg == "-dumpbase"
					) {
					i += 1;
					args[nargs++] = argv[i];			// pass argument along
				} else if ( arg == "-MD" || arg == "-MMD" ) {
					// gcc frontend generates the dependency file-name after the -MD/-MMD flag, but it is necessary to
					// prefix that file name with -MF.
					args[nargs++] = "-MF";				// insert before file
					i += 1;
					args[nargs++] = argv[i];			// pass argument along
				} // if
			} // if
		} else {										// obtain input and possibly output files
			if ( cpp_in == nullptr ) {
				cpp_in = argv[i];
				uDEBUGPRT( cerr << "cpp_in:\"" << cpp_in << "\"" << endl; );
			} else if ( cpp_out == nullptr ) {
				cpp_out = argv[i];
				uDEBUGPRT( cerr << "cpp_out:\"" << cpp_out << "\""<< endl; );
			} else {
				cerr << "Usage: " << argv[0] << " input-file [output-file] [options]" << endl;
				exit( EXIT_FAILURE );
			} // if
		} // if
	} // for

	uDEBUGPRT(
		cerr << "args:";
		for ( int i = 1; i < nargs; i += 1 ) {
			cerr << " " << args[i];
		} // for
		if ( cpp_in != nullptr ) cerr << " " << cpp_in;
		if ( cpp_out != nullptr ) cerr << " " << cpp_out;
		cerr << endl;
	);

	if ( cpp_in == nullptr ) {
		cerr << "Usage: " << argv[0] << " input-file [output-file] [options]" << endl;
		exit( EXIT_FAILURE );
	} // if

	if ( cpp_flag ) {
		// The -E flag is specified on the u++ command so only run the preprocessor and output is written to standard
		// output or -o. The call to u++ has a -E so it does not have to be added to the argument list.

		args[0] = compiler_path.c_str();
		if ( lang.size() != 0 ) {
			args[nargs++] = "-x";
			args[nargs++] = ( *new string( lang ) ).c_str();
		} // if
		args[nargs++] = cpp_in;
		if ( o_flag ) {									// location for output
			args[nargs++] = "-o";
		} // if
		args[nargs++] = cpp_out;
		args[nargs] = nullptr;							// terminate argument list

		uDEBUGPRT(
			cerr << "nargs: " << nargs << endl;
			for ( int i = 0; args[i] != nullptr; i += 1 ) {
				cerr << args[i] << " ";
			} // for
			cerr << endl;
		);
		uDEBUGPRT( cerr << string( 100, '*' ) << endl; );

		execvp( args[0], (char * const *)args );		// should not return
		perror( "CC1plus Translator error: stage 1, execvp" );
		exit( EXIT_FAILURE );
	} // if

	// Run the C preprocessor and save the output in the given file.

	if ( fork() == 0 ) {								// child process ?
		// -o xxx.ii cannot be used to write the output file from cpp because no output file is created if cpp detects
		// an error (e.g., cannot find include file). Whereas, output is always generated, even when there is an error,
		// when cpp writes to stdout. Hence, stdout is redirected into the temporary file.
		if ( freopen( cpp_out, "w", stdout ) == nullptr ) { // redirect stdout to output file
			perror( "CC1plus Translator error: stage 1, freopen" );
			exit( EXIT_FAILURE );
		} // if

		#ifdef CLANG
		args[0] = "clang++-6.0";
		#else
		args[0] = compiler_path.c_str();
		#endif // CLANG
		if ( lang.size() != 0 ) {
			args[nargs++] = "-x";
			args[nargs++] = ( *new string( lang ) ).c_str();
		} // if
		args[nargs++] = cpp_in;							// input to cpp
		args[nargs] = nullptr;							// terminate argument list

		uDEBUGPRT(
			cerr << "cpp nargs: " << nargs << endl;
			for ( int i = 0; args[i] != nullptr; i += 1 ) {
				cerr << args[i] << " ";
			} // for
			cerr << endl;
		);

		execvp( args[0], (char * const *)args );		// should not return
		perror( "CC1plus Translator error: stage 1 cpp, execvp" );
		cerr << " invoked " << args[0] << endl;
		exit( EXIT_FAILURE );
	} // if

	wait( &code );										// wait for child to finish

	uDEBUGPRT( cerr << "return code from cpp:" << WEXITSTATUS(code) << endl; );

	if ( WIFSIGNALED(code) ) {							// child failed ?
		rmtmpfile();									// remove tmpname
		cerr << "CC1plus Translator error: stage 1, child failed " << WTERMSIG(code) << endl;
		exit( EXIT_FAILURE );
	} // if

	exit( WEXITSTATUS( code ) );						// bad cpp result stops top-level gcc
} // Stage1


static void Stage2( const int argc, const char * const * argv ) {
	int code;
	string arg;

	const char * cpp_in = nullptr;
	const char * cpp_out = nullptr;

	const char * args[argc + 100];						// leave space for 100 additional u++ command line values
	int nargs = 1;										// number of arguments in args list; 0 => command name
	const char * cargs[20];								// leave space for 20 additional u++-cpp command line values
	int ncargs = 1;										// 0 => command name

	uDEBUGPRT( cerr << "#########" << endl << "Stage2 " << string( 100, '#' ) << endl << "#########" << endl; );
	checkEnv2( cargs, ncargs );							// arguments passed via environment variables
	uDEBUGPRT( cerr << string( 100, '*' ) << endl; );
	uDEBUGPRT(
		for ( int i = 1; i < argc; i += 1 ) {
			cerr << "argv[" << i << "]:\"" << argv[i] << "\"" << endl;
		} // for
	);
	uDEBUGPRT( cerr << string( 100, '*' ) << endl; );

	// process all the arguments

	uDEBUGPRT( cerr << "start processing arguments" << endl; );
	for ( int i = 1; i < argc; i += 1 ) {
		uDEBUGPRT( cerr << "argv[" << i << "]:\"" << argv[i] << "\"" << endl; );
		arg = argv[i];
		uDEBUGPRT( cerr << "arg:\"" << arg << "\"" << endl; );
		if ( prefix( arg, "-" ) ) {
			// strip inappropriate flags

			if ( arg == "-quiet" || arg == "-version" || arg == "-fpreprocessed" ||
				 // Currently uC++ does not suppose precompiled .h files.
				 prefix( arg, "--output-pch" ) ) {

				// strip inappropriate flags with an argument

			} else if ( arg == "-auxbase" || arg == "-auxbase-strip" ||
						arg == "-dumpbase" || arg == "-dumpbase-ext" || arg == "-dumpdir" ) {
				i += 1;
				uDEBUGPRT( cerr << "arg:\"" << argv[i] << "\"" << endl; );

				// u++ flags controlling the u++-cpp step

			} else if ( arg == "-D__U_PROFILE__" || arg == "-D__U_STD_CPP11__" ) {
				args[nargs++] = argv[i];				// pass the flag to cpp
				cargs[ncargs++] = argv[i];				// pass the flag to upp
			} else if ( arg == "-D" && ( string( argv[i + 1] ) == "__U_PROFILE__" || string( argv[i + 1] ) == "__U_STD_CPP11__" ) ) {
				args[nargs++] = argv[i];				// pass flag to cpp
				args[nargs++] = argv[i + 1];			// pass argument to cpp
				cargs[ncargs++] = argv[i];				// pass flag to upp
				cargs[ncargs++] = argv[i + 1];			// pass argument to upp
				i += 1;									// and argument

				// all other flags

			} else {
				args[nargs++] = argv[i];				// pass flag along
				if ( arg == "-o" ) {
					i += 1;
					cpp_out = argv[i];
					args[nargs++] = argv[i];			// pass argument along
					uDEBUGPRT( cerr << "arg:\"" << argv[i] << "\"" << endl; );
				} // if
			} // if
		} else {										// obtain input and possibly output files
			if ( cpp_in == nullptr ) {
				cpp_in = argv[i];
				uDEBUGPRT( cerr << "cpp_in:\"" << cpp_in << "\"" << endl; );
			} else if ( cpp_out == nullptr ) {
				cpp_out = argv[i];
				uDEBUGPRT( cerr << "cpp_out:\"" << cpp_out << "\""<< endl; );
			} else {
				cerr << "Usage: " << argv[0] << " more than two files specified" << endl;
				exit( EXIT_FAILURE );
			} // if
		} // if
	} // for
	uDEBUGPRT( cerr << "end processing arguments" << endl; );

	if ( cpp_in == nullptr ) {
		cerr << "Usage: " << argv[0] << " missing input file" << endl;
		exit( EXIT_FAILURE );
	} // if
	if ( cpp_out == nullptr ) {
		cerr << "Usage: " << argv[0] << " missing output file" << endl;
		exit( EXIT_FAILURE );
	} // if

	// Create a temporary file, if needed, to store output of the u++-cpp preprocessor. Cannot be created in forked
	// process because variables tmpname and tmpfilefd are cloned.

	string upp_cpp_out;

	if ( ! UPP_flag ) {									// run compiler ?
		if ( save_temps ) {
			upp_cpp_out = cpp_in;
			size_t dot = upp_cpp_out.find_last_of( "." );
			if ( dot == string::npos ) {
				cerr << "CC1plus Translator error: stage 2, bad file name " << endl;
				exit( EXIT_FAILURE );
			} // if

			upp_cpp_out = upp_cpp_out.substr( 0, dot ) + UPP_SUFFIX;
			if ( creat( upp_cpp_out.c_str(), 0666 ) == -1 ) {
				perror( "CC1plus Translator error: stage 2, creat" );
				exit( EXIT_FAILURE );
			} // if
		} else {
			tmpfilefd = mkstemps( tmpname, 5 );
			if ( tmpfilefd == -1 ) {
				perror( "CC1plus Translator error: stage 2, mkstemp" );
				exit( EXIT_FAILURE );
			} // if
			upp_cpp_out = tmpname;
		} // if
		uDEBUGPRT( cerr << "upp_cpp_out: " << upp_cpp_out << endl; );
	} // if

	// If -U++ flag specified, run the u++-cpp preprocessor on the temporary file, and output is written to standard
	// output.  Otherwise, run the u++-cpp preprocessor on the temporary file and save the result into the output file.

	if ( fork() == 0 ) {								// child runs uC++ preprocessor
		cargs[0] = ( *new string( bprefix + "u++-cpp" ) ).c_str();
		cargs[ncargs++] = cpp_in;

		if ( UPP_flag ) {								// run u++-cpp ?
			if ( o_file.size() != 0 ) {					// location for output
				cargs[ncargs++] = ( *new string( o_file ) ).c_str();
			} // if
		} else {
			cargs[ncargs++] = upp_cpp_out.c_str();
		} // if

		cargs[ncargs] = nullptr;						// terminate argument list

		uDEBUGPRT(
			for ( int i = 0; cargs[i] != nullptr; i += 1 ) {
				cerr << cargs[i] << " ";
			} // for
			cerr << endl;
		);

		execvp( cargs[0], (char * const *)cargs );		// should not return
		perror( "CC1plus Translator error: stage 2 u++-cpp, execvp" );
		cerr << " invoked " << cargs[0] << endl;
		exit( EXIT_FAILURE );
	} // if

	wait( &code );										// wait for child to finish

	if ( WIFSIGNALED(code) ) {							// child failed ?
		rmtmpfile();									// remove tmpname
		cerr << "CC1plus Translator error: stage 2, child failed " << WTERMSIG(code) << endl;
		exit( EXIT_FAILURE );
	} // if

	if ( UPP_flag ) {									// no tmpfile created
		exit( WEXITSTATUS( code ) );					// stop regardless of success or failure
	} // if

	if ( WEXITSTATUS(code) ) {							// child error ?
		rmtmpfile();									// remove tmpname
		exit( WEXITSTATUS( code ) );					// do not continue
	} // if

	uDEBUGPRT(
		cerr << "args:";
		for ( int i = 1; i < nargs; i += 1 ) {
			cerr << " " << args[i];
		} // for
		cerr << " " << cpp_in << endl;
	);

	if ( fork() == 0 ) {								// child runs g++
		args[0] = compiler_path.c_str();
		args[nargs++] = "-S";							// only compile and put assembler output in specified file
		args[nargs++] = "-x";
		args[nargs++] = "c++-cpp-output";

		args[nargs++] = upp_cpp_out.c_str();
		args[nargs] = nullptr;							// terminate argument list

		uDEBUGPRT(
			cerr << "stage2 nargs: " << nargs << endl;
			for ( int i = 0; args[i] != nullptr; i += 1 ) {
				cerr << args[i] << " ";
			} // for
			cerr << endl;
		);

		execvp( args[0], (char * const *)args );		// should not return
		perror( "CC1plus Translator error: stage 2 cc1, execvp" );
		cerr << " invoked " << args[0] << endl;
		exit( EXIT_FAILURE );							// tell gcc not to go any further
	} // if

	wait( &code );										// wait for child to finish
	rmtmpfile();										// remove tmpname

	if ( WIFSIGNALED(code) ) {							// child failed ?
		cerr << "CC1plus Translator error: stage 2, child failed " << WTERMSIG(code) << endl;
		exit( EXIT_FAILURE );
	} // if

	uDEBUGPRT( cerr << "return code from gcc cc1:" << WEXITSTATUS(code) << endl; );

	exit( WEXITSTATUS( code ) );						// stop regardless of success or failure
} // Stage2


// This program is called twice because of the -no-integrated-cpp. The calls are differentiated by the first
// command-line argument. The first call replaces the traditional cpp pass to preprocess the C program. The second call
// is to the compiler, which is broken into two steps: preprocess again with u++-cpp and then call gcc to compile the
// doubly preprocessed program.

int main( const int argc, const char * const argv[], __attribute__((unused)) const char * const env[] ) {
	uDEBUGPRT( cerr << "#########" << endl << "main cc1plus " << string( 100, '#' ) << endl << "#########" << endl; );

	signal( SIGINT,  sigTermHandler );
	signal( SIGTERM, sigTermHandler );

	string arg( argv[1] );

	// Currently, stage 1 starts with flag -E and stage 2 with flag -fpreprocessed.

	if ( arg == "-E" ) {
		Stage1( argc, argv );
	} else if ( arg == "-fpreprocessed" ) {
		Stage2( argc, argv );
	} else {
		cerr << "Usage: " << argv[0] << " [-E input-file [output-file] ] | [-fpreprocessed input-file output-file] [options]" << endl;
		exit( EXIT_FAILURE );
	} // if
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "make install" //
// End: //
