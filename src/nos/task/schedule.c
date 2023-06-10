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
    
    
    PETHREAD Thread = InternalData->CurrentThread;
    if(InternalData->NextThread) {
        Thread = InternalData->NextThread;
        InternalData->NextThread = NULL;
        return Thread;
    }

    READY_THREAD_DESCRIPTOR* ReadyThread = InternalData->Processor->ThreadQueue;
    while(ReadyThread) {
        KDebugPrint("Ready thread #%d , CPU %d", ReadyThread->Thread->ThreadId, ReadyThread->Thread->Processor->Id.ProcessorId);
        ReadyThread = ReadyThread->Next;
    }



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

PEPROCESS IdleProcess = NULL;

void KRNLAPI KeSchedulingSystemInit() {
    KDebugPrint("KERNEL Final Initialization Step called, Initializing the scheduling system...");
    _enable();

    PROCESSOR* Processor = KeGetCurrentProcessor();
    KDebugPrint("Current processor #%d INTERNAL_DATA %x", Processor->Id.ProcessorId, Processor->InternalData);
    // Register the schudling timer Interrupt Vector
    if(!KeRegisterSystemInterrupt(Processor->Id.ProcessorId, (UINT8*)&Processor->InternalData->SchedulingTimerIv, FALSE, TRUE, (INTERRUPT_SERVICE_HANDLER)SchedulerEntry)) {
        KDebugPrint("KeSchedulingSystemInit Failed : ERR0");
        while(1) __halt();
    }



    // Setup scheduler data in internal cpu structure
    Processor->InternalData->SchedulingEnabled = TRUE;

    if(!IdleProcess) {
        UINT64 ev = 0;
        // get first system thread (kernel init thread)
        Processor->InternalData->CurrentThread = KeWalkThreads(KernelProcess, &ev);

        if(NERROR(KeCreateProcess(NULL, &IdleProcess, 0, SUBSYSTEM_NATIVE, L"System Idle Process", L"", KiIdleThread))) {
            KDebugPrint("Create idle process failed.");
            while(1) __halt();
        }
        ev = 0;
        Processor->InternalData->IdleThread = KeWalkThreads(IdleProcess, &ev);
    } else {
        if(NERROR(KeCreateThread(IdleProcess, &Processor->InternalData->IdleThread, 0, KiIdleThread))) {
            KDebugPrint("failed to create idle thread");
            while(1) __halt();
        }
        Processor->InternalData->CurrentThread = Processor->InternalData->IdleThread;
    }
    

    CpuEnableApicTimer();
}

void KRNLAPI KeEnableScheduler() {

}

void KRNLAPI KeDisableScheduler() {
    
}

// These functions don't required to be atomic because they are executed on the same thread cpu
void ScTopAddReadyThread(PETHREAD Thread) {
    Thread->Ready.Previous = NULL;
    Thread->Ready.Next = Thread->Processor->ThreadQueue;
    Thread->Processor->ThreadQueue = &Thread->Ready;
    if(Thread->Processor->BottomOfThreadQueue) {
        Thread->Ready.Next->Previous = &Thread->Ready;
    } else {
        Thread->Processor->BottomOfThreadQueue = Thread->Processor->ThreadQueue;
    }
}
void ScBottomAddReadyThread(PETHREAD Thread) {
    Thread->Ready.Previous = Thread->Processor->BottomOfThreadQueue;
    Thread->Ready.Next = NULL;
    if(Thread->Processor->ThreadQueue) {
    Thread->Processor->BottomOfThreadQueue->Next = &Thread->Ready;
    Thread->Processor->BottomOfThreadQueue = &Thread->Ready;

    } else {
        Thread->Processor->ThreadQueue = &Thread->Ready;
        Thread->Processor->BottomOfThreadQueue = Thread->Processor->ThreadQueue;
    }
}

void ScRemoveFromReadyQueue(PETHREAD Thread) {
    Thread->Ready.Ready = FALSE;
    if(Thread->Processor->ThreadQueue == &Thread->Ready) {
        Thread->Processor->ThreadQueue = Thread->Ready.Next;
    }
    if(Thread->Processor->BottomOfThreadQueue == &Thread->Ready) {
        Thread->Processor->BottomOfThreadQueue = Thread->Ready.Previous;
    }
    if(Thread->Ready.Previous) {
        Thread->Ready.Previous->Next = Thread->Ready.Next;
    }
}