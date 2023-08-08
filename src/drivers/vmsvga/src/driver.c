#include <vmsvga.h>

PCI_DRIVER_INTERFACE Pci;

NSTATUS NOSENTRY DriverEntry(IN PDRIVER Driver) {
    KDebugPrint("VMSVGA Driver Startup DriverId=%u", Driver->DriverId);
    if(!PciQueryInterface(&Pci)) {
        KDebugPrint("Query PCI Interface failed.");
        return STATUS_UNSUPPORTED;
    }
    return KeRegisterEventHandler(SYSTEM_EVENT_DEVICE_ADD, 0, DevAdd) ? STATUS_SUCCESS : STATUS_UNSUPPORTED;
}

NSTATUS DevAdd(SYSTEM_DEVICE_ADD_CONTEXT* Context) {

    if(Context->BusType == BUS_PCI &&
    Context->DeviceData.PciDeviceData.VendorId == PCI_VENDOR_ID_VMWARE &&
    Context->DeviceData.PciDeviceData.DeviceId == PCI_DEVICE_ID_VMWARE_SVGA2
    ) {
        KDebugPrint("Found VMWARE SVGA Device.");
        VMSVGA* Svga = AllocateNullPool(sizeof(VMSVGA));
        memcpy(&Svga->PciLocation, &Context->DeviceData.PciDeviceData.PciDeviceLocation, sizeof(PCI_DEVICE_LOCATION));
        PCI_DEVICE_LOCATION* loc = &Svga->PciLocation;
        Svga->IoBase = UINT16VOID PciGetBaseAddress(&Pci, loc, 0);
        Svga->FbMem = PciGetBaseAddress(&Pci, loc, 1);
        Svga->FifoMem = PciGetBaseAddress(&Pci, loc, 2);

        Svga->VersionId = SVGA_ID_2;
        do {
            SvgaWrite(Svga, SVGA_REG_ID, Svga->VersionId);
            if(SvgaRead(Svga, SVGA_REG_ID) == Svga->VersionId) {
                break;
            } else {
                Svga->VersionId--;
            }
        } while(Svga->VersionId >= SVGA_ID_0);

        if(Svga->VersionId < SVGA_ID_0) {
            MmFreePool(Svga);
            KDebugPrint("Error negotiating VMSVGA Version");
            return STATUS_UNSUPPORTED;
        }

        KDebugPrint("VMSVGA ver %x : IO %x FB %x FIFO %x", Svga->VersionId, Svga->IoBase, Svga->FbMem, Svga->FifoMem);
    
        Svga->VramSize = SvgaRead(Svga, SVGA_REG_VRAM_SIZE);
        Svga->FbSize = SvgaRead(Svga, SVGA_REG_FB_SIZE);
        Svga->FifoSize = SvgaRead(Svga, SVGA_REG_MEM_SIZE);

        KDebugPrint("VMSVGA Size VRAM %x FB %x FIFO %x", Svga->VramSize, Svga->FbSize, Svga->FifoSize);
    
        if(Svga->VersionId >= SVGA_ID_1) {
            Svga->Capabilities = SvgaRead(Svga, SVGA_REG_CAPABILITIES);
        }

        SvgaEnable(Svga);

    }

    return STATUS_SUCCESS;
}