## Define the appropriate configuration variables.

PACKAGE = /u0/usystem/software/u++-7.0.0

## Include the architecture dependent definitions.

include ${PACKAGE}/CONFIG

## Define the path names of important directories.

SRCDIR = ${PACKAGE}/src/debugger

## Define some of the tools.

include ${PACKAGE}/src/MakeTools

## Define the C++ source files.

LIBSRC = ${addprefix ${SRCDIR}/, ${addsuffix .cc, \
uBConditionEval \
uDebuggerProtocolUnit \
uLocalDebugger \
} }

LIBSRC-D = ${LIBSRC}
ifeq (${MULTI},TRUE)
#	LIBSRC-M = ${LIBSRC}
	LIBSRC-M-D = ${LIBSRC}
endif

## Define the header files

HEADERS = uLocalDebugger.h uDebuggerAPI.h

## Define which libraries should be built.

#LIB = ${LIBDIR}/uLocalDebugger.a
LIB-D = ${LIBDIR}/uLocalDebugger-d.a
ifeq (${MULTI},TRUE)
#	LIB-M = ${LIBDIR}/uLocalDebugger-m.a
	LIB-M-D = ${LIBDIR}/uLocalDebugger-m-d.a
endif

## Define the things that can be defined based upon previous definitions.

include ${PACKAGE}/src/MakeDefinitions

## Define specific flags

SIZE_OF_BREAKPOINT_FIELD := 64

CCFLAGS += -DSIZE_OF_BREAKPOINT_FIELD=${SIZE_OF_BREAKPOINT_FIELD} #-DDEBUG_ON

CLEAN += ${SRCDIR}/uLocalDebuggerHandler.h

## Define the specific recipes.

all : ${LIBRARIES}

INSTALLFILES = ${addprefix ${INSTALLLIBDIR}/, ${notdir ${LIBRARIES}}} ${addprefix ${INSTALLINCDIR}/, ${HEADERS}}

install : all ${INSTALLFILES}

## Define special rules for making the break point header file.

uLocalDebuggerHandler.h : ${SRCDIR}/uMakeBPstubs-${TCPU}
	${SRCDIR}/uMakeBPstubs-${TCPU} ${SIZE_OF_BREAKPOINT_FIELD} > $@

## Define default dependencies and recipes for object files.

include ${PACKAGE}/src/MakeRecipes

## Include the dependencies for object files and libraries.

${OBJDIR}/uLocalDebugger.o ${OBJDIR}/uLocalDebugger-d.o ${OBJDIR}/uLocalDebugger-m.o ${OBJDIR}/uLocalDebugger-m-d.o : uLocalDebuggerHandler.h
-include ${DEPENDS}

## Create directories (TEMPORARY: fixed in gmake 3.80)

_xxx1 := ${shell mkdir -p ${LIBDIR} ${OBJDIR}}

ifeq (${MAKECMDGOALS},install)
_xxx2 := ${shell mkdir -p ${INSTALLLIBDIR} ${INSTALLINCDIR}}
endif

## Local Variables: ##
## compile-command: "make install" ##
## End: ##
