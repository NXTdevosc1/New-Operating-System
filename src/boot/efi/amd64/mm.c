#include <loader.h>
NOS_MEMORY_LINKED_LIST MemoryLinkedList = {0};

NOS_MEMORY_DESCRIPTOR* MmSourceEntry(EFI_PHYSICAL_ADDRESS Addr) {
    for(int x = 0;x<0x40;x++) {
        if(!(MemoryLinkedList.Groups[x].Present)) continue;
        for(int y = 0;y<0x40;y++) {
            if(!(MemoryLinkedList.Groups[x].Present & (1 << y))) continue;
            NOS_MEMORY_DESCRIPTOR* Desc = &MemoryLinkedList.Groups[x].MemoryDescriptors[y];
            if(!Desc->Attributes) continue; // Only use descriptors representing allocated memory 
            if(((UINT64)Desc->PhysicalAddress + (Desc->NumPages << 12)) == Addr)
                return Desc;
        }
    }
    return NULL;
}

static inline UINT64 _BitScanForward64(UINT64 x) {
    UINT64 r = 0;
    __asm__ ("rep bsfq\t%1, %0" : "+r"(r) : "r"(x));
    return r;
}

void BlAllocateMemoryDescriptor(EFI_PHYSICAL_ADDRESS Address, UINT64 NumPages, BOOLEAN Allocated) {


    if(!NosInitData.NosMemoryMap) NosInitData.NosMemoryMap = &MemoryLinkedList;
    if(MemoryLinkedList.Full == (UINT64)-1) {
        QemuWriteSerialMessage("ERROR : Memory Linked List is full before kernel takeoff. Halting...");
        while(1); // Error
    }
    NOS_MEMORY_DESCRIPTOR* Desc;
    // Find source entry
    if((Desc = MmSourceEntry(Address)) && Allocated) {
        Desc->NumPages+=NumPages;
        return;
    }

    // Create new entry
    UINT64 Indx;
    UINT64 Indx2;
    UINT64 f = ~MemoryLinkedList.Full, p;
    Indx = _BitScanForward64(f);
    p = ~MemoryLinkedList.Groups[Indx].Present;
    Indx2 = _BitScanForward64(p);
    MemoryLinkedList.Groups[Indx].Present |= (1 << Indx2);
    if(MemoryLinkedList.Groups[Indx].Present == (UINT64)-1) {
        MemoryLinkedList.Full |= (1 << Indx);
    }
    Desc = &MemoryLinkedList.Groups[Indx].MemoryDescriptors[Indx2];
    Desc->Attributes = Allocated;
    Desc->PhysicalAddress = (void*)Address;
    Desc->NumPages = NumPages;
}

void* BlAllocateOnePage() {
    for(int x = 0;x<0x40;x++) {
        if(!(MemoryLinkedList.Groups[x].Present)) continue;
        for(int y = 0;y<0x40;y++) {
            if(!(MemoryLinkedList.Groups[x].Present & (1 << y))) continue;
            NOS_MEMORY_DESCRIPTOR* Desc = &MemoryLinkedList.Groups[x].MemoryDescriptors[y];
            if((Desc->Attributes & MM_DESCRIPTOR_ALLOCATED)) continue;
            void* Addr = Desc->PhysicalAddress;

            Desc->PhysicalAddress = (void*)((char*)Desc->PhysicalAddress + 0x1000);
            Desc->NumPages--;
            if(!Desc->NumPages) {
                MemoryLinkedList.Groups[x].Present &= ~(1 << y);
                MemoryLinkedList.Full &= ~(1 << x);
            }
            BlAllocateMemoryDescriptor((EFI_PHYSICAL_ADDRESS)Addr, 1, 1);
            return Addr;
        }
    }
    QemuWriteSerialMessage("ERROR : Failed to allocate one page. Halting...");
    while(1);
}

UINTN _UsedSystemPages = 0; // In 4KB Pages
UINTN _NumSystemPages = 0; // In 4KB Pages
void BlInitSystemHeap(UINTN NumLargePages) {
    EFI_PHYSICAL_ADDRESS Memory;
    if(EFI_ERROR(gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, (NumLargePages << 9) + 0x200 /*Used for alignment*/, &Memory))) {
		Print(L"InitSystemHeap failed : Failed to allocate memory\n");
        gBS->Exit(gImageHandle, EFI_BUFFER_TOO_SMALL, 0, NULL);
    }
    Memory += 0x200000 - (Memory & 0x1FFFFF);
    NosInitData.NosPhysicalBase = (void*)Memory;
    _NumSystemPages = NumLargePages << 9;
    QemuWriteSerialMessage("System Heap:");
    QemuWriteSerialMessage(ToHexStringUint64(Memory));
}

/*
    Allocates memory inside the system heap
*/
void* BlAllocateSystemHeap(UINTN NumPages) {
    if(_NumSystemPages - _UsedSystemPages < NumPages) {
		Print(L"Failed to allocate memory for the System\n");
        // Unexpected error
        gBS->Exit(gImageHandle, EFI_LOAD_ERROR, 0, NULL);
    }
    return (void*)((char*)NosInitData.NosPhysicalBase + (_UsedSystemPages << 12));
}