#pragma once
#include <ddk.h>
#include <ahcistruct.h>
#include <intrin.h>
#include <kfs.h>



#define ATA_IDENTIFY_DEVICE_DMA 0xEE
#define ATA_IDENTIFY_DEVICE 0xEC

#define AHCI_READWRITE_MAX_NUMBYTES ((DWORD)0xFFFF << 16)

#define PORTxCMDxSTART 1
#define PORTxCMDxSUD (1 << 1)
#define PORTxCMDxPOD (1 << 2)
#define PORTxCMDxCLO (1 << 3)
#define PORTxCMDxFRE (1 << 4)
#define PORTxCMDxCCS (1 << 8)
#define PORTxCMDxMPSS (1 << 13)
#define PORTxCMDxFRR (1 << 14)
#define PORTxCMDxCR (1 << 15)
#define PORTxCMDxCPS (1 << 16)
#define PORTxCMDxPMA (1 << 17)
#define PORTxCMDxHPCP (1 << 18)
#define PORTxCMDxMPSAP (1 << 19)
#define PORTxCMDxCPD (1 << 20)
#define PORTxCMDxESP (1 << 21)
#define PORTxCMDxFISBSCP (1 << 22)
#define PORTxCMDxAPTSTE (1 << 23)
#define PORTxCMDxATAPI (1 << 24)
#define PORTxCMDxALPME (1 << 25)
#define PORTxCMDxASP (1 << 26)
#define PORTxCMDxICC 27


NSTATUS PciAddEvent(SYSTEM_DEVICE_ADD_CONTEXT* Context);

#define AHCI_PORTS_OFFSET 0x100
#define AHCI_MAX_PORTS 0x20
#define AHCI_CONFIGURATION_PAGES 2
 
typedef struct _AHCI AHCI, *PAHCI;
typedef struct _AHCIPORT {
    PAHCI Ahci;
    UINT8 PortIndex;
    BOOLEAN FirstD2H;
    BOOLEAN Atapi;
    HBA_PORT* HbaPort;
    AHCI_COMMAND_LIST_ENTRY* CommandList;
    AHCI_COMMAND_TABLE* CommandTable;
    AHCI_RECEIVED_FIS* ReceivedFis;

    AHCI_COMMAND_LIST_ENTRY* _PhysCommandList;
    AHCI_COMMAND_TABLE* _PhysCommandTable;
    AHCI_RECEIVED_FIS* _PhysReceivedFis;

    ATA_IDENTIFY_DEVICE_DATA* AtaDeviceIdentify;
    DRIVE_IDENTIFICATION_DATA OsDriveIdentify;
    // Commands
    volatile UINT32 AllocatedCmd;
    volatile UINT32 PendingCmd;
    volatile struct {
        BOOLEAN Async;
        PETHREAD Thread; // Threads that are suspended waiting for ahci command to finish
        union {
            NSTATUS* ReturnCode;
            UINT64* IncrementOnDone;
        } ReturnSet;
    } Commands[32];
    PDRIVE Drive;
} AHCIPORT, *PAHCIPORT;



typedef struct _AHCI {
    PDEVICE Device;
    HBA_REGISTERS* Hba;
    HBA_PORT* HbaPorts;
    BOOLEAN LongAddresses;
    UINT8 MaxSlotNumber;
    UINT8 NumPorts;
    AHCIPORT Ports[32];


} AHCI, *PAHCI;

PCI_DRIVER_INTERFACE Pci;

// AHCI Utilities

NSTATUS AhciInitDevice(PCI_DEVICE_LOCATION* ploc);
PVOID AhciAllocateMemory(PAHCIPORT Port, UINT64 SizeInBytes);
void AhciFreeMemory(PAHCIPORT Port, void* Mem, UINT64 SizeInBytes);

void AhciAbort(PAHCI Ahci, NSTATUS ExitCode);
void AhciReset(PAHCI Ahci);


NSTATUS __cdecl AhciInterruptHandler(INTERRUPT_HANDLER_DATA* Interrupt);

#define AhciPhysicalAddress(x) ((UINT64)KeConvertPointer(NULL, (PVOID)x))

void AhciInitPort(PAHCIPORT Port);
void AhciDisablePort(PAHCIPORT Port);

// Parses and reads the model number into Port->OsDriveIdentify.Name
void AhciReadModelNumber(PAHCIPORT Port);

UINT32 AhciAllocateSlot(PAHCIPORT Port);
NSTATUS AhciIssueCommandSync(PAHCIPORT Port, UINT Cmd);


void AhciInitSataDevice(PAHCIPORT Port);

void AhciIssueCommandAsync(PAHCIPORT Port, UINT Cmd, UINT64* IncrementOnDone);

NSTATUS __fastcall AhciSataRead(PAHCIPORT Port, UINT64 Lba, UINT64 NumBytes, char* Buffer);
NSTATUS __fastcall AhciSataWrite(PAHCIPORT Port, UINT64 Lba, UINT64 NumBytes, char* Buffer);