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
// Because ACPI memory maybe slow
UINT64 SegmentAddresses[0x10000] = {0};


UINT16 MaxPciSegment = 0;

HANDLE PciDeviceAddEvent;

PDEVICE PciDeviceObject;

PCI_DEVICE_INDEPENDENT_REGION* PciGetConfiguration(
    UINT16 Segment, UINT8 Bus, UINT8 Device, UINT8 Function
) {
    ACPI_MCFG_ALLOCATION* Allocation = PciSegments[Segment];
    if(!Allocation) return NULL;
    if(Bus > Allocation->EndBusNumber || Device > 31 || Function > 7) {
        KDebugPrint("PCI BUG0");
        while(1) __halt();
    }
    return (PCI_DEVICE_INDEPENDENT_REGION*)(Allocation->Address | ((UINT64)Bus << 20) | ((UINT64)Device << 15) | ((UINT64)Function << 12));
}

// CURRENTLY LEGACY PCI IS NOT IMPLEMENTED


UINT32 _PciRead32(PCI_DEVICE_LOCATION* Location, UINT16 Offset) {
    if(!SegmentAddresses[Location->Fields.Segment]) {
        KeRaiseException(STATUS_INVALID_PCI_ACCESS);
    }
    return *(UINT32*)((Location->Address & 0xFFFF000) | SegmentAddresses[Location->Fields.Segment] | Offset);
}
UINT64 _PciRead64(PCI_DEVICE_LOCATION* Location, UINT16 Offset) {
    UINT64 ret = _PciRead32(Location, Offset);
    ret |= ((UINT64)_PciRead32(Location, Offset + 4)) << 32;
    return ret;
}
UINT16 _PciRead16(PCI_DEVICE_LOCATION* Location, UINT16 Offset) {
    if(!SegmentAddresses[Location->Fields.Segment]) {
        KeRaiseException(STATUS_INVALID_PCI_ACCESS);
    }
    return *(UINT16*)((Location->Address & 0xFFFF000) | SegmentAddresses[Location->Fields.Segment] | Offset);
}
UINT8 _PciRead8(PCI_DEVICE_LOCATION* Location, UINT16 Offset) {
    if(!SegmentAddresses[Location->Fields.Segment]) {
        KeRaiseException(STATUS_INVALID_PCI_ACCESS);
    }
    return *(UINT8*)((Location->Address & 0xFFFF000) | SegmentAddresses[Location->Fields.Segment] | Offset);
}

void _PciWrite32(PCI_DEVICE_LOCATION* Location, UINT16 Offset, UINT32 Value) {
    if(!SegmentAddresses[Location->Fields.Segment]) {
            KeRaiseException(STATUS_INVALID_PCI_ACCESS);
        }
    *(UINT32*)((Location->Address & 0xFFFF000) | SegmentAddresses[Location->Fields.Segment] | Offset) = Value;
}
void _PciWrite64(PCI_DEVICE_LOCATION* Location, UINT16 Offset, UINT64 Value) {
    _PciWrite32(Location, Offset, Value);
    _PciWrite32(Location, Offset + 4, Value >> 32);
}
void _PciWrite16(PCI_DEVICE_LOCATION* Location, UINT16 Offset, UINT16 Value) {
    if(!SegmentAddresses[Location->Fields.Segment]) {
            KeRaiseException(STATUS_INVALID_PCI_ACCESS);
        }
    *(UINT16*)((Location->Address & 0xFFFF000) | SegmentAddresses[Location->Fields.Segment] | Offset) = Value;
}
void _PciWrite8(PCI_DEVICE_LOCATION* Location, UINT16 Offset, UINT8 Value) {
    if(!SegmentAddresses[Location->Fields.Segment]) {
            KeRaiseException(STATUS_INVALID_PCI_ACCESS);
        }
    *(UINT8*)((Location->Address & 0xFFFF000) | SegmentAddresses[Location->Fields.Segment] | Offset) = Value;
}

UINT16 _PciGetConfigurationByIndex(UINT16 Index) {
    UINT16 Config = 0;
    for(;;) {
        if(PciSegments[Config]) {
            Index--;
            if(!Index) break;
        }
        if(Config == 0xFFFF) return (UINT16)-1;
        Config++;
    }
    return Config;
}


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

    DriverIf->Write64 = _PciWrite64;
    DriverIf->Write32 = _PciWrite32;
    DriverIf->Write16 = _PciWrite16;
    DriverIf->Write8 = _PciWrite8;
    KDebugPrint("PCI IO DRVIF %x", DriverIf);

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
        SegmentAddresses[McfgAllocation->PciSegment] = McfgAllocation->Address;
        if(McfgAllocation->PciSegment > MaxPciSegment) MaxPciSegment = McfgAllocation->PciSegment;
    }

    PCI_DEVICE_INDEPENDENT_REGION* Ir;

    KDebugPrint("Max PCI Segment : %d", MaxPciSegment);

    PciDeviceAddEvent = KeOpenEvent(SYSTEM_EVENT_DEVICE_ADD);

    // Create Main Pci Device
    PciDeviceObject = KeCreateDevice(VIRTUAL_DEVICE, 0, L"Peripheral Component Interconnect (PCI)", NULL);

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
                    if(HeaderType == 0) {
                        // PCI Device
                        PCI_TYPE00* DeviceConfig = (PCI_TYPE00*)Ir;
                        KDebugPrint("At Seg %d Bus %d Device %d Function %d", Segment, Bus, Device, Function);
                        KDebugPrint("PCI Device Class %d Subclass %d ProgIf %d", DeviceConfig->Hdr.ClassCode[2], DeviceConfig->Hdr.ClassCode[1], DeviceConfig->Hdr.ClassCode[0]);
                    


                        SYSTEM_DEVICE_ADD_CONTEXT* Ctx = MmAllocatePool(sizeof(SYSTEM_DEVICE_ADD_CONTEXT), 0);
                        Ctx->BusType = BUS_PCI;
                        Ctx->DeviceData.PciDeviceData.PciDeviceLocation.Address = 0;

                        Ctx->DeviceData.PciDeviceData.PciDeviceLocation.Fields.Segment = Segment;
                        Ctx->DeviceData.PciDeviceData.PciDeviceLocation.Fields.Bus = Bus;
                        Ctx->DeviceData.PciDeviceData.PciDeviceLocation.Fields.Device = Device;
                        Ctx->DeviceData.PciDeviceData.PciDeviceLocation.Fields.Function = Function;



                        Ctx->DeviceData.PciDeviceData.VendorId = DeviceConfig->Hdr.VendorId;
                        Ctx->DeviceData.PciDeviceData.DeviceId = DeviceConfig->Hdr.DeviceId;
                        Ctx->DeviceData.PciDeviceData.Class = DeviceConfig->Hdr.ClassCode[2];
                        Ctx->DeviceData.PciDeviceData.Subclass = DeviceConfig->Hdr.ClassCode[1];
                        Ctx->DeviceData.PciDeviceData.ProgIf = DeviceConfig->Hdr.ClassCode[0];

                        // Set default command register
                        _PciWrite16(&Ctx->DeviceData.PciDeviceData.PciDeviceLocation,
                        PCI_COMMAND, 0x547 
                        );

                        void* _dump;
                        
                        // event desc wont be saved because these are permanent event signals
                        KeSignalEvent(PciDeviceAddEvent, Ctx, &_dump);
                    } else if(HeaderType == 1) {
                        // PCI-to-PCI Bridge

                    }
                }
            }
        }
    }

    return STATUS_SUCCESS;
}
