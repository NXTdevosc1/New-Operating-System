#include <nos/nos.h>
// Data compression algorithm

typedef struct _ENCODED_ENTRY {
    UINT8 Base;
    UINT8 Size; // size of buffer in bytes
    UINT8 bpe; // Bits per entry
    UINT8 Buffer;
} ENCODED_ENTRY;


void ExEncodeBuffer(UINT8* Buffer, UINT64 SizeInQwords, UINT64* ResultBuffer) {

}