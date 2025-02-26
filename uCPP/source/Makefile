## Release version number; do not change.
VERSION = 7.0.0

## Define where the final targets are installed. If you wish to put the final
## pieces in different places, you MUST change these macros before you create
## the configuration file and before you build the package because these
## variables are used to hard-code addresses in some of the package modules.
## These variables can be changed directly in this file, or from the command
## line of the build, as in:
##
##    % make INSTALLPREFIX=/usr/local linux-x86
##    % make install
##
## or by setting an environment variable, as in (Bourne sh):
##
##    % INSTALLPREFIX=/usr/local; export INSTALLPREFIX
##    % make linux-x86
##    % make install
##
## The previous commands build uC++ and then install the package at location
## "/usr/local". The default location for installation is the current directory.
##
## For packaging it may be necessary to install the files into a temporary
## location.  In this case, specify the final install paths when building, and
## the temporary paths when installing, e.g.:
##
##    % make INSTALLPREFIX=/usr/local linux-x86
##    % make
##    % make INSTALLPREFIX=debian/tmp install
##

INSTALLPREFIX ?= ${shell pwd}
INSTALLINCDIR ?= ${INSTALLPREFIX}/inc
INSTALLLIBDIR ?= ${INSTALLPREFIX}/lib
INSTALLBINDIR ?= ${INSTALLPREFIX}/bin
INSTALLMANDIR ?= ${INSTALLPREFIX}/man
INSTALLDOCDIR ?= ${INSTALLPREFIX}/doc

## Define where temporary files are created during compilation and execution.

TMPDIR ?= /tmp

## Define the compiler used to build the uC++ commands (u++/cpp/u++-cpp), and
## the compiler used to build the uC++ runtime system and for compiling
## applications. Note, the CCAPP value is hard-coded into the "u++" command,
## but can be dynamically overridden using the "-compiler" flag on the "u++"
## command. Supported compilers: GNU g++.

CCBUILD ?= g++
CCAPP ?= ${CCBUILD}

## Define version of C++11 (-std=): c++11 (minimum), c++14, c++17, c++1y

CPP11 ?= c++17

## Define name of the uC++ translator so multiple u++ versions can coexist.

UPP ?= u++

## Define the maximum number of mutex methods in a monitor or task; N-1 mutex
## members, including destructor, and timeout. 128 is a good number for most
## uses, special optimization for architectures supporting word sizes 32/64/128.

MAXENTRYBITS ?= 128

## Define if the kernel is build with statistical counters

STATISTICS ?= TRUE

########################### END OF THINGS TO CHANGE ###########################


## Define the directory that contains the package as the current directory.

PACKAGE = ${shell pwd}

## Define where the configuration file lives.

CONFIG = ${PACKAGE}/CONFIG

## Ensure the correct shell is used

SHELL := /bin/sh

## Define the major recipes.

.SILENT : all test clean install uninstall

all \
clean \
install \
uninstall :
	## If the configuration file is not yet built, instruct
	## the user how to create one.

	if [ ! -f ${CONFIG} ] ; then \
		echo "" ; \
		echo "The file ${CONFIG} was not found." ; \
		echo "Create file ${CONFIG} with one of the following commands..." ; \
		echo "" ; \
		echo "	${MAKE} linux-arm_64" ; \
		echo "	${MAKE} linux-x86_32" ; \
		echo "	${MAKE} linux-x86_64" ; \
		echo "" ; \
		exit 1 ; \
	fi

#		echo "	${MAKE} linux-ia64" ; \
#		echo "	${MAKE} linux-x86" ; \

#		echo "	${MAKE} solaris-sparc" ; \
#		echo "	${MAKE} freebsd-x86" ; \
#		echo "	${MAKE} freebsd-x86_64" ; \

