## Define the appropriate configuration variables.

PACKAGE = /u0/usystem/software/u++-7.0.0

## Include the architecture dependent definitions.

include ${PACKAGE}/CONFIG

## Define the path names of important directories.

SRCDIR = ${PACKAGE}/src/examples

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
    ALLOCFLAGS += -DALLOCATOR=${ALLOCATOR}
    CXXFLAGS += -uAlloc${ALLOCATOR}
endif

.SILENT : all abortexit bench allocation features future actor pthread EHM realtime multiprocessor

all : bench allocation features collection cobegin actor future prng timeout pthread EHM realtime multiprocessor

errors : ownership abortexit

ownership :
	set -x ; \
	if [ ${MULTI} = TRUE ] ; then \
		multi=${MULTI} ; \
	fi ; \
	for filename in Ownership1 Ownership2 ; do \
		for ccflags in "" $${multi+"-multi"} ; do \
			${CXX} ${CXXFLAGS} $${ccflags} $${filename}.cc ; \
			./a.out ; \
		done ; \
	done ; \
	rm -f a.out core core.* ;

abortexit :
	set -x ; \
	if [ ${MULTI} = TRUE ] ; then \
		multi=${MULTI} ; \
	fi ; \
	pscmd="ps -u$${USER}" ; \
	for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
		${CXX} ${CXXFLAGS} $${ccflags} AbortExit.cc ; \
		echo "************************** 0 Killed by SIGTERM **************************" ; \
		./a.out 0 & \
		sleep 1 ; \
		kill $$! ; \
		sleep 1 ; \
		$$pscmd | grep a.out ; \
		echo "************************** 1 Exit **************************" ; \
		./a.out 1 ; \
		echo "return code is $$? should be 1" ; \
		sleep 1 ; \
		$$pscmd | grep a.out ; \
		echo "************************** 2 abort **************************" ; \
		./a.out 2 ; \
		echo "return code is $$?" ; \
		sleep 1 ; \
		$$pscmd | grep a.out ; \
		echo "************************** 3 Child killed by SIGKILL **************************" ; \
		./a.out 3 ; \
		echo "return code is $$?" ; \
		sleep 1 ; \
		$$pscmd | grep a.out ; \
		echo "************************** 4 abort **************************" ; \
		./a.out 4 ; \
		echo "return code is $$?" ; \
		sleep 1 ; \
		$$pscmd | grep a.out ; \
		echo "************************** 5 Assertion failure **************************" ; \
		./a.out 5 ; \
		echo "return code is $$?" ; \
		sleep 1 ; \
		$$pscmd | grep a.out ; \
		echo "************************** 6 Return from main **************************" ; \
		./a.out 6 ; \
		echo "return code is $$? should be 6" ; \
		sleep 1 ; \
		$$pscmd | grep a.out ; \
		echo "************************** 7 pthread_shutdown **************************" ; \
		./a.out 7 ; \
		echo "return code is $$? should be 7" ; \
		sleep 1 ; \
		$$pscmd | grep a.out ; \
	done ; \
	rm -f a.out core core.* ;

ifeq (${MAKECMDGOALS},Bench)
    CXXFLAGS += -multi -lrt
endif

bench :
	set -x ; \
	if [ ${MULTI} = TRUE ] ; then \
		multi=${MULTI} ; \
	fi ; \
	for filename in Bench ; do \
		for ccflags in "" "-nodebug -DNDEBUG" $${multi+"-multi"} $${multi+"-multi -nodebug -DNDEBUG"} ; do \
			${CXX} ${CXXFLAGS} $${ccflags} $${filename}.cc -lrt ; \
			./a.out ; \
		done ; \
	done ; \
	rm -f ./a.out ;

allocation :
	set -x ; \
	if [ ${MULTI} = TRUE ] ; then \
		multi=${MULTI} ; \
	fi ; \
	for filename in Allocation ; do \
		for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
			${CXX} ${ALLOCFLAGS} ${CXXFLAGS} $${ccflags} $${filename}.cc ; \
			/usr/bin/time -f "%Uu %Ss %Er %Mkb" ./a.out ; \
		done ; \
	done ; \
	rm -f ./a.out ;

collection :
	set -x ; \
	if [ ${MULTI} = TRUE ] ; then \
		multi=${MULTI} ; \
	fi ; \
	for filename in Stack Queue Sequence FlexArray BitSet ; do \
		for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
			${CXX} ${CXXFLAGS} $${ccflags} $${filename}.cc ; \
			./a.out ; \
		done ; \
	done ; \
	rm -f ./a.out ;

features :
	set -x ; \
	if [ ${MULTI} = TRUE ] ; then \
		multi=${MULTI} ; \
	fi ; \
	for filename in Array FloatTest CorFullProdCons CorFullProdConsStack BinaryInsertionSort Merger LockfreeStack Locks LocksFinally RWLock Accept MonAcceptBB MonConditionBB SemaphoreBB TaskAcceptBB TaskConditionBB DeleteProcessor Sleep Atomic Migrate Migrate2 ; do \
		for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
			${CXX} ${CXXFLAGS} $${ccflags} $${filename}.cc ; \
			./a.out ; \
		done ; \
	done ; \
	rm -f ./a.out ;

