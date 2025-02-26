## Define the appropriate configuration variables.

PACKAGE = /u0/usystem/software/u++-7.0.0

## Include the architecture dependent definitions.

include ${PACKAGE}/CONFIG

## Define the path names of important directories.

SRCDIR = ${PACKAGE}/src/library

## Define some of the tools.

include ${PACKAGE}/src/MakeTools

## Define the C++ source files.

LIBSRC = ${addprefix ${SRCDIR}/, ${addsuffix .cc, \
uFile \
uPoll \
uSocket \
uDefaultExecutorProcessors \
uDefaultExecutorWorkers \
uDefaultExecutorRQueues \
uDefaultExecutorSepClus \
uDefaultExecutorAffinity \
uFuture \
uCobegin \
uActor \
uPRNG \
pthread \
Unix \
} }

LIBSRC-D = ${LIBSRC}
ifeq (${MULTI},TRUE)
	LIBSRC-M = ${LIBSRC}
	LIBSRC-M-D = ${LIBSRC}
endif

## Define the header files

HEADERS = ostream fstream mutex shared_mutex type_traits bits/std_mutex.h bits/unique_lock.h ${shell ls *.h}

## Define which libraries should be built.

LIB = ${LIBDIR}/uLibrary.a
LIB-D = ${LIBDIR}/uLibrary-d.a
ifeq (${MULTI},TRUE)
	LIB-M = ${LIBDIR}/uLibrary-m.a
	LIB-M-D = ${LIBDIR}/uLibrary-m-d.a
endif

## Define the things that can be defined based upon previous definitions.

include ${PACKAGE}/src/MakeDefinitions

## Define the specific recipes.

all : ${LIBRARIES}

INSTALLFILES = ${addprefix ${INSTALLLIBDIR}/, ${notdir ${LIBRARIES}}} ${addprefix ${INSTALLINCDIR}/, ${HEADERS}}

install : all ${INSTALLFILES}

## Handle special include-file names.

${SRCDIR}/ostream: ${SRCDIR}/ostream.h
	cp ${SRCDIR}/ostream.h ${SRCDIR}/ostream

${SRCDIR}/fstream: ${SRCDIR}/fstream.h
	cp ${SRCDIR}/fstream.h ${SRCDIR}/fstream

## Define default dependencies and recipes for object files.

include ${PACKAGE}/src/MakeRecipes

## Include the dependencies for object files and libraries.

-include ${DEPENDS}

## Create directories (TEMPORARY: fixed in gmake 3.80}

_xxx1 := ${shell mkdir -p ${LIBDIR} ${OBJDIR}}

ifeq (${MAKECMDGOALS},install)
_xxx2 := ${shell mkdir -p ${INSTALLLIBDIR} ${INSTALLINCDIR} ${INSTALLINCDIR}/bits}
endif

## Local Variables: ##
## compile-command: "make install" ##
## End: ##
