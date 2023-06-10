#include <nos/nos.h>
#include <nos/task/process.h>
#include <nos/task/schedule.h>
/*
The kernel is required to work in APIC Mode
*/
void* LocalApicAddress = (void*)-1;

// Called by the scheduler
static UINT64 s = 0;
extern PETHREAD __fastcall Schedule(PROCESSOR_INTERNAL_DATA* InternalData) {
    _Memset128A_32((UINT32*)NosInitData->FrameBuffer.BaseAddress, s & 1 ? 0xFF : 0x20, (NosInitData->FrameBuffer.Pitch * 4 * NosInitData->FrameBuffer.VerticalResolution) / 0x40);
    s++;
    KDebugPrint("Schedule CALLED, TH : %x , INTERNAL_DATA : %x , APIC_ID : %x", InternalData->CurrentThread, InternalData, InternalData->Processor->Id.ProcessorId);
    return InternalData->CurrentThread;
}

void KRNLAPI KiSetSchedulerData(
    void* _Lapic
) {
    KDebugPrint("Set Scheduler Data : LAPIC : %x", _Lapic);
    LocalApicAddress = _Lapic;
}

extern void SchedulerEntry();

void KiIdleThread() {
    for(;;) __halt();
}

void KRNLAPI KeSchedulingSystemInit() {
    KDebugPrint("KERNEL Final Initialization Step called, Initializing the scheduling system...");
    _enable();

    PROCESSOR* Processor = KeGetCurrentProcessor();

    // Register the schudling timer Interrupt Vector
    if(!KeRegisterSystemInterrupt(Processor->Id.ProcessorId, (UINT8*)&Processor->InternalData->SchedulingTimerIv, FALSE, TRUE, (INTERRUPT_SERVICE_HANDLER)SchedulerEntry)) {
        KDebugPrint("KeSchedulingSystemInit Failed : ERR0");
        while(1) __halt();
    }

    // Setup scheduler data in internal cpu structure
    Processor->InternalData->SchedulingEnabled = TRUE;
    Processor->InternalData->CurrentThread = KeGetThreadById(0);

    CpuEnableApicTimer();
}

void KRNLAPI KeEnableScheduler() {

}

void KRNLAPI KeDisableScheduler() {
    
}