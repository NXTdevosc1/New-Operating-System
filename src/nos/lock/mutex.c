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
BOOLEAN KeMutexEnter(void* Thread, MUTEX* Mutex, UINT64 TimeoutInMs) {
    // Todo Dereference Thread Object
    if(Mutex->Owner == Thread) return TRUE;
    while(_interlockedbittestandset(&Mutex->AccessLock, 0)) _mm_pause();
    Mutex->Owner = Thread;
    return TRUE;
}
BOOLEAN KeMutexRelease(void* Thread, MUTEX* Mutex) {
    if(Mutex->Owner != Thread) return FALSE;
    Mutex->Owner = INVALID_HANDLE;
    Mutex->AccessLock = 0;
    return TRUE;
}