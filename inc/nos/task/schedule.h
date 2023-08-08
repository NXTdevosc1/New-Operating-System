#pragma once
#include <nosdef.h>
#include <nos/task/process.h>

void KiInitScheduler();
void KRNLAPI KeEnableScheduler();
void KRNLAPI KeDisableScheduler();

// inline void __fastcall ScLinkReadyThreadTop(PTHREAD_QUEUE_ENTRY Entry);
void __fastcall ScLinkReadyThreadBottom(PTHREAD_QUEUE_ENTRY Entry);
void __fastcall ScUnlinkReadyThread(PTHREAD_QUEUE_ENTRY Entry);

// inline void __fastcall ScLinkSleepThreadTop(PTHREAD_QUEUE_ENTRY Entry);
void __fastcall ScLinkSleepThreadBottom(PTHREAD_QUEUE_ENTRY Entry);
void __fastcall ScUnlinkSleepThread(PTHREAD_QUEUE_ENTRY Entry);

extern void KRNLAPI __Schedule();