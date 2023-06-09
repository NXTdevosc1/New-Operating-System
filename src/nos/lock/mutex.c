#include <nos/nos.h>
#include <nos/lock/lock.h>
void ExInitMutex(MUTEX* Mutex) {
    *Mutex = (MUTEX)INITIAL_MUTEX;
}

/*
 * TODO :
 - Derefence thread object
 - Use the timeout value
 - Switch to the mutex owner thread if the mutex is used
*/
BOOLEAN KRNLAPI ExMutexWait(void* Thread, MUTEX* Mutex, UINT64 TimeoutInMs) {
    // Todo Dereference Thread Object
    if(Thread) {

    if(Mutex->Owner == Thread) return TRUE;
    }
    while(_interlockedbittestandset(&Mutex->AccessLock, 0)) _mm_pause();
    Mutex->Owner = Thread;
    return TRUE;
}
// Checks if mutex is available then enters it without waiting
BOOLEAN KRNLAPI ExCheckMutexEnter(void* Thread, MUTEX* Mutex) {
    if(Thread) {

    if(Mutex->Owner == Thread) return TRUE;
    }
    if(_interlockedbittestandset64(&Mutex->AccessLock, 0)) return FALSE;
    Mutex->Owner = Thread;
    return TRUE;
}
BOOLEAN KRNLAPI ExMutexRelease(void* Thread, MUTEX* Mutex) {
    if(Thread) {
    if(Mutex->Owner != Thread) return FALSE;

    }
    Mutex->Owner = INVALID_HANDLE;
    Mutex->AccessLock = 0;
    return TRUE;
}