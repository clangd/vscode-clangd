## Define the appropriate configuration variables.

PACKAGE = /u0/usystem/software/u++-7.0.0

## Include the architecture dependent definitions.

include ${PACKAGE}/CONFIG

## Define the path names of important directories.

SRCDIR = ${PACKAGE}/src/translator

## Define some of the tools.

include ${PACKAGE}/src/MakeTools

## Do not use the u++ compiler to build the u++ compiler.

CC := ${CCBUILD}

## Override from MakeTools
CCFLAGS := -std=${CPP11} -I. -DPACKAGE=\"${PACKAGE}\" -DVERSION=\"${VERSION}\" -D__${VENDOR}__ -D__${OS}__ -D__${CPU}__ -DTVENDOR=\"${TVENDOR}\" -DTOS=\"${TOS}\" -DTCPU=\"${TCPU}\" -DINSTALLINCDIR=\"${INSTALLINCDIR}\" -DINSTALLLIBDIR=\"${INSTALLLIBDIR}\" -DINSTALLBINDIR=\"${INSTALLBINDIR}\" -DTMPDIR=\"${TMPDIR}\" -DMULTI=\"${MULTI}\" -DCCAPP=\"${CCAPP}\" -D__U_MAXENTRYBITS__=${MAXENTRYBITS} -DCPP11=\"${CPP11}\" -DWORDSIZE=${WORDSIZE}

ifeq (${STATISTICS},TRUE)
	CCFLAGS += -DSTATISTICS
endif

ifeq (${AFFINITY},TRUE)
	CCFLAGS += -DAFFINITY
endif

ifeq (${BROKEN_CANCEL},TRUE)
	CCFLAGS += -DBROKEN_CANCEL
endif

#PURIFY = purify --cache-dir=${HOME}/tmp

GCCVERSION = ${shell ${CCAPP} --version}
CPPNAME = cc1plus

HOSTNAME = ${shell hostname}
ifeq (${HOSTNAME},plg2)			# development machine
	CCFLAGS += -g -O2 -Wall -Wextra #
else
	CCFLAGS += -g -O2 # -DNDEBUG
endif

## Define the C++ source files.

#SRC = ${SRCDIR}/{\
#}.cc

#OBJ = ${OBJDIR}/{${SRC:b}}.o

## Define the source and object files for the driver.

DSRC = ${addprefix ${SRCDIR}/, ${addsuffix .cc, \
u++ \
} }

DOBJ = ${addprefix ${OBJDIR}/, ${addsuffix .o, ${basename ${notdir ${DSRC} } } } }

## Define the source and object files for the replacement preprocessor.

PSRC = ${addprefix ${SRCDIR}/, ${addsuffix .cc, \
${CPPNAME} \
} }

POBJ = ${addprefix ${OBJDIR}/, ${addsuffix .o, ${basename ${notdir ${PSRC} } } } }

## Define the source and object files for the translator.

TSRC = ${addprefix ${SRCDIR}/, ${addsuffix .cc, \
attribute \
gen \
hash \
input \
key \
main \
output \
parse \
scan \
structor \
symbol \
table \
token \
} }

TOBJ = ${addprefix ${OBJDIR}/, ${addsuffix .o, ${basename ${notdir ${TSRC} } } } }

## Use the host information

TVENDOR := ${VENDOR}
TOS := ${OS}
TCPU := ${CPU}

## Define the things that can be defined based upon previous definitions.

include ${PACKAGE}/src/MakeDefinitions

## Define which dependencies should be built.

DEPENDS = ${OBJDIR}/depend.mk

## Define which executables should be built.

BINS = ${UPP}
LIBS = ${CPPNAME} u++-cpp

## Define the specific recipes.

all : ${addprefix ${BINDIR}/, ${BINS}} ${addprefix ${LIBDIR}/, ${LIBS}}

INSTALLFILES = ${addprefix ${INSTALLLIBDIR}/, ${LIBS}} ${addprefix ${INSTALLBINDIR}/, ${BINS}}

install : all ${INSTALLFILES}

test :
	set -x ; \
	for filename in Operators AcceptStmt SelectStmt BreakStmt Finally CatchResume Exception Termination Event MutexTaskParms Constructor \
			Lookup Inheritance InheritanceException Template Friend STL Types C++11 ; do \
		${INSTALLBINDIR}/${UPP} ${CCFLAGS} -c test$${filename}.cc ; \
	done ; \
	rm -f ./a.out test*.o ;

## Everything depends on the make file.

${OBJ} ${DOBJ} ${POBJ} ${TOBJ} : Makefile

## Define default dependencies and recipes for making object files.

clean :
	${RM} ${CLEAN}

uninstall :
	${RM} ${INSTALLFILES}

${OBJDIR}/%.o : ${SRCDIR}/%.cc
	${PURIFY} ${CC} ${CCFLAGS} -MD $< -c -o $@

## Dependencies and recipes for the driver.

${BINDIR}/${UPP} : ${OBJ} ${DOBJ}
	${CC} ${CCFLAGS} ${OBJ} ${DOBJ} -o $@

## Dependencies and recipes for the preprocessor.

${LIBDIR}/${CPPNAME} : ${OBJ} ${POBJ}
	${CC} ${CCFLAGS} ${OBJ} ${POBJ} -o $@

## Dependencies: and recipes for the translator.

${LIBDIR}/u++-cpp : ${OBJ} ${TOBJ}
	${PURIFY} ${CC} ${CCFLAGS} ${OBJ} ${TOBJ} -o $@

#ifdef INSTALLBINDIR
${INSTALLBINDIR}/% : ${BINDIR}/%
	${INSTALL} $< $@
	${CHMODEXEC} $@
#endif

#ifdef INSTALLLIBDIR
${INSTALLLIBDIR}/% : ${LIBDIR}/%
	${INSTALL} $< $@
	${CHMODEXEC} $@
#endif

## Constructed dependencies for object files.

DDEPEND = ${addprefix ${OBJDIR}/, ${addsuffix .d, ${basename ${notdir ${DSRC}}}}}
PDEPEND = ${addprefix ${OBJDIR}/, ${addsuffix .d, ${basename ${notdir ${PSRC}}}}}
TDEPEND = ${addprefix ${OBJDIR}/, ${addsuffix .d, ${basename ${notdir ${TSRC}}}}}
-include ${DEPENDS} ${DDEPEND} ${PDEPEND} ${TDEPEND}

## Create directories (TEMPORARY: fixed in gmake 3.80}

_xxx1 := ${shell mkdir -p ${LIBDIR} ${OBJDIR} ${BINDIR}}

ifeq (${MAKECMDGOALS},install)
_xxx2 := ${shell mkdir -p ${INSTALLLIBDIR} ${INSTALLBINDIR}}
endif

## Local Variables: ##
## compile-command: "make install" ##
## End: ##
