#define SYSAPI __declspec(dllexport) __fastcall
#include <ddk.h>


HANDLE PciInterfaceHandle;

BOOLEAN PciGetInterface(PCI_DRIVER_INTERFACE* DriverInterface) {
    
}