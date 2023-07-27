#include <nos/nos.h>
#include <nos/lock/lock.h>

UINT64 KRNLAPI ExAcquireSpinLock(SPINLOCK* SpinLock) {
    UINT64 CpuFlags = __readeflags();
    // We do this before pending for the spinlock 
    // in case of any bug the system will freeze
    while(_interlockedbittestandset(SpinLock, 0)) _mm_pause();
    _disable();
    return CpuFlags;
}
void KRNLAPI ExReleaseSpinLock(SPINLOCK* SpinLock, UINT64 CpuFlags) {
    
    _interlockedbittestandreset(SpinLock, 0);
    __writeeflags(CpuFlags);
}