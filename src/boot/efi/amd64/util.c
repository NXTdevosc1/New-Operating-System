#include <loader.h>

/*
 * BlNullGuid
 * Checks if the Guid is NULL ({0, 0, 0, 0})
*/

BOOLEAN BlNullGuid(EFI_GUID Guid) {
	UINT64* g = (UINT64*)&Guid;
	if(g[0] == 0 && g[1] == 0) return TRUE;
	
	return FALSE;
}

/*
 * isMemEqual
 * Checks if the memory at "a" contains the same value as the memory at "b"
*/

BOOLEAN BlisMemEqual(void* a, void* b, UINT64 Count) {
	for(UINT64 i = 0;i<Count;i++) {
		if(((char*)a)[i] != ((char*)b)[i]) return FALSE;
	}
	return TRUE;
}


void BlCopyAlignedMemory(void* _dest, void* _src, UINT64 NumBytes) {
	// NumBytes >>= 3;
	volatile UINT8* d = _dest, *s = _src;
    while(NumBytes) {
        *d = *s;
        d++;
        s++;
        NumBytes--;
    }
}
void BlZeroAlignedMemory(void* _dest, UINT64 NumBytes) {
	// NumBytes >>= 3;
	volatile UINT8* d = _dest;
    while(NumBytes) {
        *d = 0;
        d++;
        NumBytes--;
    }
}

void OutPortB(unsigned short port, unsigned char val) {
	asm volatile ("outb %0, %1"::"a"(val),"Nd"(port));
}

void BlSerialWrite(const char* Message) {
    while(*Message) {
        OutPortB(0x3F8, *Message);
        Message++;
    }
    OutPortB(0x3F8, '\n');
}

char uintTo_StringOutput[128] = { 0 };
const char* BlToStringUint64(UINT64 value){
    UINT8 size = 0;
    UINT64 sizeTest = value;
    while(sizeTest / 10 > 0)
    {
        sizeTest /=10;
        size++;
    }
    UINT8 index = 0;
    while(value / 10 > 0)
    {
        UINT8 remainder = value % 10;
        value /= 10;
        uintTo_StringOutput[size - index] = remainder + '0';
        index++;
    }
    UINT8 remainder = value % 10;
    uintTo_StringOutput[size - index] = remainder + '0';
    uintTo_StringOutput[size + 1] = 0;
    return uintTo_StringOutput;
}

char hexTo_StringOutput[0x100] = { 0 };

const char* BlToHexStringUint64(UINT64 value)
{
    UINT8 size = 0;
    unsigned long long ValTmp = value;
    do {
        size++;
    } while((ValTmp >>= 4));
    for (UINT8 i = 0; i < size; i++)
    {
        unsigned char c = value & 0xF;
        if(c < 0xA){
            hexTo_StringOutput[size - (i + 1)] = '0' + c;
        }else{
            hexTo_StringOutput[size - (i + 1)] = 'A' + (c - 0xA);
        }
        value >>= 4;
    }
    hexTo_StringOutput[size] = 0;
    return hexTo_StringOutput;
}

int BlStrlen(char* str) {
    int len = 0;
    while(*str) {
        str++;
        len++;
    }
    return len;
}