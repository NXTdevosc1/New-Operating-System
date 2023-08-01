#include <nos/nos.h>

typedef NSTATUS(__cdecl *SYSTEM_EVENT_HANDLER)(void *Context);

typedef struct _EVTHANDLERSTRUCT EVTHANDLERSTRUCT;
typedef struct _EVTSTRUCT EVTSTRUCT;
typedef struct _EVENT_DESCRIPTOR EVENT, *PEVENT;
typedef struct _EVTHANDLERSTRUCT
{
    UINT32 Running; // if TRUE then run events will skip
    SYSTEM_EVENT_HANDLER Handler;
    EVTHANDLERSTRUCT *Previous;
    EVTHANDLERSTRUCT *Next;
} EVTHANDLERSTRUCT;

typedef struct _EVTSTRUCT
{
    PEVENT Event;
    void *Context;
    EVTSTRUCT *Previous;
    EVTSTRUCT *Next;
} EVTSTRUCT;

typedef struct _EVENT_DESCRIPTOR
{
    MUTEX PreventDelete;
    SPINLOCK EventSignalSpinlock;
    UINT64 EventId;
    volatile UINT64 NumHandlers;
    volatile UINT64 NumEvents;

    EVTSTRUCT *Events;
    EVTSTRUCT *LastEvent;

    EVTHANDLERSTRUCT *Handlers;
    EVTHANDLERSTRUCT *LastHandler;

} EVENT, *PEVENT;

NSTATUS EvtObEvt(PEPROCESS Process, UINT Event, HANDLE Handle, UINT64 Access)
{
    return STATUS_SUCCESS;
}

static SPINLOCK sl = 0;

void RunEvents(PEVENT Event, EVTHANDLERSTRUCT *Handler)
{
    if (_interlockedbittestandset(&Handler->Running, 0))
        return;
    EVTSTRUCT *Evt = Event->Events;
    while (Evt)
    {
        // Run event handlers asynchronously in separate threads
        if (NERROR(KeCreateThread(KeGetCurrentProcess(), NULL, 0, Handler->Handler, Evt->Context)))
        {
            KDebugPrint("ERROR: EVENT FAILED TO CREATE THREAD");
        }
        Evt = Evt->Next;
    }
    _bittestandreset(&Handler->Running, 0);
}

BOOLEAN KRNLAPI KeRegisterEventHandler(
    UINT64 EventId,
    UINT64 Flags,
    SYSTEM_EVENT_HANDLER EventHandler)
{
    KDebugPrint("KeRegEVT");
    UINT64 cpf = ExAcquireSpinLock(&sl);
    UINT64 _ev = 0;
    POBJECT Obj;
    PEVENT Event;

    while ((_ev = ObEnumerateObjects(NULL, OBJECT_EVENT, &Obj, NULL, _ev)) != 0)
    {
        Event = Obj->Address;
        if (Event->EventId == EventId)
            goto RegisterHandler;
    }

    // Create the event
    // event object will autodelete when there are no handlers
    if (NERROR(ObCreateObject(
            NULL,
            &Obj,
            OBJECT_PERMANENT,
            OBJECT_EVENT,
            NULL,
            sizeof(PEVENT),
            EvtObEvt)))
    {
        KDebugPrint("Failed to create event object");
        while (1)
            __halt();
    }
    Event = Obj->Address;
    Event->EventId = EventId;
    // Register the handler
RegisterHandler:
    ExReleaseSpinLock(&sl, cpf);

    EVTHANDLERSTRUCT *evth = MmAllocatePool(sizeof(EVTHANDLERSTRUCT), 0);
    evth->Handler = EventHandler;
    evth->Previous = Event->LastHandler;
    evth->Next = NULL;

    if (!Event->Handlers)
    {
        Event->Handlers = evth;
        Event->LastHandler = evth;
    }
    else
    {
        Event->LastHandler->Next = evth;
        Event->LastHandler = evth;
    }

    // Run event handler for existing events
    RunEvents(Event, evth);
    return TRUE;
}

BOOLEAN KRNLAPI KeUnregisterEventHandler(
    UINT64 EventId,
    SYSTEM_EVENT_HANDLER EventHandler)
{
    KDebugPrint("KeUnregisterEventHandler : UNIMPLEMENTED");
    while (1)
        __halt();
    UINT64 cpf = ExAcquireSpinLock(&sl);
    // Find the event

    // ExMutexWait((void*)1, &Event, 0);
    // Find the handler

    ExReleaseSpinLock(&sl, cpf);
    return TRUE;
}

