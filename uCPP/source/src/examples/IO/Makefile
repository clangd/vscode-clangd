## Define the appropriate configuration variables.

PACKAGE = /u0/usystem/software/u++-7.0.0

## Include the architecture dependent definitions.

include ${PACKAGE}/CONFIG

SHELLFLAGS := set -x ;

## Define the path names of important directories.

SRCDIR = ${PACKAGE}/src/examples/IO
LFILE = ${PACKAGE}/src/kernel/uC++.cc

CXX = ${INSTALLBINDIR}/${UPP}

HOSTNAME = ${shell hostname}
ifeq (${HOSTNAME},plg2)
    CXXFLAGS += -O2 -Wall -Wextra #
else
    CXXFLAGS += -O2 #
endif

ifeq (${TCPU},i386)
    CXXFLAGS += -m32
endif

ifdef ALLOCATOR
    CXXFLAGS += -uAlloc${ALLOCATOR}
endif

.SILENT : all file pipe socket unix inet sendfile plain

all : file pipe socket

socket : unix inet sendfile

file :
	${SHELLFLAGS} \
	if [ ${MULTI} = TRUE ] ; then \
	    multi=${MULTI} ; \
	fi ; \
	times=3 ; \
	for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
	    ${CXX} ${CXXFLAGS} $${ccflags} CmdLine.cc ; \
	    i=0 ; \
	    while [ $${i} -lt $${times} ] ; do \
		./a.out 34 0 File.cc ; \
		i=`expr $${i} + 1` ; \
		echo "\n************************** $${i} **************************" ; \
	    done ; \
	done ; \
	for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
	    ${CXX} ${CXXFLAGS} $${ccflags} File.cc ; \
	    i=0 ; \
	    while [ $${i} -lt $${times} ] ; do \
		./a.out File.cc ; \
		i=`expr $${i} + 1` ; \
		echo "\n************************** $${i} **************************" ; \
	    done ; \
	done ; \
	for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
	    ${CXX} ${CXXFLAGS} $${ccflags} NBStream.cc ; \
	    i=0 ; \
	    while [ $${i} -lt $${times} ] ; do \
		( echo 1; sleep 1; echo 2; sleep 1; echo 3; sleep 1; echo 4; sleep 1; ) | ./a.out ; \
		i=`expr $${i} + 1` ; \
		echo "\n************************** $${i} **************************" ; \
	    done ; \
	done ; \
	for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
	    ${CXX} ${CXXFLAGS} $${ccflags} NBFile.cc ; \
	    i=0 ; \
	    while [ $${i} -lt $${times} ] ; do \
		( echo 1; sleep 1; echo 2; sleep 1; echo 3; sleep 1; echo 4; sleep 1; ) | ./a.out ; \
		cat xxx ; \
		i=`expr $${i} + 1` ; \
		echo "\n************************** $${i} **************************" ; \
	    done ; \
	done ; \
	for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
	    ${CXX} ${CXXFLAGS} $${ccflags} StringIO.cc ; \
	    i=0 ; \
	    while [ $${i} -lt $${times} ] ; do \
		./a.out ${LFILE} > xxx ; cmp ${LFILE} xxx ; \
		i=`expr $${i} + 1` ; \
		echo "************************** $${i} **************************" ; \
	    done ; \
	done ; \
	for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
	    ${CXX} ${CXXFLAGS} $${ccflags} Filebuf.cc ; \
	    ./a.out Filebuf.cc ; \
	done ; \
	rm -f xxx a.out ;

pipe :
	${SHELLFLAGS} \
	ulimit -n 1024 ; \
	if [ ${MULTI} = TRUE ] ; then \
	    multi=${MULTI} ; \
	fi ; \
	for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
	    ${CXX} ${CXXFLAGS} $${ccflags} PipesSelect.cc ; \
	    ./a.out ; \
	done ; \
	rm -f a.out ;

#	\
#	for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
#		${CXX} ${CXXFLAGS} $${ccflags} Pipes.cc ; \
#		./a.out ; \
#	done ; \

