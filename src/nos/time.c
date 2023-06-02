#include <nos/nos.h>
#include <ktime.h>
#include <intmgr.h>
#include <nos/pnp/pnp.h>
#include <nosio.h>
typedef struct _KTIMER {
    PDEVICE Device;
    POBJECT TimerObject;
    UINT Usage;
    UINT64 Frequency; // in HZ
    UINT64 TickCounter;
    HANDLE KernelHandle;
} KTIMER, *PKTIMER;

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

    if(NERROR(ObOpenHandle(Timer->Device->ObjectDescriptor, KernelProcess, -1, &Timer->KernelHandle))) {
        KDebugPrint("TIMER : OPEN_HANDLE Failed.");
        while(1) __halt();
    }
    *_OutTimer = Timer;
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
    Timer->TickCounter++;
}