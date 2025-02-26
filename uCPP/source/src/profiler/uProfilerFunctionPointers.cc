//                              -*- Mode: C++ -*- 
// 
// uC++ Version 7.0.0, Copyright (C) Robert Denda 1997
// 
// uProfilerFunctionPointers.cc -- 
// 
// Author           : Robert Denda
// Created On       : Thu Jul 31 20:35:59 1997
// Last Modified By : Peter A. Buhr
// Last Modified On : Fri Sep 21 07:29:58 2007
// Update Count     : 85
//
// This  library is free  software; you  can redistribute  it and/or  modify it
// under the terms of the GNU Lesser General Public License as published by the
// Free Software  Foundation; either  version 2.1 of  the License, or  (at your
// option) any later version.
// 
// This library is distributed in the  hope that it will be useful, but WITHOUT
// ANY  WARRANTY;  without even  the  implied  warranty  of MERCHANTABILITY  or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
// for more details.
// 
// You should  have received a  copy of the  GNU Lesser General  Public License
// along  with this library.
// 


#define __U_KERNEL__
#include <uC++.h>
#include "uProfiler.h"


uProfiler *uProfiler::profilerInstance = nullptr;

void (* uProfiler::uProfiler_registerTask)(uProfiler *, const uBaseTask &, const UPP::uSerial &, const uBaseTask &) = (void (*)(uProfiler *, const uBaseTask &, const UPP::uSerial &, const uBaseTask &)) 0; 
void (* uProfiler::uProfiler_deregisterTask)(uProfiler *, const uBaseTask &) = (void (*)(uProfiler *, const uBaseTask &)) 0;
void (* uProfiler::uProfiler_registerTaskStartExecution)(uProfiler *, const uBaseTask &) = (void (*)(uProfiler *, const uBaseTask &)) 0; 
void (* uProfiler::uProfiler_registerTaskEndExecution)(uProfiler *, const uBaseTask &) = (void (*)(uProfiler *, const uBaseTask &)) 0;
void (* uProfiler::uProfiler_registerTaskMigrate)(uProfiler *, const uBaseTask &, const uCluster &, const uCluster &) = (void (*)(uProfiler *, const uBaseTask &, const uCluster &, const uCluster &)) 0;

void (* uProfiler::uProfiler_registerCluster)(uProfiler *, const uCluster &) = (void (*)(uProfiler *, const uCluster &)) 0;
void (* uProfiler::uProfiler_deregisterCluster)(uProfiler *, const uCluster &) = (void(*)(uProfiler *, const uCluster &)) 0;

void (* uProfiler::uProfiler_registerProcessor)(uProfiler *, const uProcessor &) = (void (*)(uProfiler *, const uProcessor &)) 0;
void (* uProfiler::uProfiler_deregisterProcessor)(uProfiler *, const uProcessor &) = (void (*)(uProfiler *, const uProcessor & )) 0;
void (* uProfiler::uProfiler_registerProcessorMigrate)(uProfiler *, const uProcessor &, const uCluster &, const uCluster &) = (void (*)(uProfiler *, const uProcessor &, const uCluster &, const uCluster &)) 0;

void (* uProfiler::uProfiler_registerFunctionEntry)(uProfiler *, uBaseTask *, unsigned int, unsigned int, unsigned int) = (void (*)(uProfiler *, uBaseTask *, unsigned int, unsigned int, unsigned int)) 0;
void (* uProfiler::uProfiler_registerFunctionExit)(uProfiler *, uBaseTask *) = (void (*)(uProfiler *, uBaseTask *)) 0;

void (* uProfiler::uProfiler_registerWait)(uProfiler *, const uCondition &, const uBaseTask &, const UPP::uSerial &) = (void(*)(uProfiler *, const uCondition &, const uBaseTask &, const UPP::uSerial &)) 0;
void (* uProfiler::uProfiler_registerReady)(uProfiler *, const uCondition &, const uBaseTask &, const UPP::uSerial &) = (void(*)(uProfiler *, const uCondition &, const uBaseTask &, const UPP::uSerial &)) 0;
void (* uProfiler::uProfiler_registerSignal)(uProfiler *, const uCondition &, const uBaseTask &, const UPP::uSerial &) = (void(*)(uProfiler *, const uCondition &, const uBaseTask &, const UPP::uSerial &)) 0;

void (* uProfiler::uProfiler_registerMonitor)(uProfiler *, const UPP::uSerial &, const char *, const uBaseTask &) = (void(*)(uProfiler *, const UPP::uSerial &, const char *, const uBaseTask &)) 0;
void (* uProfiler::uProfiler_deregisterMonitor)(uProfiler *, const UPP::uSerial &, const uBaseTask &) = (void(*)(uProfiler *, const UPP::uSerial &, const uBaseTask &)) 0;
void (* uProfiler::uProfiler_registerMutexFunctionEntryTry)(uProfiler *, const UPP::uSerial &, const uBaseTask &) = (void(*)(uProfiler *, const UPP::uSerial &, const uBaseTask &)) 0;
void (* uProfiler::uProfiler_registerMutexFunctionEntryDone)(uProfiler *, const UPP::uSerial &, const uBaseTask &) = (void(*)(uProfiler *, const UPP::uSerial &, const uBaseTask &)) 0;