cobegin :
	set -x ; \
	if [ ${MULTI} = TRUE ] ; then \
		multi=${MULTI} ; \
	fi ; \
	for filename in Cobegin ; do \
		for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
			${CXX} ${CXXFLAGS} $${ccflags} $${filename}.cc ; \
			./a.out ; \
		done ; \
	done ; \
	rm -f ./a.out $${tmpname} ;

actor :
	set -x ; \
	if [ ${MULTI} = TRUE ] ; then \
		multi=${MULTI} ; \
	fi ; \
	for filename in ActorHello ActorFib1 ActorFib2 ActorFib3 ActorPhoneNo ActorDeviceDriver ActorFork ActorFork2 ActorChameneos ActorInherit ActorPromise ActorTrace ActorPingPong ActorRestart ActorRing ActorSieve ActorMatrixSum ActorProdCons ; do \
		for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
			${CXX} ${CXXFLAGS} $${ccflags} $${filename}.cc ; \
			./a.out ; \
		done ; \
	done ; \
	rm -f ./a.out ;

future :
	set -x ; \
	if [ ${MULTI} = TRUE ] ; then \
		multi=${MULTI} ; \
	fi ; \
	for filename in Futures Futures2 Executor Matrix ; do \
		for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
			${CXX} ${CXXFLAGS} $${ccflags} $${filename}.cc ; \
			./a.out ; \
		done ; \
	done ; \
	rm -f ./a.out ;

prng :
	set -x ; \
	if [ ${MULTI} = TRUE ] ; then \
		multi=${MULTI} ; \
	fi ; \
	for filename in PRNG ; do \
		for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
			${CXX} ${CXXFLAGS} $${ccflags} $${filename}.cc ; \
			./a.out ; \
		done ; \
	done ; \
	rm -f ./a.out ;

timeout :
	set -x ; \
	if [ ${MULTI} = TRUE ] ; then \
		multi=${MULTI} ; \
	fi ; \
	for filename in TimeSlice TimeoutAccept TimeoutCondLock TimeoutSem TimeoutProcessor TimeoutPoke ; do \
		for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
			${CXX} ${CXXFLAGS} $${ccflags} $${filename}.cc ; \
			./a.out ; \
		done ; \
	done ; \
	rm -f ./a.out ;

pthread :
	set -x ; \
	if [ ${MULTI} = TRUE ] ; then \
		multi=${MULTI} ; \
	fi ; \
	for filename in Pthread TimeoutPthreadsCond ; do \
		for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
			${CXX} ${CXXFLAGS} $${ccflags} $${filename}.cc ; \
			./a.out ; \
		done ; \
	done ; \
	rm -f ./a.out ;

EHM :
	set -x ; \
	if [ ${MULTI} = TRUE ] ; then \
		multi=${MULTI} ; \
	fi ; \
	for filename in EHM1 EHM2 EHM3 EHM4 EHM5 EHM6 EHM7 EHM8 EHM9 EHM10 EHM11 CRAII UncaughtException ; do \
		for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
			${CXX} ${CXXFLAGS} $${ccflags} $${filename}.cc ; \
			./a.out ; \
		done ; \
	done ; \
	rm -f ./a.out ;

realtime :
	set -x ; \
	if [ ${MULTI} = TRUE ] ; then \
		multi=${MULTI} ; \
	fi ; \
	for filename in PeriodicTaskTest PeriodicTaskTest1 PeriodicTaskTestStatic RealTimePhilosophers RealTimePhilosophers1 RealTimePhilosophersStatic Disinherit Disinherit1 DisinheritStatic Disinherit1Static ; do \
		for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
			${CXX} ${CXXFLAGS} $${ccflags} $${filename}.cc ; \
			/usr/bin/time -f "%Uu %Ss %Er %Mkb" ./a.out ; \
		done ; \
	done ; \
	rm -f ./a.out ;

multiprocessor :
	set -x ; \
	if [ ${MULTI} = TRUE ] ; then \
		multi=${MULTI} ; \
	fi ; \
	${CXX} ${CXXFLAGS} uCalibrate.cc -lrt ; \
	./a.out > uCalibrate.h ; \
	for ccflags in "" "-nodebug" $${multi+"-multi"} $${multi+"-multi -nodebug"} ; do \
		${CXX} ${CXXFLAGS} $${ccflags} Sim.cc ; \
		/usr/bin/time -f "%Uu %Ss %Er %Mkb" ./a.out  1 100 500000 ; \
		/usr/bin/time -f "%Uu %Ss %Er %Mkb" ./a.out  2 100 500000 ; \
		/usr/bin/time -f "%Uu %Ss %Er %Mkb" ./a.out  4 100 500000 ; \
		/usr/bin/time -f "%Uu %Ss %Er %Mkb" ./a.out  8 100 500000 ; \
		/usr/bin/time -f "%Uu %Ss %Er %Mkb" ./a.out 16 100 500000 ; \
	done ; \
	rm -f ./a.out ;


## Local Variables: ##
## compile-command: "make install" ##
## End: ##
