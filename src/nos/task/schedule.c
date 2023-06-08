#include <nos/nos.h>
#include <nos/task/process.h>
#include <nos/task/schedule.h>
/*
The kernel is required to work in APIC Mode
*/
void* LocalApicAddress = (void*)-1;

void KRNLAPI KiSetSchedulerData(
    void* _Lapic
) {
    KDebugPrint("Set Scheduler Data : LAPIC : %x", _Lapic);
    LocalApicAddress = _Lapic;
}

extern void SchedulerEntry();

void KRNLAPI KeSchedulingSystemInit() {
    KDebugPrint("KERNEL Final Initialization Step called, Initializing the scheduling system...");
    _enable();

    PROCESSOR* Processor = KeGetCurrentProcessor();

    // Register the schudling timer Interrupt Vector
    if(!KeRegisterSystemInterrupt(Processor->Id.ProcessorId, &Processor->InternalData->SchedulingTimerIv, FALSE, TRUE, (INTERRUPT_SERVICE_HANDLER)SchedulerEntry)) {
        KDebugPrint("KeSchedulingSystemInit Failed : ERR0");
        while(1) __halt();
    }

    CpuEnableApicTimer();
}

void KRNLAPI KeEnableScheduler() {

}

void KRNLAPI KeDisableScheduler() {
    
}