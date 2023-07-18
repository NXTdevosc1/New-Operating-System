#include <ddk.h>
#include <AcpiSystem/acpi.h>
#include <pcidef.h>
typedef struct acpi_table_mcfg
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT8                   Reserved[8];

} ACPI_TABLE_MCFG;


/* Subtable */

typedef struct acpi_mcfg_allocation
{
    UINT64                  Address;            /* Base address, processor-relative */
    UINT16                  PciSegment;         /* PCI segment group number */
    UINT8                   StartBusNumber;     /* Starting PCI Bus number */
    UINT8                   EndBusNumber;       /* Final PCI Bus number */
    UINT32                  Reserved;

} ACPI_MCFG_ALLOCATION;

char* PcieUnsupported = "PCIe Memory Mapped Configuration Not Supported, USING Standard PCI Configuration";

UINT32 NumPciSegments = 0;
ACPI_MCFG_ALLOCATION* PciSegments[0x10000] = {0};

PCI_DEVICE_INDEPENDENT_REGION* PciGetConfiguration(
    UINT16 Segment, UINT8 Bus, UINT8 Device, UINT8 Function
) {
    ACPI_MCFG_ALLOCATION* Allocation = PciSegments[Segment];
    if(!Allocation) return NULL;
    if(Bus > Allocation->EndBusNumber || Device > 31 || Function > 7) return NULL;
    return (PCI_DEVICE_INDEPENDENT_REGION*)(Allocation->Address | ((UINT64)Bus << 20) | ((UINT64)Device << 15) | ((UINT64)Function << 12));
}

UINT64 _PciRead64(PCI_DEVICE_LOCATION* Location, UINT16 Offset) {

}
UINT32 _PciRead32(PCI_DEVICE_LOCATION* Location, UINT16 Offset) {

}
UINT16 _PciRead16(PCI_DEVICE_LOCATION* Location, UINT16 Offset) {

}
UINT8 _PciRead8(PCI_DEVICE_LOCATION* Location, UINT16 Offset) {

}

void _PciWrite64(PCI_DEVICE_LOCATION* Location, UINT16 Offset, UINT64 Value) {

}
void _PciWrite32(PCI_DEVICE_LOCATION* Location, UINT16 Offset, UINT32 Value) {

}
void _PciWrite16(PCI_DEVICE_LOCATION* Location, UINT16 Offset, UINT16 Value) {

}
void _PciWrite8(PCI_DEVICE_LOCATION* Location, UINT16 Offset, UINT8 Value) {

}

UINT16 _PciGetConfigurationByIndex(UINT16 Index) {

}

UINT MaxPciSegment = 0;

HANDLE PciDeviceAddEvent;

PDEVICE PciDeviceObject;

// #define PCI_IO_GET_CONFIGURATION 0


IORESULT PciIoCallback(IOPARAMS) {
    if(NumParameters != 1) KeRaiseException(STATUS_INVALID_PARAMETER);
    PCI_DRIVER_INTERFACE* DriverIf = Parameters[0];
    ObjZeroMemory(DriverIf);
    if(NumPciSegments) {
        DriverIf->NumConfigurations = NumPciSegments;
        DriverIf->IsMemoryMapped = TRUE;
    } else {
        DriverIf->NumConfigurations = 1;
        DriverIf->IsMemoryMapped = FALSE;
    }

    DriverIf->GetMemoryMappedConfiguration = PciGetConfiguration;
    DriverIf->GetConfigurationByIndex = _PciGetConfigurationByIndex;
    DriverIf->Read64 = _PciRead64;
    DriverIf->Read32 = _PciRead32;
    DriverIf->Read16 = _PciRead16;
    DriverIf->Read8 = _PciRead8;

    DriverIf->Write64 = _PciRead64;
    DriverIf->Write32 = _PciWrite32;
    DriverIf->Write16 = _PciWrite16;
    DriverIf->Write8 = _PciWrite8;

    return (IORESULT)TRUE;
}

