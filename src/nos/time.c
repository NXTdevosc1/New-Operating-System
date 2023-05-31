#include <nos/nos.h>
#include <ktime.h>
#include <intmgr.h>

// typedef struct _KTIMER {
//     PDEVICE Device;
//     UINT Usage;
//     UINT64 Frequency; // in HZ
//     UINT64 Ticks;
//     KTIMER_INTERFACE Interface;
//     PKTIMER Previous;
//     PKTIMER Next;
// } KTIMER, *PKTIMER;



// PKTIMER FirstTimer = NULL, LastTimer = NULL;

// SPINLOCK __ktimer_spinlock;


// NSTATUS KRNLAPI KeInstallTimer(
//     PDEVICE Device,
//     UINT Usage,
//     UINT64 Frequency, // in HZ
//     PKTIMER_INTERFACE Interface,
//     void* Context
// ) {
//     // TODO : Check device object

//     INTERRUPT_SERVICE_HANDLER Handler;
//     if(Usage == TIMER_USAGE_TIMEDATE)

//     UINT64 pflags = ExAcquireSpinLock(&__ktimer_spinlock);

//     // Now Select which handler you want to use
    

//     // if(NERROR(ExInstallInterruptHandler(
//     //     InterruptInformation->Fields.GlobalSystemInterrupt,
//     //     0,

//     // ))) {

//     // ExReleaseSpinLock(&__ktimer_spinlock, pflags);
//     // }

//     PKTIMER Timer = MmAllocatePool(sizeof(KTIMER), 0);
//     if(!Timer) return STATUS_UNSUFFICIENT_MEMORY;

//     Timer->Device = Device;
//     Timer->Usage = Usage;
//     Timer->Frequency = Frequency;
//     Timer->Previous = FirstTimer;
//     Timer->Next = NULL;
//     memcpy(&Timer->Interface, Interface, sizeof(KTIMER_INTERFACE));
//     if(FirstTimer) {
//         LastTimer->Next = Timer;
//         LastTimer = Timer;
//     } else {
//         FirstTimer = Timer;
//         LastTimer = Timer;
//     }

//     ExReleaseSpinLock(&__ktimer_spinlock, pflags);

//     return STATUS_SUCCESS;
// }

// BOOLEAN KRNLAPI KeRemoveTimer(
//     PKTIMER Timer
// ) {
//     if(Timer == FirstTimer) {
//         FirstTimer = NULL;
//         LastTimer = NULL;
//         MmFreePool(Timer);
//         return TRUE;
//     }
//     Timer->Previous->Next = Timer->Next;
//     MmFreePool(Timer);
//     return TRUE;
// }

// void KRNLAPI KeTimerTick(PKTIMER Timer) {
//     Timer->Ticks++;
// }

// void KRNLAPI KeGetTimeAndDate() {

// }