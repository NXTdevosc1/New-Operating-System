#include <nos/nos.h>
#include <nos/task/schedule.h>
#include <nos/processor/hw.h>
/*
The kernel is required to work in APIC Mode
*/
void* LocalApicAddress = (void*)-1;

// Called by the scheduler
extern void __idle();

// void testddd() {
    
//     while(1) {
//         KDebugPrint("TTESSSST");
//         Sleep(1000);
//     } 
// }

extern PETHREAD __fastcall Schedule(PROCESSOR_INTERNAL_DATA* InternalData, BOOLEAN Alt) {
    PETHREAD Thread = NULL;
    // Check on next thread
    if(InternalData->NextThread) {
        Thread = InternalData->NextThread;
        InternalData->NextThread = NULL;
        return Thread;
    }
    // Check on sleeping threads
    PTHREAD_QUEUE_ENTRY Queue = InternalData->Processor->SleepQueue;
    if(Queue) {
        UINT64 Cnt = KeReadCounter(BestCounter);
        for(;Queue;Queue = Queue->Next) {
            // KDebugPrint("sleep queue looping");
            if(BestCounter->TickCounter < Queue->Thread->SleepUntil.TicksSinceBoot) continue;
            if(BestCounter->TickCounter == Queue->Thread->SleepUntil.TicksSinceBoot &&
            Cnt < Queue->Thread->SleepUntil.CounterValue
            ) continue;

            Thread = Queue->Thread;
            ScUnlinkSleepThread(Queue);

            KDebugPrint("Thread #%u Woke up : %ls", Thread->ThreadId, Thread->Process->ProcessDisplayName);
            // Thread->Registers.rip = testddd;

            break;
        }
    }
    // KDebugPrint("_____________________________");
    // Check on waiting threads
    if(!Thread) {
        Thread = InternalData->CurrentThread; // Use current thread to compare against other threads
        Queue = InternalData->Processor->ThreadQueue;
        if(!Queue) {
            // KDebugPrint("IDLING...");
            InternalData->CurrentThread = InternalData->IdleThread;
            *(void**)_AddressOfReturnAddress() = __idle;
            if(!Alt) {
                ApicWrite(0xB0, 0); // Send EOI
            }
            return (PETHREAD)InternalData;
        }
        if(!GetThreadFlag(Thread, THREAD_READY)) { // this thread just went to sleep
            Thread = Queue->Thread; // Set to the first ready thread
        }
        for(;Queue;Queue = Queue->Next) {
            if(Queue->Thread->DynamicPriority > Thread->DynamicPriority) Thread = Queue->Thread;
            if(Queue->Thread != InternalData->CurrentThread) Queue->Thread->DynamicPriority++;
        }
    }
    // We now selected a thread, this one should go back to the end of the list
    if(GetThreadFlag(Thread, THREAD_READY)) {
        ScUnlinkReadyThread(&Thread->QueueEntry);
    }
    ScLinkReadyThreadBottom(&Thread->QueueEntry);

    Thread->DynamicPriority = Thread->StaticPriority;

    // Now set current thread
    InternalData->CurrentThread = Thread;
    Thread->Registers.rflags |= 0x200;
    // KDebugPrint("SCHED %u CNT %x RIP %x RFLAGS %x CS %x RSP %x SS %x", Thread->ThreadId, Cnt, Thread->Registers.rip, Thread->Registers.rflags,
    // Thread->Registers.cs, Thread->Registers.rsp, Thread->Registers.ss
    // );
    return Thread;
}

void KRNLAPI KiSetLapicAddress(void* _Lapic) {
    LocalApicAddress = _Lapic;
}

void KRNLAPI KiSetSchedulerData(
    void* _Lapic
) {
    KDebugPrint("Set Scheduler Data : LAPIC : %x", _Lapic);
    LocalApicAddress = _Lapic;

    // Start using APIC ID to get a processor
    BootProcessor->ProcessorEnabled = TRUE;
    
    UINT64 _ev = 0;
    POBJECT Out;
    while((_ev = ObEnumerateObjects(NULL, OBJECT_PROCESSOR, &Out, NULL, _ev)) != 0) {
        KDebugPrint("Object %x", Out);
        RFPROCESSOR Processor = Out->Address;
        KDebugPrint("Processor %x", Processor);
        KDebugPrint("Processor#%d Characteristics : %x", Processor->Id.ProcessorId, Processor->Id.Characteristics);

        // Init the processor
        if(Processor->Id.Characteristics & PROCESSOR_BOOTABLE) {
            HwBootProcessor(Processor);
        }
    }
}

extern void SchedulerEntry();
extern void __AltSchedule();


PEPROCESS IdleProcess = NULL;

