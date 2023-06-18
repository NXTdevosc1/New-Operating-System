#include <nos/nos.h>
#include <nos/console/console.h>

UINT NumberOfLines = 0;
UINT UsedLines = 0;
#define MaxCharCount 120

UINT ConSize =0;


struct {
    char Sender[MaxCharCount];
    char Message[MaxCharCount];
    UINT Color;
} *ConBuffer;

extern void DrawRect(UINT x, UINT y, UINT Width, UINT Height, UINT Color);

void ConClear() {
    if(!ConBuffer) {
        NumberOfLines = NosInitData->FrameBuffer.VerticalResolution / 35;
        ConSize = sizeof(*ConBuffer) * NumberOfLines;
        ConBuffer = MmAllocateMemory(NULL, ConvertToPages(ConSize), PAGE_WRITE_ACCESS, 0);
    }
    ZeroMemory(ConBuffer, ConSize);
    ZeroMemory(NosInitData->FrameBuffer.BaseAddress, NumberOfLines * 35 * NosInitData->FrameBuffer.Pitch);
}

void ConNewLine() {
    ConWrite("", "", 0);
}

void ConPush(char* Sender, char* Message, UINT Color) {
    if(UsedLines == NumberOfLines) {
        for(int i = 1;i<NumberOfLines;i++) {
            strcpy(ConBuffer[i - 1].Sender, ConBuffer[i].Sender);
            strcpy(ConBuffer[i - 1].Message, ConBuffer[i].Message);
            ConBuffer[i - 1].Color = ConBuffer[i].Color;
        }
        strcpy(ConBuffer[NumberOfLines - 1].Sender, Sender);
        strcpy(ConBuffer[NumberOfLines - 1].Message, Message);
        ConBuffer[NumberOfLines - 1].Color = Color;

    } else {
        strcpy(ConBuffer[UsedLines].Sender, Sender);
        strcpy(ConBuffer[UsedLines].Message, Message);
        ConBuffer[UsedLines].Color = Color;
        UsedLines++;
    }
}

void ConRedraw() {
    UINT YCursor = 20;
    UINT XCursor = 20;
    // _Memset128A_32(NosInitData->FrameBuffer.BaseAddress, 0, (UsedLines * 35) * NosInitData->FrameBuffer.Pitch);
    for(int i = 0;i<UsedLines;i++) {
        char* Sender = ConBuffer[i].Sender, *Message = ConBuffer[i].Message;
        UINT Color = ConBuffer[i].Color;
            while(*Sender) {
                DrawRect(XCursor, YCursor, 8, 16, 0xFF);
                XCursor+=8;
                Sender++;
            }
            XCursor += 20;
            while(*Message) {
                DrawRect(XCursor, YCursor, 8, 16, Color);
                XCursor+= 8;
                Message++;
            }
            YCursor+=20;
            XCursor = 20;
    }
}

void ConWrite(char* Sender, char* Message, UINT32 Color) {
    if(!Color) Color = 0xFFFFFF;
    ConPush(Sender, Message, Color);
    ConRedraw();

}