#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>


#define CMD_INIT "bootinit"
#define CMD_WRITE "set"
#define CMD_READ "get"
#define CMD_ADD_DRIVER "drvadd"
#define CMD_REMOVE_DRIVER "drvremove"
typedef struct _NOS_BOOT_DRIVER {
    UINT32 DriverType;
    UINT32 Flags;
    UINT8 DriverPath[255];
    UINT16 EndChar0;
    void* ImageBuffer; // To be loaded by the kernel
    UINT64 ImageSize;
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

#define command(cmd) (strcmp(argv[1], cmd) == 0)

#define WriteConfig(_file, _addr, _size) {SetFilePointer(_file, 0, NULL, FILE_BEGIN); if(!WriteFile((HANDLE)(_file), (void*)(_addr), (_size), NULL, NULL)) {printf("failed to write file.\n");exit(-1);}}

void OpenBootConfiguration(HANDLE* File, NOS_BOOT_HEADER** buffer) {
    *File = CreateFileW(L"./boot.nos", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if(*File == INVALID_HANDLE_VALUE) {
        printf("Failed to open boot.nos\n");
        exit(-1);
    }
    UINT32 fsize = GetFileSize(*File, NULL) + 0x1000;
    *buffer = malloc(fsize);
    ReadFile(*File, *buffer, fsize, NULL, NULL);
}

char* DriverTypeStrings[] = {
    "Device Driver",
    "File System Driver",
    "Subsystem Driver"
};


int main(int argc, char** argv) {
    
    if(command(CMD_INIT)) {
        HANDLE f = CreateFileW(L"boot.nos", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
        WriteFile(
            f, &BootHeader, sizeof(BootHeader), NULL, NULL
        );
        printf("Boot Init Successfull.\n");
    } else if(command(CMD_ADD_DRIVER)) {
        // args : type/flags/path
        HANDLE boot = NULL;
        NOS_BOOT_HEADER* Header = NULL;
        OpenBootConfiguration(&boot, &Header);

        // search for driver
        UINT32 type = strtoul(argv[2], NULL, 0x10);
        UINT32 flags = strtoul(argv[3], NULL, 0x10);
        char* path = argv[4];
        for(UINT32 i = 0;i<Header->NumDrivers;i++) {
            if(strcmp(path, Header->Drivers[i].DriverPath) == 0) {
                printf("Driver already exists.\n");
                exit(-1);
            }
        }
        printf("Adding driver path : %s\n", path);
        NOS_BOOT_DRIVER* BootDriver = Header->Drivers + Header->NumDrivers;
        ZeroMemory(BootDriver, sizeof(NOS_BOOT_DRIVER));
        Header->NumDrivers++;
        BootDriver->DriverType = type;
        BootDriver->Flags = flags;
        memcpy(BootDriver->DriverPath, path, strlen(path));
        WriteConfig(boot, Header, sizeof(NOS_BOOT_HEADER) + sizeof(NOS_BOOT_DRIVER) * Header->NumDrivers);
        printf("Driver added successfully\n");
        CloseHandle(boot);
    } else if(command(CMD_WRITE)) {

    } else if(command(CMD_READ)) {
        if(argc == 2) {
            // read boot configuration basic information
            HANDLE boot;
            NOS_BOOT_HEADER* Header;
            OpenBootConfiguration(&boot, &Header);
    ReadFile(boot, Header, 300, NULL, NULL);

            printf("Operating System : %s\nVersion: %d.%d\nStartup Mode : %d\nStartup Flags : %.8x\nLanguage:%d\nNumber of drivers : %d\n\nDrivers:\n",
            Header->OsName, Header->OsMajorVersion, Header->OsMinorVersion,
            Header->StartupMode, Header->StartupFlags, Header->Language, Header->NumDrivers
            );
            for(UINT i = 0;i<Header->NumDrivers;i++) {
                printf("- Driver Path : %s\n- Driver Type : %s\n- Flags : %.8x\n\n", Header->Drivers[i].DriverPath, DriverTypeStrings[Header->Drivers[i].DriverType], Header->Drivers[i].Flags);
            }

            printf("Nos Boot Configuration Editor.\n");
        }
    } else {
        printf("Unknown command.\n");
    }
    return 0;
}