//                              -*- Mode: C++ -*-
//
// uC++ Version 7.0.0, Copyright (C) Peter A. Buhr and Nikita Borisov 1995
//
// u++.cc --
//
// Author           : Nikita Borisov
// Created On       : Tue Apr 28 15:26:27 1992
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Sep 25 08:31:26 2023
// Update Count     : 1053
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
#include <fstream>										// ifstream
#include <cstdio>										// perror
#include <cstdlib>										// getenv, putenv, exit
#include <cstring>										// strcmp, strlen
#include <string>										// STL version

#include <unistd.h>										// execvp

using std::ifstream;
using std::cerr;
using std::endl;
using std::string;
using std::to_string;

//#define __U_DEBUG_H__
#include "debug.h"

#define xstr(s) str(s)
#define str(s) #s

static string __UPP_FLAGPREFIX__( "__UPP_FLAG" );		// "__UPP_FLAG__=" suffix

static void Putenv( char * argv[], string arg ) {
	// environment variables must have unique names
	static int flags = 0;

	uDEBUGPRT( cerr << "Putenv:" << string( __UPP_FLAGPREFIX__ + to_string( flags++ ) + "__=" ) + arg << endl; );
	if ( putenv( (char *)( *new string( string( __UPP_FLAGPREFIX__ + to_string( flags++ ) + "__=" ) + arg ) ).c_str() ) ) {
		cerr << argv[0] << " error, cannot set environment variable." << endl;
		exit( EXIT_FAILURE );
	} // if
} // Putenv

static bool prefix( const string & arg, const string & pre ) { // check if string has prefix
	return arg.substr( 0, pre.size() ) == pre;
} // prefix

static void shuffle( const char * args[], int S, int E, int N ) {
	// S & E index 1 passed the end so adjust with -1
	uDEBUGPRT( cerr << "shuffle:" << S << " " << E << " " << N << endl; );
	for ( int j = E-1 + N; j > S-1 + N; j -=1 ) {
		uDEBUGPRT( cerr << "\t" << j << " " << j-N << endl; );
		args[j] = args[j-N];
	} // for
} // shuffle


