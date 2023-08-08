/*
lock.h

* The Exrnel Mutex & Semaphore Implementation
*/
#pragma once
#include <nosdef.h>


typedef volatile struct _MUTEX {
    void* Owner; // THREAD_OBJECT
    UINT32 AccessLock;
} MUTEX;

#define INITIAL_MUTEX {(void*)(UINT64)-1, 0}

#ifndef __SEMDEFINED
typedef volatile UINT64 SEMAPHORE;
#endif

typedef volatile UINT32 SPINLOCK;

// Direct mutex function calls (with less security checks)
void ExInitMutex(MUTEX* Mutex);
BOOLEAN KRNLAPI ExMutexWait(void* Thread, MUTEX* Mutex, UINT64 TimeoutInMs);
BOOLEAN KRNLAPI ExCheckMutexEnter(void* Thread, MUTEX* Mutex);
BOOLEAN KRNLAPI ExMutexRelease(void* Thread, MUTEX* Mutex);

void KRNLAPI ExInitSemaphore(SEMAPHORE* Semaphore, UINT InitialCount);
BOOLEAN KRNLAPI ExSemaphoreWait(SEMAPHORE* Semaphore, UINT64 TimeoutInMs);
BOOLEAN KRNLAPI ExSemaphoreSignal(SEMAPHORE* Semaphore);

// Lock API

// Return value : previous CPU Flags
#define INITIAL_SPINLOCK ((SPINLOCK){0})

UINT64 KRNLAPI ExAcquireSpinLock(SPINLOCK* SpinLock);
void KRNLAPI ExReleaseSpinLock(SPINLOCK* SpinLock, UINT64 CpuFlags);