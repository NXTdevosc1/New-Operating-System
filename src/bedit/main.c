#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>


#define CMD_INIT "bootinit"
#define CMD_WRITE "set"
#define CMD_READ "get"


typedef struct _NOS_BOOT_DRIVER {
    UINT8 DriverPath[255];
    UINT8 EndChar0;
    UINT8 DriverName[63];
    UINT8 EndChar1;
    UINT8 DriverDescription[255];
    UINT8 EndChar2;
    void* ImageFile; // To be loaded by the kernel
} NOS_BOOT_DRIVER;
typedef struct _NOS_BOOT_HEADER {
    UINT64 Magic; // 0x3501826759F87346
    UINT8 StartupMode; // 0 is normal startup (currently unused)
    UINT8 StartupFlags; // currently unused
    UINT16 Language; // Refers to a lang id taken from the system settings
    UINT32 OsMajorVersion;
    UINT32 OsMinorVersion;
    UINT8 OsName[256];
    UINT32 NumDrivers;
    NOS_BOOT_DRIVER Drivers[];
} NOS_BOOT_HEADER;

NOS_BOOT_HEADER BootHeader = {
    0x3501826759F87346,
    0,
    0,
    0,
    1,
    0,
    "New Operating System",
    0,
    {0}
};
int main(int argc, char** argv) {
    
    if(strcmp(argv[1], CMD_INIT) == 0) {
        HANDLE f = CreateFileW(L"boot.nos", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
        WriteFile(
            f, &BootHeader, sizeof(BootHeader), NULL, NULL
        );
        printf("Boot Init Successfull.\n");
    } else if(strcmp(argv[1], CMD_WRITE) == 0) {

    } else if(strcmp(argv[1], CMD_READ) == 0) {

    } else {
        printf("Unknown command.\n");
    }
    return 0;
}