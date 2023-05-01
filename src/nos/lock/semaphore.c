#define __SEMDEFINED

typedef struct _SEMAPHORE {
    unsigned int MaxSlots; // the number of threads to access the semaphore
    unsigned int FullSlots;
} SEMAPHORE;
#include <nos/nos.h>


void KRNLAPI KeInitSemaphore(SEMAPHORE* Semaphore, UINT MaxSlots) {
    Semaphore->MaxSlots = MaxSlots;
    Semaphore->FullSlots = 0;
}
BOOLEAN KRNLAPI KeSemaphoreWait(SEMAPHORE* Semaphore, UINT64 TimeoutInMs) {
    // TODO : Implement Timeout
    while(!KeSemaphoreSignal(Semaphore)) _mm_pause();
    return TRUE;
}
BOOLEAN KRNLAPI KeSemaphoreSignal(SEMAPHORE* Semaphore) {
    if(Semaphore->FullSlots >= Semaphore->MaxSlots) return FALSE;
    UINT s = _InterlockedIncrement(&Semaphore->FullSlots);
    if(s > Semaphore->MaxSlots) {
        _InterlockedDecrement(&Semaphore->FullSlots);
        return FALSE;
    }
    return TRUE;
}
