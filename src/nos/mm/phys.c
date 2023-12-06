#include <nos/nos.h>
#include <mm.h>
#include <nos/mm/physmem.h>

PVOID KRNLAPI MmRequestContiguousPages(
    UINT PageSize,
    UINT64 Length)
{

    UINT64 cl = Length;
    PVOID ret;
    for (int i = PageSize; i < 4; i++, cl = ((cl & 0x1FF) ? ((cl >> 9) + 1) : (cl >> 9)))
    {
        if (cl > 0x1FF)
            continue;

        KDebugPrint("i %d CL %x", i, cl);
        if ((ret = VmmAllocate(_NosPhysicalMemoryImage, i, cl)))
        {
            // Free extra memory
            char *freemem = (char *)ret + (Length << (12 + (PageSize * 9)));
            if (PageSize != i)
            {
                KDebugPrint("TOTAL MEM %x-%x", ret, (char *)ret + (cl << (12 + i * 9)));
            }
            for (int c = i - 1; c >= (int)PageSize; c--)
            {
                KDebugPrint("f");
                UINT64 np = 0x1FF - (Length >> (c * 9));
                KPAGEHEADER *en = ResolvePageHeader(freemem);
                en->Header.Address = (UINT64)freemem >> 12;

                KDebugPrint("NP %x FRMEM %x LEN %x C %x EN %x", np, freemem, Length, c, en);
                // VmmInsert(VmmPageLevel(_NosPhysicalMemoryImage, c), en, np);
                freemem += np << (12 + c * 9);
                Length -= np << (c * 9);
            }
            return ret;
        }
        else
        {
            KDebugPrint("RETURNED NULL VMM ALLOC");
        }
        // else
        // {
        //     cl += 0x200;
        //     cl &= ~0x1FF;
        // }
    }
    return NULL;
}

void KRNLAPI MmFreePages(
    PVOID Address)
{
}