unix :
	${SHELLFLAGS} \
	if [ ${MULTI} = TRUE ] ; then \
	    multi=${MULTI} ; \
	fi ; \
	rm -f sock ; \
	times=3 ; \
	for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
	    ${CXX} ${CXXFLAGS} $${ccflags} ClientUNIXSTREAM.cc -o Client ; \
	    ${CXX} ${CXXFLAGS} $${ccflags} ServerUNIXSTREAM.cc -o Server ; \
	    ( ./Server sock & \
		( \
		    sleep 5 ; \
		    i=0 ; \
		    while [ $${i} -lt $${times} ] ; do \
			( ./Client sock < ${LFILE} > xxx1 & ./Client sock < ${LFILE} > xxx2 & ./Client sock < ${LFILE} > xxx3 & \
			    ./Client sock < ${LFILE} > xxx4 & ./Client sock < ${LFILE} > xxx5 ; wait ) ; \
			for file in xxx* ; do cmp ${LFILE} $${file} ; done ; \
			i=`expr $${i} + 1` ; \
			echo "************************** $${i} **************************" ; \
		    done ; \
		) ; wait \
	    ) ; \
	    rm -f Server Client xxx* ; \
	done ; \
	rm -f sock ; \
	for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
	    ${CXX} ${CXXFLAGS} $${ccflags} ClientUNIXDGRAM.cc -o Client ; \
	    ${CXX} ${CXXFLAGS} $${ccflags} ServerUNIXDGRAM.cc -o Server ; \
	    ( TMPDIR=/tmp ; export TMPDIR ; ./Server sock & \
		( \
		    sleep 5 ; \
		    i=0 ; \
		    while [ $${i} -lt $${times} ] ; do \
			( ./Client sock < ${LFILE} > xxx1 & ./Client sock < ${LFILE} > xxx2 & ./Client sock < ${LFILE} > xxx3 & \
			    ./Client sock < ${LFILE} > xxx4 & ./Client sock < ${LFILE} > xxx5 ; wait ) ; \
			for file in xxx* ; do cmp ${LFILE} $${file} ; done ; \
			i=`expr $${i} + 1` ; \
			echo "************************** $${i} **************************" ; \
		    done ; \
		) ; wait \
	    ) ; \
	    rm -f Server Client xxx* ; \
	done ;

inet :
	${SHELLFLAGS} \
	if [ ${MULTI} = TRUE ] ; then \
	    multi=${MULTI} ; \
	fi ; \
	rm -f portno ; \
	times=3 ; \
	for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
	    ${CXX} ${CXXFLAGS} $${ccflags} ClientINETSTREAM.cc -o Client ; \
	    ${CXX} ${CXXFLAGS} $${ccflags} ServerINETSTREAM.cc -o Server ; \
	    ( ./Server > portno & \
		( \
		    sleep 5 ; portno=`cat portno` ; \
		    i=0 ; \
		    while [ $${i} -lt $${times} ] ; do \
			( ./Client $${portno} < ${LFILE} > xxx1 & ./Client $${portno} < ${LFILE} > xxx2 & ./Client $${portno} < ${LFILE} > xxx3 & \
			    ./Client $${portno} < ${LFILE} > xxx4 & ./Client $${portno} < ${LFILE} > xxx5 ; wait ) ; \
			for file in xxx* ; do cmp ${LFILE} $${file} ; done ; \
			i=`expr $${i} + 1` ; \
			echo "************************** $${i} **************************" ; \
		    done ; \
		) ; wait \
	    ) ; \
	    rm -f portno Server Client xxx* ; \
	done ; \
	rm -f portno ; \
	for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
	    ${CXX} ${CXXFLAGS} $${ccflags} ClientINETDGRAM.cc -o Client ; \
	    ${CXX} ${CXXFLAGS} $${ccflags} ServerINETDGRAM.cc -o Server ; \
	    ( TMPDIR=/tmp ; export TMPDIR ; ./Server > portno & \
		( \
		    sleep 5 ; portno=`cat portno` ; \
		    i=0 ; \
		    while [ $${i} -lt $${times} ] ; do \
			( ./Client $${portno} < ${LFILE} > xxx1 & ./Client $${portno} < ${LFILE} > xxx2 & ./Client $${portno} < ${LFILE} > xxx3 & \
			    ./Client $${portno} < ${LFILE} > xxx4 & ./Client $${portno} < ${LFILE} > xxx5 ; wait ) ; \
			for file in xxx* ; do cmp ${LFILE} $${file} ; done ; \
			i=`expr $${i} + 1` ; \
			echo "************************** $${i} **************************" ; \
		    done ; \
		) ; wait \
	    ) ; \
	    rm -f portno Server Client xxx* ; \
	done ;