int main( int argc, char * argv[] ) {
	string Version( VERSION );							// current version number from CONFIG
	string Major( "0" ), Minor( "0" ), Patch( "0" );	// default version numbers
	int posn1 = Version.find( "." );					// find the divider between major and minor version numbers
	if ( posn1 == -1 ) {								// not there ?
		Major = Version;
	} else {
		Major = Version.substr( 0, posn1 );
		int posn2 = Version.find( ".", posn1 + 1 );		// find the divider between minor and patch numbers
		if ( posn2 == -1 ) {							// not there ?
			Minor = Version.substr( posn1 );
		} else {
			Minor = Version.substr( posn1 + 1, posn2 - posn1 - 1 );
			Patch = Version.substr( posn2 + 1 );
		} // if
	} // if

	string installincdir( INSTALLINCDIR );				// fixed location of include files
	string installlibdir( INSTALLLIBDIR );				// fixed location of the cc1 and u++-cpp commands

	string tvendor( TVENDOR );
	string tos( TOS );
	string tcpu( TCPU );

	string Multi( MULTI );

	string heading;										// banner printed at start of u++ compilation
	string arg;											// current command-line argument during command-line parsing
	string bprefix;										// path where g++ looks for compiler steps
	string langstd;										// language standard

	string compiler_path( CCAPP );						// path/name of C compiler
	string compiler_name;								// name of C compiler

	bool x_flag = false;								// -x flag
	bool nonoptarg = false;								// no non-option arguments specified, i.e., no file names
	bool link = true;									// link stage occurring
	bool verbose = false;								// -v flag
	bool quiet = false;									// -quiet flag
	bool debug = true;									// -debug flag
	bool multi = false;									// -multi flag
	bool upp_flag = false;								// -U++ flag
	bool cpp_flag = false;								// -E or -M flag, preprocessor only
	bool std_flag = false;								// -std= flag
	bool nouinc = false;								// -no-u++-include: avoid "inc" directory
	bool debugging = false;								// -g flag
	bool openmp = false;								// -openmp flag
	bool profile = false;								// -profile flag
	bool exact = true;									// profile type: exact | statistical
	int o_file = 0;										// -o filename position

	const char * args[argc + 100];						// u++ command line values, plus some space for additional flags
	int sargs = 1;										// starting location for arguments in args list
	int nargs = sargs;									// number of arguments in args list; 0 => command name

	const char * libs[argc + 20];						// non-user libraries must come separately, plus some added libraries and flags
	int nlibs = 0;

	string tmppath;
	string mvdpath;
	string bfdincdir;
	string motifincdir;
	string motiflibdir;
	string uxincdir;
	string uxlibdir;
	string mvdincdir;
	string mvdlibdir;
	string useperfmonstr;
	string perfmonincdir;
	string perfmonlibdir;
	string unwindincdir;
	string unwindlibdir;
	string token;
	string allocator;									// user specified memory allocator

	uDEBUGPRT( cerr << "u++:" << endl; );
	uDEBUGPRT(
		for ( int i = 1; i < argc; i += 1 ) {
			cerr << "argv[" << i << "]:\"" << argv[i] << "\"" << endl;
		} // for
	);

	// process command-line arguments

	for ( int i = 1; i < argc; i += 1 ) {
		arg = argv[i];									// convert to string value
		if ( prefix( arg, "-" ) ) {
			// pass through arguments

			if ( arg == "-Xlinker" || arg == "-o" ) {
				args[nargs++] = argv[i];				// pass flag along
				i += 1;
				if ( i == argc ) continue;				// next argument available ?
				args[nargs++] = argv[i];				// pass argument along
				if ( arg == "-o" ) o_file = i;			// remember file

				// uC++ specific arguments

			} else if ( arg == "-U++" ) {
				upp_flag = true;						// strip the -U++ flag
				link = false;
				args[nargs++] = "-fsyntax-only";		// stop after stage 2
			} else if ( arg == "-debug" ) {
				debug = true;							// strip the debug flag
			} else if ( arg == "-nodebug" ) {
				debug = false;							// strip the nodebug flag
			} else if ( arg == "-multi" ) {
				multi = true;							// strip the multi flag
			} else if ( arg == "-nomulti" ) {
				multi = false;							// strip the nomulti flag
			} else if ( arg == "-quiet" ) {
				quiet = true;							// strip the quiet flag
			} else if ( arg == "-noquiet" ) {
				quiet = false;							// strip the noquiet flag
			} else if ( arg == "-no-u++-include" ) {
				nouinc = true;
			} else if ( arg == "-profile" ) {
				profile = true;							// strip the profile flag
				if ( i + 1 < argc ) {					// check next argument, if available
					if ( strcmp( argv[i + 1], "exact" ) == 0 ) { // default ?
						i += 1;							// skip argument
					} else if ( strcmp( argv[i + 1], "statistical" ) == 0 ) {
						exact = false;
						i += 1;							// skip argument
					} // if
				} // if
			} else if ( arg == "-noprofile" ) {
				profile = false;						// strip the noprofile flag
			} else if ( arg == "-compiler" ) {
				// use the user specified compiler
				i += 1;
				if ( i == argc ) continue;				// next argument available ?
				compiler_path = argv[i];
				Putenv( argv, arg + "=" + argv[i] );

				// C++ specific arguments

			} else if ( arg == "-v" ) {
				verbose = true;							// verbosity required
				args[nargs++] = argv[i];				// pass flag along
			} else if ( arg == "-g" ) {
				debugging = true;						// symbolic debugging required
				args[nargs++] = argv[i];				// pass flag along
			} else if ( arg == "-save-temps" ) {
				args[nargs++] = argv[i];				// pass flag along
				Putenv( argv, arg );					// save u++-cpp output
			} else if ( prefix( arg, "-x" ) ) {			// file suffix ?
				string lang;
				args[nargs++] = argv[i];				// pass flag along
				if ( arg.length() == 2 ) {				// separate argument ?
					i += 1;
					if ( i == argc ) continue;			// next argument available ?
					lang = argv[i];
					args[nargs++] = argv[i];			// pass argument along
				} else {
					lang = arg.substr( 2 );
				} // if
				if ( x_flag ) {
					cerr << argv[0] << " warning, only one -x flag per compile, ignoring subsequent flag." << endl;
				} else {
					x_flag = true;
					Putenv( argv, string( "-x=" ) + lang );
				} // if
			} else if ( prefix( arg, "-std=" ) || prefix( arg, "--std=" ) ) {
				std_flag = true;						// -std=XX provided
				args[nargs++] = argv[i];				// pass flag along
			} else if ( prefix( arg, "-B" ) ) {
				bprefix = arg.substr(2);				// strip the -B flag
			} else if ( arg == "-c" || arg == "-S" || arg == "-E" || arg == "-M" || arg == "-MM" ) {
				args[nargs++] = argv[i];				// pass flag along
				if ( arg == "-E" || arg == "-M" || arg == "-MM" ) {
					cpp_flag = true;					// cpp only
				} // if
				link = false;                           // no linkage required
			} else if ( arg == "-D" || arg == "-U" || arg == "-I" || arg == "-MF" || arg == "-MT" || arg == "-MQ" ||
						arg == "-include" || arg == "-imacros" || arg == "-idirafter" || arg == "-iprefix" ||
						arg == "-iwithprefix" || arg == "-iwithprefixbefore" || arg == "-isystem" || arg == "-isysroot" ) {
				args[nargs++] = argv[i];				// pass flag along
				i += 1;
				args[nargs++] = argv[i];				// pass argument along
			} else if ( prefix( arg, "-alloc" ) ) {
				// use the user specified memory allocator
				i += 1;
				if ( i == argc ) continue;				// next argument available ?
				allocator = argv[i];
			} else if ( arg[1] == 'l' ) {
				// if the user specifies a library, load it after user code
				libs[nlibs++] = argv[i];
			} else if ( arg == "-fopenmp" ) {
				openmp = true;							// openmp mode
//				args[nargs++] = argv[i];				// pass argument along
			} else {
				// concatenate any other arguments
				args[nargs++] = argv[i];
			} // if
		} else {
			args[nargs++] = argv[i];					// concatenate files
			nonoptarg = true;
		} // if
	} // for

	if ( tcpu == "x86_64" ) {
		args[nargs++] = "-mcx16";						// allow double-wide CAS
	} // if

	// ARM -mno-outline-atomics => use LL/SC instead of calls to atomic routines: __aarch64_swp_acq_rel, __aarch64_cas8_acq_rel
	// ARM -march=armv8.2-a+lse => generate Arm LSE extension instructions SWAP and CAS
	// https://community.arm.com/developer/tools-software/tools/b/tools-software-ides-blog/posts/making-the-most-of-the-arm-architecture-in-gcc-10
	if ( tcpu == "arm_64" ) {
		args[nargs++] = "-mno-outline-atomics";			// use ARM LL/SC instructions for atomics
		// args[nargs++] = "-march=armv8.2-a+lse";			// use ARM atomics instructions
	} // if

	uDEBUGPRT(
		cerr << "args:";
		for ( int i = 1; i < nargs; i += 1 ) {
			cerr << " " << args[i];
		} // for
		cerr << endl;
	);

	// -E flag stops at cc1 stage 1, so u++-cpp in cc1 stage 2 is never executed.
	if ( cpp_flag && upp_flag ) {
		upp_flag = false;
		cerr << argv[0] << " warning, both -E and -U++ flags specified, using -E and ignoring -U++." << endl;
	} // if

	string d;
	if ( debug ) {
		d = "-d";
	} // if

	string m;
	if ( multi ) {
		if ( Multi == "FALSE" ) {						// system support multiprocessor ?
			cerr << argv[0] << ": Warning -multi flag not support on this system." << endl;
		} else {
			m = "-m";
		} // if
	} // if

	// profiling

	if ( profile ) {									// read MVD configuration information needed for compilation and/or linking.
		char * mvdpathname = getenv( "MVDPATH" );		// get MVDPATH environment variable

		if ( mvdpathname == nullptr ) {
			cerr << argv[0] << ": Warning environment variable MVDPATH not set. Profiling disabled." << endl;
			profile = false;
		} else {
			mvdpath = mvdpathname;						// make string
			if ( mvdpathname[strlen( mvdpathname ) - 1] != '/' ) { // trailing slash ?
				mvdpath += "/";							// add slash
			} // if

			// Define lib and include directories
			uxincdir = mvdpath + "X11R6/include";
			uxlibdir = mvdpath + "X11R6/lib";

			// Read Motif and MVD lib and include directories from the MVD CONFIG file

			string configFilename = mvdpath + "CONFIG";
			ifstream configFile( configFilename.c_str() );

			if ( ! configFile.good() ) {
				cerr << argv[0] << ": Warning could not open file \"" << configFilename << "\". Profiling disabled." << endl;
				profile = false;
			} else {
				const int dirnum = 11;
				struct {
					const char * dirkind;
					int used;
					string * dirname;
				} dirs[dirnum] = {
					{ "INSTALLINCDIR", 1, &mvdincdir },
					{ "INSTALLLIBDIR", 1, &mvdlibdir },
					{ "BFDINCLUDEDIR", 1, &bfdincdir },
					{ "MOTIFINCLUDEDIR", 1, &motifincdir },
					{ "MOTIFLIBDIR", 1, &motiflibdir },
					{ "PERFMON", 1, &useperfmonstr },
					{ "PFMINCLUDEDIR", 1, &perfmonincdir },
					{ "PFMLIBDIR", 1, &perfmonlibdir },
					{ "UNWINDINCLUDEDIR", 1, &unwindincdir },
					{ "UNWINDLIBDIR", 1, &unwindlibdir },
					{ "TMPDIR", 1, &tmppath },
				};
				string dirkind, equal, dirname;
				int cnt, i;
				int numOfDir = 0;

				for ( cnt = 0 ; cnt < dirnum; cnt += 1 ) { // names can appear in any order
					for ( ;; ) {
						configFile >> dirkind;
						if ( configFile.eof() || configFile.fail() ) goto fini;
						for ( i = 0; i < dirnum && dirkind != dirs[i].dirkind; i += 1 ) {} // linear search
						if ( i < dirnum ) break;		// found a line to be parsed
					} // for
					configFile >> equal;
					if ( configFile.eof() || configFile.fail() || equal != "=" ) break;
					getline( configFile, dirname );		// could be empty
					if ( configFile.eof() || ! configFile.good() ) break;
					int p = dirname.find_first_not_of( " " ); // find position of 1st blank character
					if ( p == -1 ) p = dirname.length(); // any characters left ?
					dirname = dirname.substr( p );		// remove leading blanks

					numOfDir += dirs[i].used;			// handle repeats
					dirs[i].used = 0;
					*dirs[i].dirname = dirname;
					uDEBUGPRT( cerr << dirkind << equal << dirname << endl; );
				} // for
			  fini:
				if ( numOfDir != dirnum ) {
					profile = false;
					cerr << argv[0] << ": Warning file \"" << configFilename << "\" corrupt.  Profiling disabled." << endl;
				} // if
			} // if
		} // if
	} // if

	if ( link ) {
		// shift arguments to make room for special libraries

		int pargs = 0;
		if ( profile ) {
			pargs += 7;									// N profiler arguments added at beginning
		} // if
		shuffle( args, sargs, nargs, pargs );
		nargs += pargs;

		if ( profile ) {
			// link the profiling library before the user code
			args[sargs++] = "-u";
			args[sargs++] = "U_SMEXACTMENUWD";			// force profiler start-up widgets to be loaded
			args[sargs++] = "-u";
			args[sargs++] = "U_SMSTATSMENUWD";			// force profiler start-up widgets to be loaded
			args[sargs++] = "-u";
			args[sargs++] = "U_SMOTHERMENUWD";			// force profiler start-up widgets to be loaded
			// SKULLDUGGERY: Put the profiler library before the user code to force the linker to include the
			// no-profiled versions of compiled inlined routines from uC++.h.
			args[sargs++] = ( *new string( mvdlibdir + "/uProfile"  + m + d + ".a" ) ).c_str();
			// SKULLDUGGERY: Put the profiler library after the user code to force the linker to include the
			// -finstrument-functions, if there is any reference to them in the user code.
			args[nargs++] = ( *new string( mvdlibdir + "/uProfile"  + m + d + ".a" ) ).c_str();
			if ( ! debugging ) {						// add -g if not specified
				args[nargs++] = "-g";
			} // if
		} // if

		// override uDefaultProcessors for OpenMP -- must come before uKernel

		if ( openmp ) {
			args[nargs++] = ( *new string( installlibdir + "/uDefaultProcessors-OpenMP.o" ) ).c_str();
		} // if

		if ( allocator != "" ) {
			args[nargs++] = "-u";
			args[nargs++] = "malloc";					// force heap to be loaded
//			args[nargs++] = "-u";
//			args[nargs++] = "_ZN12uHeapControl11prepareTaskEP9uBaseTask";
			args[nargs++] = allocator.c_str();
		} // if

		// link with the correct version of the kernel module

		args[nargs++] = "-x";							// protect these files in case -x specified
		args[nargs++] = "none";
		args[nargs++] = ( *new string( installlibdir + "/uKernel" + m + d + ".a" ) ).c_str();
		args[nargs++] = ( *new string( installlibdir + "/uScheduler" + m + d + ".a" ) ).c_str();

		// link with the correct version of the local debugger module

		args[nargs++] = ( *new string( installlibdir + "/uLocalDebugger" + m + "-d.a" ) ).c_str();

		// link with the correct version of the library module

		args[nargs++] = ( *new string( installlibdir + "/uLibrary" + m + d + ".a" ) ).c_str();

		// link with the correct version of the profiler module

		args[nargs++] = ( *new string( installlibdir + "/uProfilerFunctionPointers" + ".o" ) ).c_str();

		// any machine specific libraries

		libs[nlibs++] = "-ldl";							// calls to dlsym/dlerror

		if ( profile ) {
			args[nargs++] = ( *new string( string("-L") + mvdlibdir ) ).c_str();
			args[nargs++] = ( *new string( string("-L") + uxlibdir ) ).c_str();
			if ( motiflibdir.length() != 0 ) {
				args[nargs++] = ( *new string( string("-L") + motiflibdir ) ).c_str();
			} // if
			args[nargs++] = "-L/usr/X11R6/lib";
			args[nargs++] = ( *new string( string("-Wl,-R,") + uxlibdir + ( motiflibdir.length() != 0 ? string(":") + motiflibdir : "" ) ) ).c_str();
			libs[nlibs++] = "-lXm";
			libs[nlibs++] = "-lX11";
			libs[nlibs++] = "-lXt";
			libs[nlibs++] = "-lSM";
			libs[nlibs++] = "-lICE";
		    // libs[nlibs++] = "-lXpm";
			libs[nlibs++] = "-lXext";
			libs[nlibs++] = ( *new string( string( "-luX" ) + m + d ) ).c_str();
			libs[nlibs++] = "-lm";
			libs[nlibs++] = "-lbfd";
			libs[nlibs++] = "-liberty";

			if ( perfmonlibdir.length() != 0 ) {
				args[nargs++] = ( *new string( string( "-L" ) + perfmonlibdir ) ).c_str();
				args[nargs++] = ( *new string( string("-Wl,-R,") + perfmonlibdir ) ).c_str();
			} // if
			if ( unwindlibdir.length() != 0 ) {
				args[nargs++] = ( *new string( string( "-L" ) + unwindlibdir ) ).c_str();
				args[nargs++] = ( *new string( string("-Wl,-R,") + unwindlibdir ) ).c_str();
			} // if
			if ( useperfmonstr == "PERFMON" ) {			// link in performance counter
	            libs[nlibs++] = "-lpfm";
			} else if ( useperfmonstr == "PERFMON3" ) {
	            libs[nlibs++] = "-lpfm3";
			} else if ( useperfmonstr == "PERFCTR" ) {
	            libs[nlibs++] = "-lperfctr";
			} // if
			if ( tos == "linux" && tcpu == "x86_64" ) {
	            libs[nlibs++] = "-lunwind";				// link in libunwind for backtraces
			} // if
		} // if

		if ( multi ) {
			libs[nlibs++] = "-pthread";
		} // if
		if ( tcpu == "x86_64" || tcpu == "arm_64" ) {
			libs[nlibs++] = "-latomic";					// allow double-wide CAS
		} // if

		if ( openmp ) {
			libs[nlibs++] = "-fopenmp";
		} // if
	} // if

	// The u++ translator is used to build the kernel and library support code to allow uC++ keywords, like _Coroutine,
	// _Task, _Mutex, etc. However, the translator does not need to make available the uC++ includes during kernel build
	// because the include directory (inc) is being created, so these directories and special includes are turned off
	// (see src/MakeTools). Kernel build is the only time this flag should be used.
	//
	// Note, when testing using -no-u++-include, the "inc" file is not present so special include files to adjust text
	// do not occur. Hence, other errors may occur. See the "library" directory special include files.

	if ( ! nouinc ) {
		// add the directory that contains the include files to the list of arguments after any user specified include
		// directives

		args[nargs++] = ( *new string( string("-I") + installincdir ) ).c_str();

		// automagically add uC++.h as the first include to each translation unit so users do not have to remember to do
		// this

		args[nargs++] = "-include";
		args[nargs++] = ( *new string( installincdir + string("/uC++.h") ) ).c_str();
	} // if

	if ( profile ) {
		args[nargs++] = ( *new string( string( "-I" ) + mvdincdir ) ).c_str();
		args[nargs++] = ( *new string( string( "-I" ) + uxincdir ) ).c_str();
		if ( motifincdir.length() != 0 ) {
			args[nargs++] = ( *new string( string( "-I" ) + motifincdir ) ).c_str();
		} // if
		if ( bfdincdir.length() != 0 ) {
			args[nargs++] = ( *new string( string( "-I" ) + bfdincdir ) ).c_str();
		} // if
	} // if

	// add the correct set of flags based on the type of compile this is

	args[nargs++] = ( *new string( string("-D__U_CPLUSPLUS__=") + Major ) ).c_str();
	args[nargs++] = ( *new string( string("-D__U_CPLUSPLUS_MINOR__=") + Minor ) ).c_str();
	args[nargs++] = ( *new string( string("-D__U_CPLUSPLUS_PATCH__=") + Patch ) ).c_str();

	if ( cpp_flag ) {
		args[nargs++] = "-D__U_CPP__";
	} // if

	if ( upp_flag ) {
		Putenv( argv, "-UPP" );
		if ( o_file ) Putenv( argv, string( "-o=" ) + argv[o_file] );
	} // if

	if ( multi ) {
		args[nargs++] = "-D__U_MULTI__";
		heading += " (multiple processor)";
	} else {
		heading += " (single processor)";
	} // if

	if ( debug ) {
		heading += " (debug)";
		args[nargs++] = "-D__U_DEBUG__";
	} else {
		heading += " (no debug)";
	} // if

	if ( profile ) {
	    heading += " (profile)";
		if ( exact ) {
			args[nargs++] = ( *new string( "-finstrument-functions" ) ).c_str();
		} // if
	    Putenv( argv, "-D__U_PROFILE__" );
	} else {
	    heading += " (no profile)";
	} // if

	if ( openmp ) {
		args[nargs++] = "-D__U_OPENMP__";
	} // if

	args[nargs++] = "-D__U_MAXENTRYBITS__=" xstr(__U_MAXENTRYBITS__); // has underscores because it is used to build translator

	args[nargs++] = "-D__U_WORDSIZE__=" xstr(WORDSIZE);

#if defined( STATISTICS )								// Kernel Statistics ?
	args[nargs++] = "-D__U_STATISTICS__";
#endif // STATISTICS

#if defined( AFFINITY )									// Thread Local Storage ?
	args[nargs++] = "-D__U_AFFINITY__";
#endif // AFFINITY

#if defined( BROKEN_CANCEL )							// TEMPORARY: old glibc has pthread_testcancel as throw()
	args[nargs++] = "-D__U_BROKEN_CANCEL__";
#endif // BROKEN_CANCEL

	if ( bprefix.length() == 0 ) {
		bprefix = installlibdir;
	} // if
	if ( bprefix[bprefix.length() - 1] != '/' ) bprefix += '/';
	Putenv( argv, string("-B=") + bprefix );

	args[nargs++] = "-Xlinker";							// used by backtrace
	args[nargs++] = "-export-dynamic";

	// execute the compilation command

	args[0] = compiler_path.c_str();					// set compiler command for exec
	// find actual name of the compiler independent of the path to it
	int p = compiler_path.find_last_of( '/' );			// scan r -> l for first '/'
	if ( p == -1 ) {
		compiler_name = compiler_path;
	} else {
		compiler_name = *new string( compiler_path.substr( p + 1 ) );
	} // if

	if ( prefix( compiler_name, "g++" ) ) {				// allow suffix on g++ name
		if ( ! std_flag && ! x_flag ) {
			args[nargs++] = ( *new string( string("-std=") + CPP11 ) ).c_str(); // default
		} // if
		Putenv( argv, "-D__U_STD_CPP11__" );
		args[nargs++] = "-no-integrated-cpp";
		args[nargs++] = ( *new string( string("-B") + bprefix ) ).c_str();
	} else {
		cerr << argv[0] << " error, compiler \"" << compiler_name << "\" unsupported." << endl;
		exit( EXIT_FAILURE );
	} // if

	// Add the uC++ definitions of vendor, cpu and os names to the compilation command.

	args[nargs++] = ( *new string( string("-D__") + TVENDOR + "__" ) ).c_str();
	args[nargs++] = ( *new string( string("-D__") + TCPU + "__" ) ).c_str();
	args[nargs++] = ( *new string( string("-D__") + TOS + "__" ) ).c_str();

	for ( int i = 0; i < nlibs; i += 1 ) {				// copy non-user libraries after all user libraries
		args[nargs++] = libs[i];
	} // for

	args[nargs] = nullptr;								// terminate

	uDEBUGPRT(
		cerr << "nargs: " << nargs << endl;
		cerr << "args:" << endl;
		for ( int i = 0; args[i] != nullptr; i += 1 ) {
			cerr << " \"" << args[i] << "\"" << endl;
		} // for
	);

	if ( ! quiet ) {
		cerr << "uC++ " << "Version " << Version << heading << endl;
	} // if

	if ( verbose ) {
		if ( argc == 2 ) exit( EXIT_SUCCESS );			// if only the -v flag is specified, do not invoke g++

		for ( int i = 0; args[i] != nullptr; i += 1 ) {
			cerr << args[i] << " ";
		} // for
		cerr << endl;
	} // if

	if ( ! nonoptarg ) {
		cerr << argv[0] << " error, no input files" << endl;
		exit( EXIT_FAILURE );
	} // if

	// execute the command and return the result

	execvp( args[0], (char * const *)args );			// should not return
	perror( "uC++ Translator error: u++ level, execvp" );
	exit( EXIT_FAILURE );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "make install" //
// End: //
