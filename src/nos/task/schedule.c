#include <nos/nos.h>
#include <nos/task/process.h>
#include <nos/task/schedule.h>
/*
The kernel is required to work in APIC Mode
*/
void* LocalApicAddress = (void*)-1;

// Called by the scheduler
extern void __idle();

extern PETHREAD __fastcall Schedule(PROCESSOR_INTERNAL_DATA* InternalData) {
    PETHREAD Thread = InternalData->CurrentThread;
    
    if(InternalData->NextThread) {
        Thread = InternalData->NextThread;
        InternalData->NextThread = NULL;
        return Thread;
    }

    READY_THREAD_DESCRIPTOR* ReadyThread = InternalData->Processor->ThreadQueue;
    
    if(!ReadyThread) {
// Run Idle Thread
        InternalData->CurrentThread = InternalData->IdleThread;
        *(void**)_AddressOfReturnAddress() = __idle;
        return (PETHREAD)InternalData;
    }

// Searches for a thread with higher dynamic priority and increase DP for all ready threads
    while(ReadyThread) {
        if(ReadyThread->Thread->DynamicPriority > Thread->DynamicPriority) Thread = ReadyThread->Thread;
        if(ReadyThread->Thread != InternalData->CurrentThread) ReadyThread->Thread->DynamicPriority++;
        ReadyThread = ReadyThread->Next;
    }

// Check if we have preempted the current running thread
    if(InternalData->CurrentThread != Thread) {
        // Move it to the end of the ready queue
        ScRemoveFromReadyQueue(InternalData->CurrentThread);
        ScBottomAddReadyThread(InternalData->CurrentThread);
    }
    InternalData->CurrentThread->DynamicPriority = InternalData->CurrentThread->StaticPriority;

    InternalData->CurrentThread = Thread;
    return InternalData->CurrentThread;
}

void KRNLAPI KiSetSchedulerData(
    void* _Lapic
) {
    KDebugPrint("Set Scheduler Data : LAPIC : %x", _Lapic);
    LocalApicAddress = _Lapic;
}

extern void SchedulerEntry();



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

        if(NERROR(KeCreateProcess(NULL, &IdleProcess, PROCESS_CREATE_IDLE, SUBSYSTEM_NATIVE, L"System Idle Process", L"", __idle))) {
            KDebugPrint("Create idle process failed.");
            while(1) __halt();
        }
        ev = 0;
        Processor->InternalData->IdleThread = KeWalkThreads(IdleProcess, &ev);
    } else {
        if(NERROR(KeCreateThread(IdleProcess, &Processor->InternalData->IdleThread, THREAD_CREATE_IDLE, __idle, NULL))) {
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
inline void ScTopAddReadyThread(PETHREAD Thread) {
    Thread->Ready.Previous = NULL;
    Thread->Ready.Next = Thread->Processor->ThreadQueue;
    Thread->Processor->ThreadQueue = &Thread->Ready;
    if(Thread->Processor->BottomOfThreadQueue) {
        Thread->Ready.Next->Previous = &Thread->Ready;
    } else {
        Thread->Processor->BottomOfThreadQueue = Thread->Processor->ThreadQueue;
    }
    Thread->Ready.Ready = TRUE;
}
inline void ScBottomAddReadyThread(PETHREAD Thread) {
    Thread->Ready.Previous = Thread->Processor->BottomOfThreadQueue;
    Thread->Ready.Next = NULL;
    if(Thread->Processor->ThreadQueue) {
        Thread->Processor->BottomOfThreadQueue->Next = &Thread->Ready;
        Thread->Processor->BottomOfThreadQueue = &Thread->Ready;
    } else {
        Thread->Processor->ThreadQueue = &Thread->Ready;
        Thread->Processor->BottomOfThreadQueue = Thread->Processor->ThreadQueue;
    }
    Thread->Ready.Ready = TRUE;
}

inline void ScRemoveFromReadyQueue(PETHREAD Thread) {
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
    if(Thread->Ready.Next) {
        Thread->Ready.Next->Previous = Thread->Ready.Previous;
    }
}