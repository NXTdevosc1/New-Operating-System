#include "sddkinternal.h"
#include <pcidef.h>
#include <intrin.h>

BOOLEAN SYSAPI PciQueryInterface(OUT PCI_DRIVER_INTERFACE* DriverInterface) {
    HANDLE PciHandle = INVALID_HANDLE;
    // Search for the PCI Device
    UINT64 _ev = 0;
    PDEVICE Device;
    if(KeEnumerateDevices(NULL, &Device, L"Peripheral Component Interconnect", FALSE, &_ev)) {
        if(NERROR(ObOpenHandle(KeGetDeviceObjectHeader(Device), NULL, 0, &PciHandle)) &&
        PciHandle == INVALID_HANDLE // Probably the handle is already open
        ) {
            KDebugPrint("Failed to open PCI Handle");
            return FALSE;
        }
        BOOLEAN Ret = (BOOLEAN)IoProtocol(PciHandle, 0, 1, &DriverInterface);
        ObCloseHandle(NULL, PciHandle);
        return Ret;
    }
    KDebugPrint("No PCI was found");
    return FALSE;
}

NSTATUS SYSAPI EnableMsiInterrupts(PCI_DRIVER_INTERFACE* Pci, PCI_DEVICE_LOCATION* Location, INTERRUPT_SERVICE_HANDLER Handler, void* Context) {
    if(!(Pci->Read8(Location, PCI_STATUS) & (1 << 4))) return STATUS_UNSUPPORTED; // Capabilites are not supported
    
    // Enable PCI Interrupts
    // Remove Interrupt Disable (0x400)
    Pci->Write16(Location, PCI_COMMAND, (Pci->Read16(Location, PCI_COMMAND) & ~0x400));

    KDebugPrint("Enabling MSI Interrupts Device %d Bus %d Function %d", Location->Fields.Device, Location->Fields.Bus, Location->Fields.Bus);
    UINT8 Cptr = Pci->Read8(Location, PCI_CAPABILITYPTR) & ~3;
    while(Cptr) {
        UINT8 CapId = Pci->Read8(Location, Cptr + PCI_CAPABILITY_ID);
        KDebugPrint("cptr %x capid %x", Cptr, CapId);
        if(CapId == PCI_MSI_CAPABILITY) {
            KDebugPrint("Found MSI Capability");
            // Allocate an interrupt
            UINT32 Iv = -1;
            UINT64 ProcessorId;
            NSTATUS s;
            if(NERROR((s = KeInstallInterruptHandler(&Iv, &ProcessorId, 0, Handler, Context)))) {
                return s;
            }
            // PROCESSOR* cpu = KeGetProcessorById(ProcessorId);
            // PROCESSOR_IDENTIFICATION_DATA Id;
            // KeProcessorReadIdentificationData(cpu, &Id);
            // KDebugPrint("MSI APIC ID %x ACPI ID %x", ProcessorId, Id.AcpiId);
            // ProcessorId = Id.AcpiId;
            UINT64 Address = (__readmsr(0x1B)/*APIC Base MSR*/ & ((UINT64)~0xFFF)) | (ProcessorId << 12);
            if((Pci->Read16(Location, Cptr + MSI_MESSAGE_CONTROL) & 0x80)) {
                // Use MSI64
                Pci->Write64(Location, Cptr + MSI_MESSAGE_ADDRESS, Address);
                Pci->Write32(Location, Cptr + MSI64_MESSAGE_DATA, Iv | (1 << 14) /*Assert*/);
                Pci->Write32(Location, Cptr + MSI64_MASK, 0);
            } else {
                // Use MSI32
                Pci->Write32(Location, Cptr + MSI_MESSAGE_ADDRESS, Address);
                Pci->Write32(Location, Cptr + MSI_MESSAGE_DATA, Iv | (1 << 14) /*Assert*/);
                Pci->Write32(Location, Cptr + MSI_MASK, 0);

            }
            // Enable MSI
            Pci->Write16(Location, Cptr + MSI_MESSAGE_CONTROL, (Pci->Read16(Location, Cptr + MSI_MESSAGE_CONTROL)) | 1);
        
            return STATUS_SUCCESS;
        }
        Cptr = Pci->Read8(Location, Cptr + PCI_CAPABILITY_NEXT) & ~3;
    }
    return STATUS_UNSUPPORTED;
}


PVOID SYSAPI PciGetBaseAddress(PCI_DRIVER_INTERFACE* _Pci, PCI_DEVICE_LOCATION* _PciAddress, UINT _BarIndex){
    UINT64 Bar = _Pci->Read32(_PciAddress, PCI_BAR + (_BarIndex * 4));
    if((Bar & PCI_BAR_64BIT)) {
        Bar |= ((UINT64)_Pci->Read32(_PciAddress, PCI_BAR + 4 + (_BarIndex * 4))) << 32;
    }
    return (void*)(Bar & ~0xF);
}