HANDLE KRNLAPI KeOpenEvent(UINT64 EventId)
{
    KDebugPrint("KeOpenEVT");
    UINT64 cpf = ExAcquireSpinLock(&sl);

    UINT64 _ev = 0;
    POBJECT Obj;
    PEVENT Event;

    while ((_ev = ObEnumerateObjects(NULL, OBJECT_EVENT, &Obj, NULL, _ev)) != 0)
    {
        Event = Obj->Address;
        if (Event->EventId == EventId)
        {
            HANDLE Handle;
            NSTATUS s = ObOpenHandle(Obj, NULL, 0, &Handle);
            ExReleaseSpinLock(&sl, cpf);
            if (s == STATUS_SUCCESS || s == STATUS_HANDLE_ALREADY_OPEN)
            {
                return Handle;
            }
            else
            {
                KDebugPrint("KeOpenEvent : BUG0");
                while (1)
                    __halt();
            }
        }
    }

    // Create the event
    if (NERROR(ObCreateObject(
            NULL,
            &Obj,
            OBJECT_PERMANENT,
            OBJECT_EVENT,
            NULL,
            sizeof(EVENT),
            EvtObEvt)))
    {
        KDebugPrint("Failed to create event object");
        while (1)
            __halt();
    }
    Event = Obj->Address;

    Event->EventId = EventId;

    ExReleaseSpinLock(&sl, cpf);
    return KeOpenEvent(EventId);
}

NSTATUS KRNLAPI KeSignalEvent(HANDLE EventHandle, void *Context, void **EventDescriptor)
{
    KDebugPrint("KeSignalEVT");
    POBJECT Obj;
    PEVENT Event;
    if (!(Event = ObGetObjectByHandle(EventHandle, &Obj, OBJECT_EVENT)))
        return STATUS_INVALID_PARAMETER;

    // Any thread can enter the mutex, but when deleting the event the thread value is set to (void*)1
    if (!ExMutexWait(NULL, &Event->PreventDelete, 0))
    {
        KDebugPrint("BUG0 : KE_SIGNAL_EVENT");
        while (1)
            __halt();
    }

    // Allocate an event
    EVTSTRUCT *Evt = MmAllocatePool(sizeof(EVTSTRUCT), 0);
    if (!Evt)
    {
        KDebugPrint("BUG1 : KE_SIGNAL_EVENT");
        while (1)
            __halt();
    }

    Evt->Event = Event;
    Evt->Context = Context;
    Evt->Previous = Event->LastEvent;
    Evt->Next = NULL;

    UINT64 cpf = ExAcquireSpinLock(&Event->EventSignalSpinlock);
    if (!Event->Events)
    {
        Event->Events = Evt;
        Event->LastEvent = Evt;
    }
    else
    {
        Event->LastEvent->Next = Evt;
        Event->LastEvent = Evt;
    }

    ExReleaseSpinLock(&Event->EventSignalSpinlock, cpf);

    EVTHANDLERSTRUCT *Handler = Event->Handlers;
    while (Handler)
    {
        RunEvents(Event, Handler);
        Handler = Handler->Next;
    }

    ExMutexRelease(NULL, &Event->PreventDelete);

    return STATUS_SUCCESS;
}

BOOLEAN KRNLAPI KeRemoveEvent(void *EventDescriptor)
{
    EVTSTRUCT *Evt = EventDescriptor;
    PEVENT Event = Evt->Event;
    if (!ExMutexWait(NULL, &Event->PreventDelete, 0))
    {
        KDebugPrint("BUG0 : KE_SIGNAL_EVENT");
        while (1)
            __halt();
    }
    // Unlink the event
    UINT64 cpf = ExAcquireSpinLock(&Event->EventSignalSpinlock);
    if (Event->Events == Evt)
    {
        Event->Events = NULL;
        Event->LastEvent = NULL;
    }
    else if (Event->LastEvent == Evt)
    {
        Evt->Previous->Next = NULL;
        Event->LastEvent = Evt->Previous;
    }
    else
    {
        Evt->Previous->Next = Evt->Next;
    }
    ExReleaseSpinLock(&Event->EventSignalSpinlock, cpf);
    ExMutexRelease(NULL, &Event->PreventDelete);
    // Free the event struct
    MmFreePool(Evt);

    return TRUE;
}