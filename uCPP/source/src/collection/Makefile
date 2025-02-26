## Define the appropriate configuration variables.

PACKAGE = /u0/usystem/software/u++-7.0.0

## Include the architecture dependent definitions.

include ${PACKAGE}/CONFIG

## Define the path names of important directories.

SRCDIR = ${PACKAGE}/src/collection

## Define some of the tools.

include ${PACKAGE}/src/MakeTools

## Define the header files

HEADERS = ${shell ls *.h}

## Define the things that can be defined based upon previous definitions.

include ${PACKAGE}/src/MakeDefinitions

## Define the specific recipes.

all :

INSTALLFILES = ${addprefix ${INSTALLINCDIR}/, ${HEADERS}}

install : all ${INSTALLFILES}

CLEAN =

## Define default dependencies and recipes for object files.

include ${PACKAGE}/src/MakeRecipes

## Create directories (TEMPORARY: fixed in gmake 3.80)

ifeq (${MAKECMDGOALS},install)
_xxx2 := ${shell mkdir -p ${INSTALLINCDIR}}
endif

## Local Variables: ##
## compile-command: "make install" ##
## End: ##