void KRNLAPI KeSchedulingSystemInit() {
    KDebugPrint("KERNEL Final Initialization Step called, Initializing the scheduling system...");
    _enable();

    PROCESSOR* Processor = KeGetCurrentProcessor();
    KDebugPrint("Current processor #%d INTERNAL_DATA %x", Processor->Id.ProcessorId, Processor->InternalData);
    // // Register the schudling timer Interrupt Vector
    if(!KeRegisterSystemInterrupt(Processor->Id.ProcessorId, (UINT8*)&Processor->InternalData->SchedulingTimerIv, FALSE, TRUE, (INTERRUPT_SERVICE_HANDLER)SchedulerEntry)) {
        KDebugPrint("KeSchedulingSystemInit Failed : ERR0");
        while(1) __halt();
    }


    {
        IDT_ENTRY* en = Processor->Idt + SYSINT_SCHEDULE;
        en->CodeSegment = 0x08;
        en->Ist = 2;
        en->Address0 = (UINT64)__AltSchedule;
        en->Address1 = (UINT64)__AltSchedule >> 16;
        en->Address2 = (UINT64)__AltSchedule >> 32;
        en->Type = InterruptGate;
        en->Present = 1;
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
    Processor->InternalData->IdleThread->StaticPriority = 0;
    Processor->InternalData->IdleThread->DynamicPriority = 0;

    

    CpuEnableApicTimer();

    // __halt();
    // __halt();
    // __halt();


    // while(1) {
    //     KDebugPrint("Trying sleep");
    //     Sleep(1000);
    // }
}

void __fastcall ScLinkReadyThreadBottom(PTHREAD_QUEUE_ENTRY Entry) {
    // KDebugPrint("LNK_RDY_BOTM");
    PETHREAD Thread = Entry->Thread;
    if(GetThreadFlag(Thread, THREAD_READY)) {
        KDebugPrint("SC_BUG0 0");
        while(1) __halt();
    }
    if(Thread->Processor->ThreadQueue) {
        Thread->Processor->BottomOfThreadQueue->Next = Entry;
        Entry->Previous = Thread->Processor->BottomOfThreadQueue;
        Thread->Processor->BottomOfThreadQueue = Entry;
    } else {
        Thread->Processor->ThreadQueue = Entry;
        Thread->Processor->BottomOfThreadQueue = Entry;
    }
    SetThreadFlag(Thread, THREAD_READY);
}

void __fastcall ScLinkSleepThreadBottom(PTHREAD_QUEUE_ENTRY Entry) {
    // KDebugPrint("LNK_SLP_BOTM");

    PETHREAD Thread = Entry->Thread;
    if(GetThreadFlag(Thread, THREAD_READY)) {
        KDebugPrint("SC_BUG0 1");
        while(1) __halt();
    }
    if(Thread->Processor->SleepQueue) {
        Thread->Processor->BottomOfSleepQueue->Next = Entry;
        Entry->Previous = Thread->Processor->BottomOfSleepQueue;
        Thread->Processor->BottomOfSleepQueue = Entry;
    } else {
        Thread->Processor->SleepQueue = Entry;
        Thread->Processor->BottomOfSleepQueue = Entry;
    }
}
void __fastcall ScUnlinkReadyThread(PTHREAD_QUEUE_ENTRY Entry) {
    // KDebugPrint("ULNK_RDY");

    // CASE 1 (if its the first entry)
    // CASE 2 (if its a middle entry)
    // CASE 3 (if its the last entry)
    // CASE 4 (if its the only entry)

    PETHREAD Thread = Entry->Thread;
    if(!GetThreadFlag(Thread, THREAD_READY)) {
        KDebugPrint("SC_BUG0 2");
        while(1) __halt();
    }
    if(Thread->Processor->ThreadQueue == Entry) {
        if(Thread->Processor->BottomOfThreadQueue == Entry) {
            // CASE 4
            Thread->Processor->ThreadQueue = NULL;
            Thread->Processor->BottomOfThreadQueue = NULL;
        } else {
            // CASE 1
            Thread->Processor->ThreadQueue = Entry->Next;
        }
    } else if(Thread->Processor->BottomOfThreadQueue == Entry) {
        // CASE 3
        Thread->Processor->BottomOfThreadQueue = Entry->Previous;
    } else {
        // CASE 2
        Entry->Previous->Next = Entry->Next;
        Entry->Next->Previous = Entry->Previous;
    }

    Entry->Previous = NULL;
    Entry->Next = NULL;

    ResetThreadFlag(Thread, THREAD_READY);
}

void __fastcall ScUnlinkSleepThread(PTHREAD_QUEUE_ENTRY Entry) {

    // KDebugPrint("UNLK SLEEP");

    // CASE 1 (if its the first entry)
    // CASE 2 (if its a middle entry)
    // CASE 3 (if its the last entry)
    // CASE 4 (if its the only entry)

    PETHREAD Thread = Entry->Thread;
    if(GetThreadFlag(Thread, THREAD_READY)) {
        KDebugPrint("SC_BUG0 3");
        while(1) __halt();
    }
    if(Thread->Processor->SleepQueue == Entry) {
        if(Thread->Processor->BottomOfSleepQueue == Entry) {
            // CASE 4
            Thread->Processor->SleepQueue = NULL;
            Thread->Processor->BottomOfSleepQueue = NULL;
        } else {
            // CASE 1
            Thread->Processor->SleepQueue = Entry->Next;
        }
    } else if(Thread->Processor->BottomOfSleepQueue == Entry) {
        // CASE 3
        Thread->Processor->BottomOfSleepQueue = Entry->Previous;
    } else {
        // CASE 2
        Entry->Previous->Next = Entry->Next;
        Entry->Next->Previous = Entry->Previous;
    }

    Entry->Previous = NULL;
    Entry->Next = NULL;
}