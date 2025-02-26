## Define the appropriate configuration variables.

PACKAGE = /u0/usystem/software/u++-7.0.0

## Include the architecture dependent definitions.

include ${PACKAGE}/CONFIG

## Define the path names of important directories.

MANDIR = ${PACKAGE}/doc/man

## Define some of the tools.

include ${PACKAGE}/src/MakeTools

## Define the text source files.

MAN = ${addsuffix .1, \
u++ \
}

# Directives #

all :

install : ${INSTALLMANDIR} ${INSTALLMANDIR}/man1 ${INSTALLMANDIR}/man1/${MAN};

depend :

clean :

uninstall :
	rm -rf ${INSTALLMANDIR}

${INSTALLMANDIR}/man1/${MAN} : ${MANDIR}/${MAN}
	${INSTALL} $< $@
	${CHMODDATA} $@

## Create directories (TEMPORARY: fixed in gmake 3.80}

ifeq (${MAKECMDGOALS},install)
_xxx2 := ${shell mkdir -p ${INSTALLMANDIR}/man1}
endif

## Local Variables: ##
## compile-command: "make" ##
## End: ##
