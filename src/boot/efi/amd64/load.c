#include <loader.h>


int BlGetPeHeaderOffset(void* hdr) {
	if (!isMemEqual(hdr, "MZ", 2)) return 0;
	return *(int*)((char*)hdr + 0x3c);
}

BOOLEAN BlCheckImageHeader(PE_IMAGE_HDR* hdr) {
	if (
		hdr->ThirdHeader.Subsystem != 1 ||
		hdr->MachineType != 0x8664 ||
		hdr->SizeofOptionnalHeader < sizeof(PE_OPTIONAL_HEADER) ||
		!hdr->OptionnalHeader.EntryPointAddr ||
		!isMemEqual(hdr->Signature, "PE\0\0", 4)
		) {
		return FALSE;
	}
	return TRUE;
}


/*
 * BlLoadImage : Loads an executable image in the PE format located in the Buffer
 * Buffer : Executable image file buffer
 * HdrStart : The function returns the starting point of the PE Header
 * VirtualAddressSpace : The function returns the physical address of the created VAS
 * VasSize : The function returns the size of the VAS
 * BaseAddress : The actual base address located on the system space (>128 TB) where the VAS will be located
*/
BOOLEAN BlLoadImage(void* Buffer, PE_IMAGE_HDR** HdrStart, void** VirtualAddressSpace, UINT64* VasSize, void* BaseAddress) {
	PE_IMAGE_HDR* Header;
	UINT64 VasBufferSize = 0;
	void* VasBuffer;
	EFI_PHYSICAL_ADDRESS ImageBase;
	// Header Check
	{
		int Off = BlGetPeHeaderOffset(Buffer);
		if(!Off) return FALSE;
		Header = (PE_IMAGE_HDR*)((char*)Buffer + Off);
		if(!BlCheckImageHeader(Header)) return FALSE;
		*HdrStart = Header;
	}
	// Get virtual address buffer size
	PE_SECTION_TABLE* Sections = (PE_SECTION_TABLE*)((char*)&Header->OptionnalHeader + Header->SizeofOptionnalHeader);
	for(int i = 0;i<Header->NumSections;i++) {
		if(Sections[i].Characteristics & ((PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA))) {
			if(Sections[i].VirtualSize < Sections[i].SizeofRawData) Sections[i].VirtualSize = Sections[i].SizeofRawData;
			if((Sections[i].VirtualAddress + Sections[i].VirtualSize) > VasBufferSize) VasBufferSize = Sections[i].VirtualAddress + Sections[i].VirtualSize;
		}
	}
	// Allocate virtual address buffer
	VasBufferSize += 0x10000;

	// Allocate 2MB Aligned pages
	ImageBase = (EFI_PHYSICAL_ADDRESS)BlAllocateSystemHeap(VasBufferSize >> 12);
	if(Header->ThirdHeader.DllCharacteristics & 0x40) {
		// TODO : Relocation to BaseAddress
	}

	VasBuffer = (void*)ImageBase;
	// Copy section data to the VAS Buffer
	for(UINT16 i = 0;i<Header->NumSections;i++) {
		PE_SECTION_TABLE* Section = Sections + i;
		QemuWriteSerialMessage("SECTION : (Name, Vaddr, RAddr)");
		QemuWriteSerialMessage(Section->name);
		QemuWriteSerialMessage(ToHexStringUint64(Section->VirtualAddress));
		QemuWriteSerialMessage(ToHexStringUint64(Section->VirtualSize));
		QemuWriteSerialMessage(ToHexStringUint64(Section->PtrToRawData));

		if (Section->Characteristics & (PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA)) {
			CopyAlignedMemory((void*)((char*)VasBuffer + Section->VirtualAddress), (UINT64*)((char*)Buffer + Section->PtrToRawData), Section->SizeofRawData);
			if (Section->VirtualSize > Section->SizeofRawData) {
				UINT64 UninitializedDataSize = Section->VirtualSize - Section->SizeofRawData;
				ZeroAlignedMemory((void*)((char*)VasBuffer + Section->VirtualAddress + Section->SizeofRawData), UninitializedDataSize);
			}
		}
	}
	*VirtualAddressSpace = VasBuffer;
	*VasSize = VasBufferSize;
	return TRUE;
}