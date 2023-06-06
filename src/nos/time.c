#include <nos/nos.h>
#include <ktime.h>
#include <intmgr.h>
#include <nos/pnp/pnp.h>
#include <nosio.h>
typedef volatile struct _KTIMER {
    PDEVICE Device;
    POBJECT TimerObject;
    UINT Usage;
    UINT64 Frequency; // in HZ
    UINT64 TickCounter;
    HANDLE KernelHandle;
} KTIMER, *PKTIMER;

PKTIMER BestCounter = NULL;
PKTIMER BestTimeAndDateSource = NULL;

NSTATUS TimerEvt(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 Access) {
    return STATUS_SUCCESS;
}

NSTATUS KRNLAPI KeCreateTimer(
    IN PDEVICE Device,
    IN UINT Usage, // Bit field
    IN UINT64 Frequency,
    OUT PKTIMER* _OutTimer
) {
    POBJECT Object;

    NSTATUS s = ObCreateObject(
        Device->ObjectDescriptor,
        &Object,
        0,
        OBJECT_TIMER,
        NULL,
        sizeof(KTIMER),
        TimerEvt
    );
    if(NERROR(s)) return s;
    PKTIMER Timer = Object->Address;
    Timer->Device = Device;
    Timer->Frequency = Frequency;
    Timer->Usage = Usage;
    Timer->TimerObject = Object;

    if(NERROR(ObOpenHandle(Timer->Device->ObjectDescriptor, KernelProcess, -1, (HANDLE)&Timer->KernelHandle))) {
        KDebugPrint("TIMER : OPEN_HANDLE Failed.");
        while(1) __halt();
    }
    *_OutTimer = Timer;
    if(Usage & TIMER_USAGE_COUNTER) {
        if(!BestCounter) {
            BestCounter = Timer;
        }
        else if(BestCounter->Frequency < Frequency) BestCounter = Timer;
    }
    if(Usage & TIMER_USAGE_TIMEDATE) {
        if(!BestTimeAndDateSource) BestTimeAndDateSource = Timer;
        else if(BestTimeAndDateSource->Frequency < Frequency) BestTimeAndDateSource = Timer;
    }
    return STATUS_SUCCESS;
}

UINT64 KRNLAPI KeReadCounter(
    IN PKTIMER Timer
) {
    if(!(Timer->Usage & TIMER_USAGE_COUNTER)) return (UINT64)-1;

    return IoProtocol(Timer->KernelHandle, TIMER_IO_READ_COUNTER);
}

void KRNLAPI KeTimerTick(
    PKTIMER Timer
) {
    _InterlockedIncrement64(&Timer->TickCounter);
}

#define MICROSCALE 1000000
#define MILLISCALE 1000

void KRNLAPI Stall(UINT64 MicroSeconds) {
    if(!MicroSeconds) return;
    if(!BestCounter) {
        KDebugPrint("STALL_FAILED : No Counter registred!");
        while(1) __halt();
    }
    _disable();
    UINT64 StopFraction = KeReadCounter(BestCounter) + (MicroSeconds % MICROSCALE) * ((BestCounter->Frequency >= MICROSCALE) ? (BestCounter->Frequency / MICROSCALE) : 1);
    UINT64 StopSecond = BestCounter->TickCounter + (MicroSeconds / MICROSCALE);
    if(StopFraction >= BestCounter->Frequency) // Handle possible overflow
    {
        StopFraction -= BestCounter->Frequency;
        StopSecond++;
    }
    _enable();
    // KDebugPrint("Stall : StopUS : %x StopS : %x", StopFraction, StopSecond);
    for(;;) {
        if(BestCounter->TickCounter > StopSecond) return;
        if(BestCounter->TickCounter == StopSecond && KeReadCounter(BestCounter) >= StopFraction) return;
        _mm_pause();
    }
}

void KRNLAPI Sleep(UINT64 Milliseconds) {

}

// returns time since boot in micro seconds
UINT64 KRNLAPI KeGetTimeSinceBoot() {
    if(!BestCounter) {
        KDebugPrint("KeGetTimeSinceBoot BUG0 HALT");
        while(1) __halt();
    }
    return (BestCounter->TickCounter * MICROSCALE) + (((double)KeReadCounter(BestCounter) / (double)BestCounter->Frequency) * MICROSCALE);
}