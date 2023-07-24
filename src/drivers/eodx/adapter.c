#include <eodxdriver.h>

UINT64 _LastAdapterId = 0;

PEODX_ADAPTER FirstAdapter = NULL;
PEODX_ADAPTER LastAdapter = NULL;


/*
 * CHARACTER : SYNCHRONOUS
*/
PEODX_ADAPTER iEodxCreateDisplayAdapter(
    IN UINT16* AdapterName,
    IN UINT64 Characteristics, 
    IN UINT16 Version,
    IN OPT void* Context,
    IN EODX_ADAPTER_CALLBACK AdapterCallback
) {
    UINT8 Len = wcslen(AdapterName);
    if(Len > 120) KeRaiseException(STATUS_INVALID_PARAMETER);
    PEODX_ADAPTER Adapter = AllocateNullPool(sizeof(EODX_ADAPTER));
    if(!Adapter) KeRaiseException(STATUS_OUT_OF_MEMORY);

    Adapter->Id = _InterlockedIncrement64(&_LastAdapterId) - 1;
    memcpy(Adapter->AdapterName, AdapterName, (Len + 1) << 1);
    Adapter->Version = Version;
    Adapter->Context = Context;
    Adapter->Callback = AdapterCallback;

    Adapter->Previous = LastAdapter;
    if(FirstAdapter) {
        LastAdapter->Next = Adapter;
        LastAdapter = Adapter;
    } else {
        FirstAdapter = Adapter;
        LastAdapter = Adapter;
    }

    return Adapter;
}