void (* uProfiler::uProfiler_registerMutexFunctionExit)(uProfiler *, const UPP::uSerial &, const uBaseTask &) = (void(*)(uProfiler *, const UPP::uSerial &, const uBaseTask &)) 0;
void (* uProfiler::uProfiler_registerAcceptStart)(uProfiler *, const UPP::uSerial &, const uBaseTask &) = (void(*)(uProfiler *, const UPP::uSerial &, const uBaseTask &)) 0;
void (* uProfiler::uProfiler_registerAcceptEnd)(uProfiler *, const UPP::uSerial &, const uBaseTask &) = (void(*)(uProfiler *, const UPP::uSerial &, const uBaseTask &)) 0;

void (* uProfiler::uProfiler_registerCoroutine)(uProfiler *, const uBaseCoroutine &, const UPP::uSerial &) = (void(*)(uProfiler *, const uBaseCoroutine &, const UPP::uSerial &)) 0;
void (* uProfiler::uProfiler_deregisterCoroutine)(uProfiler *, const uBaseCoroutine &) = (void(*)(uProfiler *, const uBaseCoroutine &)) 0;
void (* uProfiler::uProfiler_registerCoroutineBlock)(uProfiler *, const uBaseTask &, const uBaseCoroutine &) = (void(*)(uProfiler *, const uBaseTask &, const uBaseCoroutine &)) 0;    
void (* uProfiler::uProfiler_registerCoroutineUnblock)(uProfiler *, const uBaseTask &) = (void(*)(uProfiler *, const uBaseTask &)) 0;

void (* uProfiler::uProfiler_registerTaskExecState)(uProfiler *, const uBaseTask &, uBaseTask::State) = (void (*)(uProfiler *, const uBaseTask &, uBaseTask::State)) 0;

void (* uProfiler::uProfiler_poll)(uProfiler *) = (void (*)(uProfiler *)) 0;

void (* uProfiler::uProfiler_registerSetName)(uProfiler *, const uBaseCoroutine &, const char *) = (void(*)(uProfiler *, const uBaseCoroutine &, const char *)) 0;

MMInfoEntry *(* uProfiler::uProfiler_registerMemoryAllocate)(uProfiler *, void *, size_t, size_t ) = (MMInfoEntry *(*)(uProfiler *, void *, size_t, size_t )) 0;
void (* uProfiler::uProfiler_registerMemoryDeallocate)(uProfiler *, void *, size_t, MMInfoEntry * ) = (void (*)(uProfiler *, void *, size_t, MMInfoEntry * )) 0;

void (* uProfiler::uProfiler_profileInactivate)(uProfiler *, const uBaseTask &) = (void (*)(uProfiler *, const uBaseTask &)) 0; 

// hooks for built in metrics ( they can't be activated by a user )

void (* uProfiler::uProfiler_builtinRegisterTaskBlock)(uProfiler *, const uBaseTask &) = (void (*)(uProfiler *, const uBaseTask &)) 0;
void (* uProfiler::uProfiler_builtinRegisterTaskUnblock)(uProfiler *, const uBaseTask &) = (void (*)(uProfiler *, const uBaseTask &))0;
void (* uProfiler::uProfiler_builtinRegisterFunctionEntry)(uProfiler *) = (void (*)(uProfiler *)) 0;
void (* uProfiler::uProfiler_builtinRegisterFunctionExit )(uProfiler *) = (void (*)(uProfiler *)) 0;
void (* uProfiler::uProfiler_builtinRegisterProcessor)(uProfiler *, const uProcessor &) = (void (*)(uProfiler *, const uProcessor &)) 0;
void (* uProfiler::uProfiler_builtinDeregisterProcessor)(uProfiler *, const uProcessor &) = (void (*)(uProfiler *, const uProcessor &)) 0;
void (* uProfiler::uProfiler_builtinRegisterTaskStartSpin)(uProfiler *, const uBaseTask &) = (void (*)(uProfiler *, const uBaseTask &)) 0;
void (* uProfiler::uProfiler_builtinRegisterTaskStopSpin)(uProfiler *, const uBaseTask &) = (void (*)(uProfiler *, const uBaseTask &)) 0;

// dynamic memory allocation in uC++ kernel

void (* uProfiler::uProfiler_preallocateMetricMemory)(uProfiler *, void **, const uBaseTask &) = (void (*)(uProfiler *, void **, const uBaseTask &)) 0;
void (* uProfiler::uProfiler_postallocateMetricMemory)(uProfiler *, const uBaseTask &) = (void (*)(uProfiler *, const uBaseTask &)) 0;
void (* uProfiler::uProfiler_setMetricMemoryPointers)(uProfiler *, void **, const uBaseTask &) = (void (*)(uProfiler *, void **, const uBaseTask &)) 0;
void (* uProfiler::uProfiler_resetMetricMemoryPointers)(uProfiler *, const uBaseTask &) = (void (*)(uProfiler *, const uBaseTask &)) 0;

// debugging

void (* uProfiler::uProfiler_printCallStack)(uProfileTaskSampler *) = (void(*)(uProfileTaskSampler *)) 0;


// Local Variables: //
// compile-command: "make install" //
// End: //
