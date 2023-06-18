#include <eodxsys.h>
#include <ieodx.h>
#include <ddk.h>
EODXINSTANCE SEodx3dInit(EODXINITDATA* EodxInit) {
    // PEODXVIEWPORT ViewPort = 
    EODXENGINE* Engine = MmAllocatePool(sizeof(EODXENGINE), 0);
    ObjZeroMemory(Engine);
    Engine->Characteristics = EodxInit->Characteristics;
    
}