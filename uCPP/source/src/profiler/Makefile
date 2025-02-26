## Define the appropriate configuration variables.

PACKAGE = /u0/usystem/software/u++-7.0.0

## Include the architecture dependent definitions.

include ${PACKAGE}/CONFIG

## Define the path names of important directories.

SRCDIR = ${PACKAGE}/src/profiler

## Define some of the tools.

include ${PACKAGE}/src/MakeTools

## Define the special object files.

MODSRC = ${addprefix ${SRCDIR}/, ${addsuffix .cc, \
uProfilerFunctionPointers \
} }

## Define the header files

HEADERS = ${shell ls *.h}

## Define the things that can be defined based upon previous definitions.

include ${PACKAGE}/src/MakeDefinitions

## Define the specific recipes.

all : ${MODULES}

INSTALLFILES = ${addprefix ${INSTALLLIBDIR}/, ${notdir ${MODULES}}} ${addprefix ${INSTALLINCDIR}/, ${HEADERS}}

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
