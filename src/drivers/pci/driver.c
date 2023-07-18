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

UINT MaxPciSegment = 0;

HANDLE PciDeviceAddEvent;

PDEVICE PciDeviceObject;

#define PCI_IO_GET_CONFIGURATION 0

IORESULT PciIoCallback(IOPARAMS) {
    if(Function == PCI_IO_GET_CONFIGURATION) {
        if(NumParameters != 4) KeRaiseException(STATUS_INVALID_PARAMETER);
        return (IORESULT)PciGetConfiguration(UINT16VOID Parameters[0], UINT8VOID Parameters[1], UINT8VOID Parameters[2], UINT8VOID Parameters[3]);
    }
    return STATUS_SUCCESS;
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