sendfile :
	${SHELLFLAGS} \
	if [ ${MULTI} = TRUE ] ; then \
	    multi=${MULTI} ; \
	fi ; \
	times=3 ; \
	rm -f sock ; \
	for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
	    ${CXX} ${CXXFLAGS} $${ccflags} ClientUNIXSTREAMSendfile.cc -o Client ; \
	    ${CXX} ${CXXFLAGS} $${ccflags} ServerUNIXSTREAMSendfile.cc -o Server ; \
	    ( ./Server sock & \
		( \
		    sleep 5 ; \
		    i=0 ; \
		    while [ $${i} -lt $${times} ] ; do \
			( echo ${LFILE} | ./Client sock > xxx1 & echo ${LFILE} | ./Client sock > xxx2 & echo ${LFILE} | ./Client sock > xxx3 & \
			    echo ${LFILE} | ./Client sock > xxx4 & echo ${LFILE} | ./Client sock > xxx5 ; wait ) ; \
			for file in xxx* ; do cmp ${LFILE} $${file} ; done ; \
			i=`expr $${i} + 1` ; \
			echo "************************** $${i} **************************" ; \
		    done ; \
		) ; wait \
	    ) ; \
	    rm -f Server Client xxx* ; \
	done ; \
	rm -f portno ; \
	for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
	    ${CXX} ${CXXFLAGS} $${ccflags} ClientINETSTREAMSendfile.cc -o Client ; \
	    ${CXX} ${CXXFLAGS} $${ccflags} ServerINETSTREAMSendfile.cc -o Server ; \
	    ( ./Server > portno & \
		( \
		    sleep 5 ; portno=`cat portno` ; \
		    i=0 ; \
		    while [ $${i} -lt $${times} ] ; do \
			( echo ${LFILE} | ./Client $${portno} > xxx1 & echo ${LFILE} | ./Client $${portno} > xxx2 & echo ${LFILE} | ./Client $${portno} > xxx3 & \
			    echo ${LFILE} | ./Client $${portno} > xxx4 & echo ${LFILE} | ./Client $${portno} > xxx5 ; wait ) ; \
			for file in xxx* ; do cmp ${LFILE} $${file} ; done ; \
			i=`expr $${i} + 1` ; \
			echo "************************** $${i} **************************" ; \
		    done ; \
		) ; wait \
	    ) ; \
	    rm -f portno Server Client xxx* ; \
	done

