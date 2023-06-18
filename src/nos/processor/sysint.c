#include <nos/nos.h>
#include <nos/processor/processor.h>



void NosSystemInterruptHandler(UINT64 InterruptNumber, void* InterruptStack) {
    KDebugPrint("SYSTEM_INTERRUPT !");
    PROCESSOR* Processor = KeGetCurrentProcessor();
    INTERRUPT_HANDLER_DATA HandlerData = {0};
    HandlerData.InterruptNumber = InterruptNumber;
    HandlerData.ProcessorId = Processor->Id.ProcessorId;
    HandlerData.Thread = Processor->InternalData->CurrentThread;
    
    NSTATUS Status = Processor->SystemInterruptHandlers[InterruptNumber](&HandlerData);
    while(1) __halt();
}