NSTATUS DriverEntry(PDRIVER Driver) {
    KDebugPrint("PCI Driver startup");
    // Enumerate PCIE

    ACPI_TABLE_MCFG* Mcfg;
    if(!AcpiGetTable("MCFG", 0, &Mcfg)) {
        KDebugPrint(PcieUnsupported);
        return STATUS_UNSUPPORTED;
    }

    KDebugPrint("MCFG %x", Mcfg);

    NumPciSegments = (Mcfg->Header.Length - sizeof(ACPI_TABLE_MCFG)) / sizeof(ACPI_MCFG_ALLOCATION);
    if(!NumPciSegments) {
        KDebugPrint(PcieUnsupported);
        return STATUS_UNSUPPORTED;
    }
    KDebugPrint("MCFG : %x NumSegments : %d", Mcfg, NumPciSegments);

    // The PCI Memory Mapped Configuration Should be already mapped by the ACPI Driver
    ACPI_MCFG_ALLOCATION* McfgAllocation = (ACPI_MCFG_ALLOCATION*)(Mcfg + 1);
    for(UINT i = 0;i<NumPciSegments;i++, McfgAllocation++) {
        PciSegments[McfgAllocation->PciSegment] = McfgAllocation;
        if(McfgAllocation->PciSegment > MaxPciSegment) MaxPciSegment = McfgAllocation->PciSegment;
    }

    PCI_DEVICE_INDEPENDENT_REGION* Ir;

    KDebugPrint("Max PCI Segment : %d", MaxPciSegment);

    PciDeviceAddEvent = KeOpenEvent(SYSTEM_EVENT_PCI_DEVICE_ADD);

    // Create Main Pci Device
    PciDeviceObject = KeCreateDevice(DEVICE_PIPE, 0, L"Peripheral Component Interconnect (PCI)", NULL);

    IO_INTERFACE_DESCRIPTOR If = {0};
    If.NumFunctions = 2;
    If.IoCallback = PciIoCallback;
    IoSetInterface(PciDeviceObject, &If);


    // Enumerate each segment
    for(UINT Segment = 0;Segment<=MaxPciSegment;Segment++) {
        if(!PciSegments[Segment]) continue;
        McfgAllocation = PciSegments[Segment];
        const UINT8 StartBus = McfgAllocation->StartBusNumber, EndBus = McfgAllocation->EndBusNumber;
        for(UINT16 Bus = StartBus;Bus<= EndBus;Bus++) {
            for(UINT8 Device = 0;Device < 32; Device++) {
                UINT8 NumFunctions = 8;
                Ir = PciGetConfiguration(Segment, Bus, Device, 0);
                if(Ir->HeaderType == 0xFF) continue;
                if(!(Ir->HeaderType & 0x80)) NumFunctions = 1;
                for(UINT8 Function = 0;Function<NumFunctions;Function++) {
                    Ir = PciGetConfiguration(Segment, Bus, Device, Function);
                    if(Ir->HeaderType == 0xFF) continue;
                    UINT8 HeaderType = Ir->HeaderType & 0x7F;
                    if(Ir->HeaderType == 0) {
                        // PCI Device
                        PCI_TYPE00* DeviceConfig = (PCI_TYPE00*)Ir;
                        KDebugPrint("At Seg %d Bus %d Device %d Function %d", Segment, Bus, Device, Function);
                        KDebugPrint("PCI Device Class %d Subclass %d ProgIf %d", DeviceConfig->Hdr.ClassCode[2], DeviceConfig->Hdr.ClassCode[1], DeviceConfig->Hdr.ClassCode[0]);
                    
                        PCI_DEVICE_ADD_CONTEXT* Ctx = MmAllocatePool(sizeof(PCI_DEVICE_ADD_CONTEXT), 0);
                        Ctx->Segment = Segment;
                        Ctx->Bus = Bus;
                        Ctx->Device = Device;
                        Ctx->Function = Function;
                        void* _dump;
                        
                        // event desc wont be saved because these are permanent event signals
                        KeSignalEvent(PciDeviceAddEvent, Ctx, &_dump);
                    } else if(Ir->HeaderType == 1) {
                        // PCI-to-PCI Bridge

                    }
                }
            }
        }
    }

    return STATUS_SUCCESS;
}