#		echo "	${MAKE} irix-mips" ; \
#		echo "	${MAKE} linux-i386" ; \
#		echo "	${MAKE} dec-alpha" ; \
#		echo "	${MAKE} ibm-rs6000" ; \
#		echo "	${MAKE} hp-pa" ; \
#		echo "	${MAKE} sequent-i386-ptx" ; \
#		echo "	${MAKE} sun-sparc-sunos" ; \
#		echo "	${MAKE} sun-m68k-sunos" ; \
#		echo "	${MAKE} sun-i386-svr4" ; \
#		echo "	${MAKE} dec-ultrix-mips" ; \
#		echo "	${MAKE} sgi-mips-r3000" ; \
#		echo "	${MAKE} gizmo" ; \

	## Build each of the modules in the system. It is important that the modules be
	## built in this order. Each successive module is built using previous modules.

	set -e ; \
	for module in doc/man src/translator src/collection src/kernel src/scheduler src/library src/profiler src/debugger ; do \
		${MAKE} -C ${PACKAGE}/$${module} ${MAKECMDGOALS} ; \
	done

scratch :
	rm -rf ${INSTALLMANDIR}/* ${INSTALLBINDIR}/* ${INSTALLLIBDIR}/* ${INSTALLINCDIR}/*

test :
	tmpname="${TMPDIR}/uC_tmp$${$$}" ; \
	for kind in "-debug -nomulti" "-nodebug -nomulti" "-debug -multi" "-nodebug -multi" ; do \
		echo '_Task T { void main() {}; }; int main() { T t[10]; }' > $${tmpname}.cc ; \
		${INSTALLBINDIR}/u++ -quiet $${kind} $${tmpname}.cc -o $${tmpname} ; \
		if [ $${?} -ne 0 ] ; then \
			echo "Compilation failed for options $${kind}" ; \
			exit 1 ; \
		fi ; \
		$${tmpname} ; \
		if [ $${?} -ne 0 ] ; then \
			echo "Test failed for options $${kind}" ; \
			exit 1 ; \
		fi ; \
	done ; \
	echo "Test passed for" ${INSTALLBINDIR}/u++ ; \
	rm -f $${tmpname}.cc $${tmpname}

# These recipe recursively invoke this file again with all configuration variables set.

linux-x86 linux-i386 linux-x86_32 : ; @${MAKE} configure VENDOR=pc OS=linux CPU=i386 TVENDOR=pc TOS=linux TCPU=i386 MULTI=TRUE

linux-x86_64 : ; @${MAKE} configure VENDOR=pc OS=linux CPU=x86_64 TVENDOR=pc TOS=linux TCPU=x86_64 MULTI=TRUE

linux-arm_64 : ; @${MAKE} configure VENDOR=pc OS=linux CPU=arm_64 TVENDOR=pc TOS=linux TCPU=arm_64 MULTI=TRUE

#linux-ia64 : ; @${MAKE} configure VENDOR=pc OS=linux CPU=ia64 TVENDOR=pc TOS=linux TCPU=ia64 MULTI=TRUE
#freebsd-x86 freebsd-i386 : ; @${MAKE} configure VENDOR=pc OS=freebsd CPU=i386 TVENDOR=pc TOS=freebsd TCPU=i386 MULTI=TRUE
#freebsd-x86_64 : ; @${MAKE} configure VENDOR=pc OS=freebsd CPU=x86_64 TVENDOR=pc TOS=freebsd TCPU=x86_64 MULTI=TRUE
#darwin-x86 darwin-i386 : ; @${MAKE} configure VENDOR=pc OS=freebsd CPU=i386 TVENDOR=pc TOS=freebsd TCPU=i386 MULTI=TRUE
#solaris-sparc : ; ${MAKE} configure VENDOR=sun OS=solaris CPU=sparc TVENDOR=sun TOS=solaris TCPU=sparc MULTI=TRUE
#sequent-i386-ptx : ; @${MAKE} configure VENDOR=sequent OS=svr4 CPU=i386 TVENDOR=sequent TOS=svr4 TCPU=i386 MULTI=TRUE
#sun-sparc-sunos : ; @${MAKE} configure VENDOR=sun OS=sunos CPU=sparc TVENDOR=sun TOS=sunos TCPU=sparc MULTI=TRUE
#sun-m68k-sunos : ; @${MAKE} configure VENDOR=sun OS=sunos CPU=m68k TVENDOR=sun TOS=sunos TCPU=m68k MULTI=TRUE
#sun-i386-svr4 : ; @${MAKE} configure VENDOR=sun OS=svr4 CPU=i386 TVENDOR=sun TOS=svr4 TCPU=i386 MULTI=TRUE
#dec-alpha : ; @${MAKE} configure VENDOR=dec OS=svr4 CPU=alpha TVENDOR=dec TOS=svr4 TCPU=alpha MULTI=TRUE
#dec-ultrix-mips : ; @${MAKE} configure VENDOR=dec OS=ultrix CPU=mips TVENDOR=dec TOS=ultrix TCPU=mips MULTI=FALSE
#irix-mips : ; @${MAKE} configure VENDOR=sgi OS=irix CPU=mips TVENDOR=sgi TOS=irix TCPU=mips MULTI=TRUE
#sgi-mips-r3000 : ; @${MAKE} configure VENDOR=sgi OS=svr4 CPU=mips TVENDOR=sgi TOS=svr4 TCPU=mips MULTI=FALSE
#ibm-rs6000 : ; @${MAKE} configure VENDOR=ibm OS=svr4 CPU=rs6000 TVENDOR=ibm TOS=svr4 TCPU=rs6000 MULTI=FALSE
#hp-pa : ; @${MAKE} configure VENDOR=hp OS=hpux CPU=hppa TVENDOR=hp TOS=hpux TCPU=hppa MULTI=FALSE
#gizmo : ; @${MAKE} configure VENDOR=sun OS=svr4 CPU=sparc TVENDOR=gizmo TOS=bsd TCPU=m68k MULTI=FALSE

.SILENT : configure

configure :
	echo 'Configuring u++-${VERSION} for ${VENDOR} host running ${OS} on a ${CPU} for ${TVENDOR} target running ${TOS} on a ${TCPU}.'

	echo '##' > ${CONFIG}
	echo '## DO NOT EDIT THIS FILE DIRECTLY. DELETE THIS FILE, EDIT THE' >> ${CONFIG}
	echo '## MAKEFILE, AND THEN RUN ${MAKE} TO CREATE A NEW CONFIG FILE.' >> ${CONFIG}
	echo '##' >> ${CONFIG}
	echo 'VERSION = ${VERSION}' >> ${CONFIG}
	echo 'VENDOR = ${VENDOR}' >> ${CONFIG}
	echo 'OS = ${OS}' >> ${CONFIG}
	echo 'CPU = ${CPU}' >> ${CONFIG}
	echo 'TVENDOR = ${TVENDOR}' >> ${CONFIG}
	echo 'TOS = ${TOS}' >> ${CONFIG}
	echo 'TCPU = ${TCPU}' >> ${CONFIG}
	echo 'INSTALLPREFIX = ${INSTALLPREFIX}' >> ${CONFIG}
	echo 'INSTALLINCDIR = ${value INSTALLINCDIR}' >> ${CONFIG}
	echo 'INSTALLLIBDIR = ${value INSTALLLIBDIR}' >> ${CONFIG}
	echo 'INSTALLBINDIR = ${value INSTALLBINDIR}' >> ${CONFIG}
	echo 'INSTALLMANDIR = ${value INSTALLMANDIR}' >> ${CONFIG}
	echo 'INSTALLDOCDIR = ${value INSTALLDOCDIR}' >> ${CONFIG}
	echo 'TMPDIR = ${TMPDIR}' >> ${CONFIG}
	echo 'CCBUILD = ${CCBUILD}' >> ${CONFIG}
	echo 'CCAPP = ${CCAPP}' >> ${CONFIG}
	echo 'UPP = ${UPP}' >> ${CONFIG}
	echo 'MAXENTRYBITS := ${MAXENTRYBITS}' >> ${CONFIG}
	echo 'STATISTICS := ${STATISTICS}' >> ${CONFIG}
	echo 'CPP11 := ${CPP11}' >> ${CONFIG}
	echo 'MULTI = ${MULTI}' >> ${CONFIG}
	echo 'SHELL := /bin/sh' >> ${CONFIG}

	for file in `find doc src -type f -name Makefile -print` ; do \
		tmpname="${TMPDIR}/uC_tmp$${$$}.cc" ; \
		sed 's@^PACKAGE = .*@PACKAGE = '${PACKAGE}'@' $${file} > $${tmpname} ; \
		cmp -s $${file} $${tmpname} ; \
		if [ $${?} -ne 0 ] ; then \
			echo "Setting hard coded variables in file $${file}" ; \
			cp $${tmpname} $${file} ; \
		fi ; \
		rm -f $${tmpname} ; \
	done

	printf "Checking compiler..." ; \
	if which ${CCAPP} > /dev/null ; then \
		tmpname="${TMPDIR}/uC_tmp$${$$}.cc" ; \
		echo 'int main() {\n#ifndef __GNUC__\nreturn 1;\n#else\nreturn __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 8;\n#endif\n}' > $${tmpname} ; \
		if ${CCAPP} $${tmpname} > /dev/null 2>&1 && ./a.out > /dev/null 2>&1 ; then \
			printf "Version of gcc too old for build, requires at least gcc 4.8.0 to access C++11 features\n" ; \
			rm -f a.out $${tmpname} ; \
			rm -f CONFIG ; \
			exit 1 ; \
		fi ; \
		rm -f a.out $${tmpname} ; \
		printf "found compiler ${CCAPP} and using -std=${CPP11}\n" ; \
	else \
		printf "compiler ${CCAPP} not found; possible cause compiler not installed\n" ; \
		rm -f CONFIG ; \
		exit 1 ; \
	fi

	printf "Checking for 32/64 bit compilation..." ; \
	if [ "${WORDSIZE}" = "" ] ; then \
		tmpname="${TMPDIR}/uC_tmp$${$$}.cc" ; \
		echo 'int main() {}' > $${tmpname} ; \
		${CCAPP} $${tmpname} > /dev/null 2>&1 ; \
		if file ./a.out | grep "32-bit" > /dev/null 2>&1 ; then \
			printf "32 bit\n" ; \
			echo "WORDSIZE := 32" >> ${CONFIG} ; \
		elif file ./a.out | grep "64-bit" > /dev/null 2>&1 ; then \
			printf "64 bit\n" ; \
			echo "WORDSIZE := 64" >> ${CONFIG} ; \
		else \
			printf "Check for 32/64 bit compilation failed\n" ; \
			rm -f a.out $${tmpname} ; \
			rm -f CONFIG ; \
			exit 1 ; \
		fi ; \
		rm -f a.out $${tmpname} ; \
	else \
		if [ "${WORDSIZE}" = "32" -o "${WORDSIZE}" = "64" ] ; then \
			printf "${WORDSIZE} bit\n" ; \
			echo "WORDSIZE := ${WORDSIZE}" >> ${CONFIG} ; \
		else \
			printf "Invalid WORDSIZE ${WORDSIZE}, must be 32 or 64\n" ; \
			rm -f CONFIG ; \
			exit 1 ; \
		fi ; \
	fi

	printf "Checking for TLS support in libc..." ; \
	tmpname="${TMPDIR}/uC_tmp$${$$}.cc" ; \
	echo '__thread int x = 1; int main() { if ( x == 1 ) return 0; else return 1; }' > $${tmpname} ; \
	if ${CCAPP} $${tmpname} > /dev/null 2>&1 && ./a.out > /dev/null 2>&1 ; then \
		echo yes ; \
	else \
		echo no ; \
		printf "Check for TLS support failed\n" ; \
		rm -f a.out $${tmpname} ; \
		rm -f CONFIG ; \
		exit 1 ; \
	fi ; \
	rm -f a.out $${tmpname}

	if [ ${TOS} = linux ] ; then \
		echo "AFFINITY := TRUE" >> ${CONFIG} ; \
	fi ;

	# TEMPORARY: old glibc declares pthread_testcancel as throw()
	if echo '#include <pthread.h>' | ${CCAPP} -E -x c++ - | grep testcancel | grep 'throw[ \t]*()' > /dev/null ; then \
		echo "BROKEN_CANCEL := TRUE" >> ${CONFIG} ; \
	else \
		echo "BROKEN_CANCEL := FALSE" >> ${CONFIG} ; \
	fi

	echo "Finished configuration, just type '${MAKE} install'."

## Local Variables: ##
## compile-command: "make install" ##
## End: ##
