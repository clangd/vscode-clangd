## Define the appropriate configuration variables.

PACKAGE = /u0/usystem/software/u++-7.0.0

## Include the architecture dependent definitions.

include ${PACKAGE}/CONFIG

## Define the path names of important directories.

SRCDIR = ${PACKAGE}/src/kernel

## Define some of the tools.

include ${PACKAGE}/src/MakeTools

## Define the C++ source files.

LIBSRC = ${addprefix ${SRCDIR}/, ${addsuffix .cc, \
uDefaultStackSize \
uMainStackSize \
uDefaultSpin \
uDefaultPreemption \
uDefaultProcessors \
uStatistics \
uDebug \
uC++ \
uMachContext \
uBootTask \
uSystemTask \
uNBIO \
uAbortExit \
uContext \
uFloat \
uCalendar \
uAlarm \
uBaseCoroutine \
uBaseTask \
uHeapLmmm \
uSignal \
uProcessor \
uCluster \
uEHM \
uSemaphore \
} }

LIBSRC-D = ${LIBSRC}
ifeq (${MULTI},TRUE)
	LIBSRC-M = ${LIBSRC}
	LIBSRC-M-D = ${LIBSRC}
endif

## Define the asm source files.

LIBASM = ${addprefix ${SRCDIR}/, ${addsuffix .S, \
uSwitch-${TCPU} \
} }

LIBASM-D = ${LIBASM}
ifeq (${MULTI},TRUE)
	LIBASM-M = ${LIBASM}
	LIBASM-M-D = ${LIBASM-M}
endif

## Define the special object files.

MODSRC = ${addprefix ${SRCDIR}/, ${addsuffix .cc, \
uDefaultProcessors-OpenMP \
} }

## Define the header files

HEADERS = assert.h uAlign.h uRandom.h uDefault.h uCalendar.h uAlarm.h uEHM.h uHeapLmmm.h uC++.h uSystemTask.h uDebug.h uAtomic.h uBaseSelector.h uAdaptiveLock.h unwind-cxx.h unwind.h

## Define which libraries should be built.

LIB = ${LIBDIR}/uKernel.a
LIB-D = ${LIBDIR}/uKernel-d.a
ifeq (${MULTI},TRUE)
	LIB-M = ${LIBDIR}/uKernel-m.a
	LIB-M-D = ${LIBDIR}/uKernel-m-d.a
endif

## Define the things that can be defined based upon previous definitions.

include ${PACKAGE}/src/MakeDefinitions

## Define the specific recipes.

all : ${LIBRARIES} ${MODULES}

INSTALLFILES = ${addprefix ${INSTALLLIBDIR}/, ${notdir ${LIBRARIES}}} ${addprefix ${INSTALLLIBDIR}/, ${notdir ${MODULES}}} ${addprefix ${INSTALLINCDIR}/, ${HEADERS}}

install : all ${INSTALLFILES}

## Define default dependencies and recipes for object files.

ifneq ($(findstring g++,${CCAPP}),)
HEAPOPT=-fno-optimize-sibling-calls
endif

# special rules to ensure alloc/free routines appear on the stack for profiling
${OBJDIR}/uHeapLmmm.o : ${SRCDIR}/uHeapLmmm.cc # no multi, no debug
	${CC} -nomulti -nodebug -DNDEBUG ${CCFLAGS} ${HEAPOPT} -MD $< -c -o $@

${OBJDIR}/uHeapLmmm-d.o : ${SRCDIR}/uHeapLmmm.cc # no multi, debug
	${CC} -nomulti -debug ${CCFLAGS} ${HEAPOPT} -MD $< -c -o $@

${OBJDIR}/uHeapLmmm-m.o : ${SRCDIR}/uHeapLmmm.cc # multi, no debug
	${CC} -multi -nodebug -DNDEBUG ${CCFLAGS} ${HEAPOPT} -MD $< -c -o $@

${OBJDIR}/uHeapLmmm-m-d.o : ${SRCDIR}/uHeapLmmm.cc # multi, debug
	${CC} -multi -debug ${CCFLAGS} ${HEAPOPT} -MD $< -c -o $@

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
