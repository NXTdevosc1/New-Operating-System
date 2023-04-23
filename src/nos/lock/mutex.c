#include <nos/nos.h>
#include <nos/lock/lock.h>
void KeInitMutex(MUTEX* Mutex) {
    *Mutex = INITIAL_MUTEX;
}

/*
 * TODO :
 - Derefence thread object
 - Use the timeout value
 - Switch to the mutex owner thread if the mutex is used
*/
BOOLEAN KRNLAPI KeMutexWait(void* Thread, MUTEX* Mutex, UINT64 TimeoutInMs) {
    // Todo Dereference Thread Object
    if(Mutex->Owner == Thread) return TRUE;
    while(_interlockedbittestandset(&Mutex->AccessLock, 0)) _mm_pause();
    Mutex->Owner = Thread;
    return TRUE;
}
// Checks if mutex is available then enters it without waiting
BOOLEAN KRNLAPI KeCheckMutexEnter(void* Thread, MUTEX* Mutex) {
    if(Mutex->Owner == Thread) return TRUE;
    if(_interlockedbittestandset64(&Mutex->AccessLock, 0)) return FALSE;
    Mutex->Owner = Thread;
    return TRUE;
}
BOOLEAN KRNLAPI KeMutexRelease(void* Thread, MUTEX* Mutex) {
    if(Mutex->Owner != Thread) return FALSE;
    Mutex->Owner = INVALID_HANDLE;
    Mutex->AccessLock = 0;
    return TRUE;
}