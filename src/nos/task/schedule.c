#include <nos/nos.h>
#include <nos/task/internal.h>

/*
The kernel is required to work in APIC Mode
*/
static void* LocalApicAddress = (void*)-1;

void KRNLAPI KiSetSchedulerData(
    void* _Lapic
) {
    KDebugPrint("Set Scheduler Data : LAPIC : %x", _Lapic);
    LocalApicAddress = _Lapic;
}

void KiInitScheduler(){

}

void KRNLAPI KeEnableScheduler() {

}

void KRNLAPI KeDisableScheduler() {
    
}