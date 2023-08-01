#include <nos/nos.h>
#include <nos/processor/processor.h>



void NosSystemInterruptHandler(UINT64 InterruptNumber, void* InterruptStack) {
    KDebugPrint("SYSTEM_INTERRUPT !");
    PROCESSOR* Processor = KeGetCurrentProcessor();
    Processor->State = PROCESSOR_STATE_INTERRUPT;

    INTERRUPT_HANDLER_DATA HandlerData = {0};
    HandlerData.InterruptNumber = InterruptNumber;
    HandlerData.ProcessorId = Processor->Id.ProcessorId;
    HandlerData.Thread = Processor->InternalData->CurrentThread;
    
    NSTATUS Status = Processor->SystemInterruptHandlers[InterruptNumber](&HandlerData);

    Processor->State = PROCESSOR_STATE_NORMAL;
}

extern NSTATUS KiRemoteExecuteHandler(INTERRUPT_HANDLER_DATA* HandlerData) {
    KDebugPrint("Remote Execute on processor#%u", HandlerData->ProcessorId);
    PROCESSOR* Processor = KeGetCurrentProcessor();
    NSTATUS Status = Processor->RemoteExecute.Routine(Processor->RemoteExecute.Context);
    if(Processor->RemoteExecute.Waiting) {
        KDebugPrint("Processor was waiting");
        Processor->RemoteExecute.ReturnCode = Status;
        Processor->RemoteExecute.Finished = TRUE;
        __wbinvd(); // reset cache
    } else {
        KDebugPrint("Processor was not waiting");

        // Just release the control
        ReleaseProcessorControl(Processor, PROCESSOR_CONTROL_EXECUTE);
    }
    return STATUS_SUCCESS;
}