plain :
	${SHELLFLAGS} \
	if [ ${MULTI} = TRUE ] ; then \
	    multi=${MULTI} ; \
	fi ; \
	times=3 ; \
	rm -f sock xxx* ; \
	g++ ${CXXFLAGS} $${ccflags} clientUNIXSTREAM.cc -o Client ; \
	g++ ${CXXFLAGS} $${ccflags} serverUNIXSTREAM.cc -o Server ; \
	( ./Server $${times} sock & \
	    (   sleep 5 ; \
		i=0 ; \
		while [ $${i} -lt $${times} ] ; do \
		    ( ./Client sock < ${LFILE} > xxx1 & ./Client sock < ${LFILE} > xxx2 & ./Client sock < ${LFILE} > xxx3 & \
			./Client sock < ${LFILE} > xxx4 & ./Client sock < ${LFILE} > xxx5 ; wait ) ; \
		    for file in xxx* ; do cmp ${LFILE} $${file} ; done ; \
		    i=`expr $${i} + 1` ; \
		    echo "************************** $${i} **************************" ; \
		done ; \
	    ) ; wait \
	) ; \
	rm -f Server Client xxx* ; \
	rm -f sock ; \
	g++ ${CXXFLAGS} $${ccflags} clientUNIXSTREAM2.cc -o Client ; \
	g++ ${CXXFLAGS} $${ccflags} serverUNIXSTREAM2.cc -o Server ; \
	( ./Server $${times} sock & \
	    (   sleep 5 ; \
		i=0 ; \
		while [ $${i} -lt $${times} ] ; do \
		    ( ./Client sock < ${LFILE} > xxx1 & ./Client sock < ${LFILE} > xxx2 & ./Client sock < ${LFILE} > xxx3 & \
			./Client sock < ${LFILE} > xxx4 & ./Client sock < ${LFILE} > xxx5 ; wait ) ; \
		    for file in xxx* ; do cmp ${LFILE} $${file} ; done ; \
		    i=`expr $${i} + 1` ; \
		    echo "************************** $${i} **************************" ; \
		done ; \
	    ) ; wait \
	) ; \
	rm -f Server Client xxx* ; \
	rm -f sock ; \
	g++ ${CXXFLAGS} $${ccflags} clientUNIXSTREAMSendfile.cc -o Client ; \
	g++ ${CXXFLAGS} $${ccflags} serverUNIXSTREAMSendfile.cc -o Server ; \
	( ./Server $${times} sock & \
	    (   sleep 5 ; \
		i=0 ; \
		while [ $${i} -lt $${times} ] ; do \
		    ( echo ${LFILE} | ./Client sock > xxx1 & echo ${LFILE} | ./Client sock > xxx2 & echo ${LFILE} | ./Client sock > xxx3 & \
			echo ${LFILE} | ./Client sock > xxx4 & echo ${LFILE} | ./Client sock > xxx5 ; wait ) ; \
		    for file in xxx* ; do cmp ${LFILE} $${file} ; done ; \
		    i=`expr $${i} + 1` ; \
		    echo "************************** $${i} **************************" ; \
		done ; \
	    ) ; wait \
	) ; \
	rm -f Server Client xxx* ; \
	rm -f sock ; \
	g++ ${CXXFLAGS} $${ccflags} clientUNIXDGRAM.cc -o Client ; \
	g++ ${CXXFLAGS} $${ccflags} serverUNIXDGRAM.cc -o Server ; \
	( ./Server sock & \
	    (   sleep 5 ; \
		i=0 ; \
		while [ $${i} -lt $${times} ] ; do \
		    ( ./Client sock < ${LFILE} > xxx1 & ./Client sock < ${LFILE} > xxx2 & ./Client sock < ${LFILE} > xxx3 & \
			./Client sock < ${LFILE} > xxx4 & ./Client sock < ${LFILE} > xxx5 ; wait ) ; \
		    for file in xxx* ; do cmp ${LFILE} $${file} ; done ; \
		    i=`expr $${i} + 1` ; \
		    echo "************************** $${i} **************************" ; \
		done ; \
	    ) ; wait \
	) ; \
	rm -f Server Client xxx* ; \
	rm -f portno ; \
	g++ ${CXXFLAGS} $${ccflags} clientINETDGRAM.cc -o Client ; \
	g++ ${CXXFLAGS} $${ccflags} serverINETDGRAM.cc -o Server ; \
	( ./Server > portno & \
	    (   sleep 5 ; \
		portno=`cat portno` ; \
		i=0 ; \
		while [ $${i} -lt $${times} ] ; do \
		    ( ./Client $${portno} < ${LFILE} > xxx1 & ./Client $${portno} < ${LFILE} > xxx2 & ./Client $${portno} < ${LFILE} > xxx3 & \
			./Client $${portno} < ${LFILE} > xxx4 & ./Client $${portno} < ${LFILE} > xxx5 ; wait ) ; \
		    for file in xxx* ; do cmp ${LFILE} $${file} ; done ; \
		    i=`expr $${i} + 1` ; \
		    echo "************************** $${i} **************************" ; \
		done ; \
	    ) ; wait \
	) ; \
	rm -f portno Server Client xxx* ;

## Local Variables: ##
## compile-command: "make install" ##
## End: ##

#	( truss ./Client $${portno} < File.cc 2>&1 | cat > y1 & truss ./Client $${portno} < File.cc 2>&1 | cat > y2 & \
#		truss ./Client $${portno} < File.cc 2>&1 | cat > y3 & truss ./Client $${portno} < File.cc 2>&1 | cat > y4 & \
#		truss ./Client $${portno} < File.cc 2>&1 | cat > y5 ;  wait )
