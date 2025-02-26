## Define the appropriate configuration variables.

PACKAGE = /u0/usystem/software/u++-7.0.0

## Include the architecture dependent definitions.

include ${PACKAGE}/CONFIG

## Define the path names of important directories.

SRCDIR = ${PACKAGE}/src/scheduler

## Define some of the tools.

include ${PACKAGE}/src/MakeTools

## Define the C++ source files.

LIBSRC = ${addprefix ${SRCDIR}/, ${addsuffix .cc, \
uDeadlineMonotonic \
uDeadlineMonotonic1 \
uDeadlineMonotonicStatic \
uLifoScheduler \
uRealTime \
uHeapQ \
uPIHeap \
uStaticPriorityQ \
uStaticPIQ \
} }

LIBSRC-D = ${LIBSRC}
ifeq (${MULTI},TRUE)
	LIBSRC-M = ${LIBSRC}
	LIBSRC-M-D = ${LIBSRC}
endif

## Define the header files

HEADERS = ${shell ls *.h}

## Define which libraries should be built.

LIB = ${LIBDIR}/uScheduler.a
LIB-D = ${LIBDIR}/uScheduler-d.a
ifeq (${MULTI},TRUE)
	LIB-M = ${LIBDIR}/uScheduler-m.a
	LIB-M-D = ${LIBDIR}/uScheduler-m-d.a
endif

## Define the things that can be defined based upon previous definitions.

include ${PACKAGE}/src/MakeDefinitions

## Define the specific recipes.

all : ${LIBRARIES}

INSTALLFILES = ${addprefix ${INSTALLLIBDIR}/, ${notdir ${LIBRARIES}}} ${addprefix ${INSTALLINCDIR}/, ${HEADERS}}

install : all ${INSTALLFILES}

## Define default dependencies and recipes for object files.

include ${PACKAGE}/src/MakeRecipes

## Include the dependencies for object files and libraries.

-include ${DEPENDS}

## Create directories (TEMPORARY: fixed in gmake 3.80)

_xxx1 := ${shell mkdir -p ${LIBDIR} ${OBJDIR}}

ifeq (${MAKECMDGOALS},install)
_xxx2 := ${shell mkdir -p ${INSTALLLIBDIR} ${INSTALLINCDIR}}
endif

## Local Variables: ##
## compile-command: "make install" ##
## End: ##
