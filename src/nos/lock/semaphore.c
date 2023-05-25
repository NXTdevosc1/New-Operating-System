#define __SEMDEFINED

typedef struct _SEMAPHORE {
    volatile long Mutex;
    volatile unsigned long Count;
} SEMAPHORE;
#include <nos/nos.h>


void KRNLAPI ExInitSemaphore(SEMAPHORE* Semaphore, UINT InitialCount) {
    Semaphore->Mutex = 0;
    Semaphore->Count = InitialCount;
}
BOOLEAN KRNLAPI ExSemaphoreWait(SEMAPHORE* Semaphore, UINT64 TimeoutInMs) {
    // TODO : Implement Timeout
Retry:
    while(!Semaphore->Count) _mm_pause();
    while(_bittestandset(&Semaphore->Mutex, 0)) _mm_pause();
    if(!Semaphore->Count) {
        Semaphore->Mutex = 0;
        goto Retry;
    }
    Semaphore->Count--;
    Semaphore->Mutex = 0;
    return TRUE;
}
BOOLEAN KRNLAPI ExSemaphoreSignal(SEMAPHORE* Semaphore) {
    _InterlockedIncrement(&Semaphore->Count);
    return TRUE;
}
