#include <ddk.h>
#include <acpisystem/acpi.h>
#include <intrin.h>

inline void CmosTimeUpdateCheck() {
    __outbyte(0x70, 0xA);
    while(__inbyte(0x71) & 0x80) Stall(100);
}

inline UINT8 CmosRead(UINT8 Reg) {

    __outbyte(0x70, Reg);
    return __inbyte(0x71);
}

inline void CmosWrite(UINT8 Reg, UINT8 Val) {
    __outbyte(0x70, Reg);
    __outbyte(0x71, Val);
}

void ReadTimeAndDate(TIMESTRUCT* Time) {

    CmosTimeUpdateCheck();

    // Read current time
    UINT32 sec, min, hr, wd, d, m, y, c;

    sec = CmosRead(0);
    min = CmosRead(2);
    hr = CmosRead(4);
    wd = CmosRead(6);
    d = CmosRead(7);
    m = CmosRead(8);
    y = CmosRead(9);

    if(!(CmosRead(0xB) & 4)) {
        sec = (sec & 0x0F) + ((sec / 16) * 10);
        min = (min & 0x0F) + ((min / 16) * 10);
        hr = ( (hr & 0x0F) + (((hr & 0x70) / 16) * 10) ) | (hr & 0x80);
        d = (d & 0x0F) + ((d / 16) * 10);
        wd = (wd & 0x0F) + ((wd / 16) * 10);

        m = (m & 0x0F) + ((m / 16) * 10);
        y = (y & 0x0F) + ((y / 16) * 10);
    }

    // Convert to 12H Time
    if(!(CmosRead(0xB) & 2) && hr & 0x80) {
        hr &= 0x7F;
        hr+=12;
    }

    Time->Second = sec;
    Time->Minute = min;
    Time->Hour = hr;
    Time->WeekDay = wd;
    Time->Day = d;
    Time->Month = m;
    Time->Year = y;
}

IORESULT RtcIo(IOPARAMS) {
    switch(Function) {
        case TIMER_IO_READ_COUNTER:
        {
            return (IORESULT)0;
        }
        case TIMER_IO_SET_TIME_AND_DATE:
        {
            // UNIMPLEMENTED
            return (IORESULT)FALSE;
        }
        case TIMER_IO_GET_TIME_AND_DATE:
        {
            if(NumParameters != 1) {
                KeRaiseException(STATUS_INVALID_PARAMETER);
            }
            ReadTimeAndDate((TIMESTRUCT*)Parameters[0]);
            return (IORESULT)TRUE;
            break;
        }
    }
    KeRaiseException(STATUS_INVALID_PARAMETER);
}

NSTATUS DriverEntry(PDRIVER Driver) {
    UINT32 Ver;
    KDebugPrint("RTC Driver startup.");
    if(!AcpiGetVersion(&Ver)) {
        KDebugPrint("Failed to get acpi version.");
        return STATUS_SUBSYSTEM_NOT_PRESENT;
    }
    KDebugPrint("ACPI Version : %u.%u", Ver >> 16, Ver & 0xFFFF);
    
    PDEVICE RtcDevice = KeCreateDevice(DEVICE_TIMER, 0, L"Real Time Clock (RTC)", NULL);
    
    IO_INTERFACE_DESCRIPTOR Io = {0};
    Io.Flags = IO_CALLBACK_SYNCHRONOUS;
    Io.IoCallback = RtcIo;
    Io.NumFunctions = 3;

    IoSetInterface(RtcDevice, &Io);


    TIMESTRUCT Time;
    ReadTimeAndDate(&Time);
    
    KDebugPrint("%d:%d:%d", Time.Hour, Time.Minute, Time.Second);
    KDebugPrint("Day %d Of the week", Time.WeekDay);
    KDebugPrint("%d/%d/%d", Time.Day, Time.Month, Time.Year);
    
    
    return STATUS_SUCCESS;
}