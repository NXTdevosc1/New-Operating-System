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
        BlSerialWrite("ERROR : Memory Linked List is full before kernel takeoff. Halting...");
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
    BlSerialWrite("ERROR : Failed to allocate one page. Halting...");
    while(1);
}

UINTN _UsedSystemPages = 0; // In 4KB Pages
UINTN _NumSystemPages = 0; // In 4KB Pages
UINTN _SysHeapLinkCount = 0; // Last index = count - 1
UINTN _SysHeapLinkOffset = 0;
#define MAX_PAGE_LINKS 0xFFF
struct {
    EFI_PHYSICAL_ADDRESS Addr;
    UINTN NumLargePages;
} SystemHeapLinks[MAX_PAGE_LINKS + 1] = {0}; // (+1) NULL Entry


// Expands the System Heap
void BlInitSystemHeap(UINTN NumLargePages) {
    EFI_PHYSICAL_ADDRESS Memory;
    if(EFI_ERROR(gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, (NumLargePages << 9), &Memory))) {
		Print(L"InitSystemHeap failed : Failed to allocate memory\n");
        gBS->Exit(gImageHandle, EFI_BUFFER_TOO_SMALL, 0, NULL);
    }
    // Memory += 0x200000 - (Memory & 0x1FFFFF);
    SystemHeapLinks[_SysHeapLinkCount].Addr = Memory;
    SystemHeapLinks[_SysHeapLinkCount].NumLargePages = NumLargePages;
    _SysHeapLinkCount++;
    // Reset heap offset
    _SysHeapLinkOffset = 0;
    _NumSystemPages += (NumLargePages << 9);
    BlSerialWrite("System Heap:");
    BlSerialWrite(BlToHexStringUint64(Memory));
}

/*
    Allocates memory inside the system heap
    returns : Physical Start of the Heap

    if RemaningPages < NumPages an extra heap is allocated and the last heap is discarded,
    the new extra heap will be used
*/
char* __SysBase = (char*)0xffff800000000000;
void* BlAllocateSystemHeap(UINTN NumPages, void** VirtualAddress) {
    if(_NumSystemPages - _UsedSystemPages < NumPages) {
		BlInitSystemHeap(Convert2MBPages(NumPages));
    }
    UINTN HeapLinkIndex = _SysHeapLinkCount - 1;
    
    *VirtualAddress = (void*)(__SysBase + (_UsedSystemPages << 12));
    void* PhysicalAddress = (void*)((char*)SystemHeapLinks[HeapLinkIndex].Addr + _SysHeapLinkOffset);
    _UsedSystemPages+=NumPages;
    _SysHeapLinkOffset+=(NumPages << 12);
    Print(L"Allocating System Pages : %.16X\n", PhysicalAddress);
    return PhysicalAddress;
}

void BlMapSystemSpace() {
    char* Offset = __SysBase;
    for(int i = 0;i<_SysHeapLinkCount;i++) {
        BlSerialWrite("Mapping System Pages :");
        BlSerialWrite(BlToHexStringUint64((UINT64)SystemHeapLinks[i].Addr));
        BlSerialWrite(BlToHexStringUint64((UINT64)Offset));
        BlSerialWrite(BlToHexStringUint64((UINT64)SystemHeapLinks[i].NumLargePages));

        BlMapMemory(Offset, (void*)SystemHeapLinks[i].Addr, SystemHeapLinks[i].NumLargePages, PM_LARGE_PAGES | PM_WRITEACCESS);
        Offset += (SystemHeapLinks[i].NumLargePages << 21);
    }
}