#pragma once
#include <nosdef.h>
#include <pnp.h>

typedef volatile struct _KTIMER KTIMER, *PKTIMER;

// void (_KTIMER_)

// TIMER USAGE

/*
The kernel will select the most precise timer to get time and date
*/
#define TIMER_USAGE_TIMEDATE 1
/*
This timer is used as a counter and does not specify a time and date
*/
#define TIMER_USAGE_COUNTER 2



/*
Typically the APIC Timer or TSC Deadline
These timers are closer to the cpu which makes access faster

Only One Schedulling Timer is allowed and it is installed by the ACPI Subsystem
*/

#define TIMER_IO_READ_COUNTER 0
#define TIMER_IO_GET_TIME_AND_DATE 1
#define TIMER_IO_SET_TIME_AND_DATE 2


NSTATUS KRNLAPI KeCreateTimer(
    IN PDEVICE Device,
    IN UINT Usage, // Bit field
    IN UINT64 Frequency,
    OUT PKTIMER* _OutTimer
);

UINT64 KRNLAPI KeReadCounter(
    IN PKTIMER Timer
);

void KRNLAPI KeTimerTick(
    PKTIMER Timer
);

UINT64 KRNLAPI KeGetTimeSinceBoot();

void KRNLAPI Stall(UINT64 MicroSeconds);
void KRNLAPI Sleep(UINT64 Milliseconds);

typedef struct _TIMESTRUCT {
    UINT64 Second : 6;
    UINT64 Minute : 6;
    UINT64 Hour : 5;
    UINT64 WeekDay : 3;
    UINT64 Day : 5;
    UINT64 Month : 4;
    UINT64 Year : 15;
} TIMESTRUCT;

UINT64 KRNLAPI KeGetTimerFrequency(PKTIMER Timer);