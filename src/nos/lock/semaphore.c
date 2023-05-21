#define __SEMDEFINED

typedef struct _SEMAPHORE {
    unsigned int MaxSlots; // the number of threads to access the semaphore
    unsigned int FullSlots;
} SEMAPHORE;
#include <nos/nos.h>


void KRNLAPI ExInitSemaphore(SEMAPHORE* Semaphore, UINT MaxSlots) {
    Semaphore->MaxSlots = MaxSlots;
    Semaphore->FullSlots = 0;
}
BOOLEAN KRNLAPI ExSemaphoreWait(SEMAPHORE* Semaphore, UINT64 TimeoutInMs) {
    // TODO : Implement Timeout
    while(!ExSemaphoreSignal(Semaphore)) _mm_pause();
    return TRUE;
}
BOOLEAN KRNLAPI ExSemaphoreSignal(SEMAPHORE* Semaphore) {
    if(Semaphore->FullSlots >= Semaphore->MaxSlots) return FALSE;
    UINT s = _InterlockedIncrement(&Semaphore->FullSlots);
    if(s > Semaphore->MaxSlots) {
        _InterlockedDecrement(&Semaphore->FullSlots);
        return FALSE;
    }
    return TRUE;
}
