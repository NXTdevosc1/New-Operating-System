#include <vmsvga.h>
#include <hm.h>
typedef struct _FRAME_BUFFER_DESCRIPTOR
{
    UINT32 HorizontalResolution;
    UINT32 VerticalResolution;
    UINT64 Pitch;
    void *BaseAddress; // for G.O.P
    UINT64 FbSize;
} FRAME_BUFFER_DESCRIPTOR;

typedef struct _NOS_INITDATA
{
    // Nos Boot Header (imported from System/boot.nos)
    void *BootHeader; // 0x00
    // Nos Image Data
    void *NosKernelImageBase;  // 0x08
    void *NosFile;             // 0x10
    UINT64 NosKernelImageSize; // 0x18

    // EFI Frame Buffer
    FRAME_BUFFER_DESCRIPTOR FrameBuffer; // 0x20
} NOS_INITDATA;

__declspec(dllexport) NOS_INITDATA *__fastcall KiGetInitData();

void SVGAPI SvgaEnable(VMSVGA *Svga)
{

    KeMapVirtualMemory(NULL, Svga->FbMem, Svga->FbMem, ConvertToPages(Svga->FbSize), PAGE_WRITE_ACCESS, PAGE_CACHE_WRITE_COMBINE);
    KeMapVirtualMemory(NULL, Svga->FifoMem, Svga->FifoMem, ConvertToPages(Svga->FifoSize), PAGE_WRITE_ACCESS, PAGE_CACHE_WRITE_BACK);

    // Init FIFO

    Svga->FifoMem[SVGA_FIFO_MIN] = SVGA_FIFO_NUM_REGS * 4;
    Svga->FifoMem[SVGA_FIFO_MAX] = Svga->FifoSize;
    Svga->FifoMem[SVGA_FIFO_NEXT_CMD] = Svga->FifoMem[SVGA_FIFO_MIN];
    Svga->FifoMem[SVGA_FIFO_STOP] = Svga->FifoMem[SVGA_FIFO_MIN];
    /*
     * Prep work for 3D version negotiation. See SVGA3D_Init for
     * details, but we have to give the host our 3D protocol version
     * before enabling the FIFO.
     */

    if (SvgaFifoCap(Svga, SVGA_CAP_EXTENDED_FIFO) &&
        SvgaFifoRegValid(Svga, SVGA_FIFO_GUEST_3D_HWVERSION))
    {
        Svga->FifoMem[SVGA_FIFO_GUEST_3D_HWVERSION] = SVGA3D_HWVERSION_CURRENT;
    }

    /*
     * Enable the SVGA device and FIFO.
     */

    SvgaWrite(Svga, SVGA_REG_ENABLE, TRUE);
    SvgaWrite(Svga, SVGA_REG_CONFIG_DONE, TRUE);

    SvgaGetMode(Svga, &Svga->Width, &Svga->Height, &Svga->Pitch, &Svga->Bpp);

    KDebugPrint("Current mode %dx%d (%d BPP) Pitch : %d",
                Svga->Width, Svga->Height, Svga->Bpp, Svga->Pitch);

    SvgaSetMode(Svga, Svga->Width, Svga->Height, 32);

    NOS_INITDATA *Initdata = KiGetInitData();

    KDebugPrint("Previous FB %x New FB %x", Initdata->FrameBuffer.BaseAddress, Svga->FbMem);

    // Initdata->FrameBuffer.BaseAddress = Svga->FbMem;

    // HmTest();
}

void SVGAPI SvgaGetMode(
    VMSVGA *Svga,
    UINT32 *HorizontalResolution,
    UINT32 *VerticalResolution,
    UINT32 *Pitch,
    UINT32 *BitsPerPixel)
{
    *HorizontalResolution = SvgaRead(Svga, SVGA_REG_WIDTH);
    *VerticalResolution = SvgaRead(Svga, SVGA_REG_HEIGHT);
    *Pitch = SvgaRead(Svga, SVGA_REG_BYTES_PER_LINE);
    *BitsPerPixel = SvgaRead(Svga, SVGA_REG_BITS_PER_PIXEL);
}

void SVGAPI SvgaSetMode(
    VMSVGA *Svga,
    UINT32 HorizontalResolution,
    UINT32 VerticalResolution,
    UINT32 BitsPerPixel)
{
    Svga->Width = HorizontalResolution;
    Svga->Height = VerticalResolution;
    Svga->Bpp = BitsPerPixel;

    SvgaWrite(Svga, SVGA_REG_WIDTH, HorizontalResolution);
    SvgaWrite(Svga, SVGA_REG_HEIGHT, VerticalResolution);
    SvgaWrite(Svga, SVGA_REG_BITS_PER_PIXEL, BitsPerPixel);
    Svga->Pitch = SvgaRead(Svga, SVGA_REG_BYTES_PER_LINE);
}