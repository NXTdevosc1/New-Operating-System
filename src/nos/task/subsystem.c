#include <nos/nos.h>
#include <nos/task/process.h>

NSTATUS NSYSAPI KeCreateSubsystem(
    IN UINT8 Subsystem,
    IN BOOLEAN OperatingMode,
    IN SUBSYSTEM_ENTRY_POINT EntryPoint,
    IN UINT64 NumPages
) {
    if(!EntryPoint || OperatingMode > 1) return STATUS_INVALID_PARAMETER;
    if(_interlockedbittestandset64(&Subsystems[Subsystem].Flags, 0)) return STATUS_ALREADY_REGISTRED;
    if(!NumPages && OperatingMode) return STATUS_INVALID_PARAMETER;
    Subsystems[Subsystem].OperatingMode = OperatingMode;
    Subsystems[Subsystem].EntryPoint = EntryPoint;
    Subsystems[Subsystem].NumPages = NumPages;
    return STATUS_SUCCESS;
}


NSTATUS NSYSAPI NativeSubsystemEntryPoint(void* EntryPoint) {
    while(1);
}

NSTATUS NSYSAPI ConsoleSubsystemEntryPoint(void* EntryPoint) {
    while(1);
}

void KiInitStandardSubsystems() {
    KeCreateSubsystem(
        SUBSYSTEM_NATIVE,
        KERNEL_MODE,
        NativeSubsystemEntryPoint,
        0
    );
    KeCreateSubsystem(
        SUBSYSTEM_USERMODE_CONSOLE,
        USER_MODE,
        ConsoleSubsystemEntryPoint,
        1
    );

    KiInitMultitaskingSubsystem();
}