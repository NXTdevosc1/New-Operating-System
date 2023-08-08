#pragma once
#include <nosdef.h>
#include <intmgr.h>
#pragma pack(push, 1)

#define PCI_BAR_IO (1)
#define PCI_BAR_64BIT (1 << 2)
#define PCI_BAR_PREFETCHABLE (1 << 3)
#define PCI_ADDRESS_MASK ((UINT32)~0xf)

///
/// Common header region in PCI Configuration Space
/// Section 6.1, PCI Local Bus Specification, 2.2
///

// PCI_DEVICE_INDEPENDENT_REGION OFFSETS

#define PCI_VENDORID 0
#define PCI_DEVICEID 2
#define PCI_COMMAND 4
#define PCI_STATUS 6
#define PCI_REVISIONID 8
#define PCI_PROGIF 9
#define PCI_SUBCLASS 10
#define PCI_CLASS 11
#define PCI_CACHELINE_SIZE 12
#define PCI_LATENCYTIMER 13
#define PCI_HEADERTYPE 14
#define PCI_BIST 15

typedef struct {
  UINT16    VendorId;
  UINT16    DeviceId;
  UINT16    Command;
  UINT16    Status;
  UINT8     RevisionID;
  UINT8     ClassCode[3];
  UINT8     CacheLineSize;
  UINT8     LatencyTimer;
  UINT8     HeaderType;
  UINT8     BIST;
} PCI_DEVICE_INDEPENDENT_REGION;

///
/// PCI Device header region in PCI Configuration Space
/// Section 6.1, PCI Local Bus Specification, 2.2
///

// OFFSETS
#define PCI_BAR 0x10
#define PCI_CISPTR 0x28
#define PCI_SUBSYSTEM_VENDORID 0x2C
#define PCI_SUBSYSTEMID 0x2E
#define PCI_EXPANSIONROM_BAR 0x30
#define PCI_CAPABILITYPTR 0x34
#define PCI_INTERRUPT_LINE 0x3C
#define PCI_INTERRUPT_PIN 0x3D
#define PCI_MINGRANT 0x3E
#define PCI_MAXLATENCY 0x3F

typedef struct {
  UINT32    Bar[6];
  UINT32    CISPtr;
  UINT16    SubsystemVendorID;
  UINT16    SubsystemID;
  UINT32    ExpansionRomBar;
  UINT8     CapabilityPtr;
  UINT8     Reserved1[3];
  UINT32    Reserved2;
  UINT8     InterruptLine;
  UINT8     InterruptPin;
  UINT8     MinGnt;
  UINT8     MaxLat;
} PCI_DEVICE_HEADER_TYPE_REGION;

typedef struct {
  PCI_DEVICE_INDEPENDENT_REGION    Hdr;
  PCI_DEVICE_HEADER_TYPE_REGION    Device;
} PCI_TYPE00;

///
/// PCI-PCI Bridge header region in PCI Configuration Space
/// Section 3.2, PCI-PCI Bridge Architecture, Version 1.2
///
typedef struct {
  UINT32    Bar[2];
  UINT8     PrimaryBus;
  UINT8     SecondaryBus;
  UINT8     SubordinateBus;
  UINT8     SecondaryLatencyTimer;
  UINT8     IoBase;
  UINT8     IoLimit;
  UINT16    SecondaryStatus;
  UINT16    MemoryBase;
  UINT16    MemoryLimit;
  UINT16    PrefetchableMemoryBase;
  UINT16    PrefetchableMemoryLimit;
  UINT32    PrefetchableBaseUpper32;
  UINT32    PrefetchableLimitUpper32;
  UINT16    IoBaseUpper16;
  UINT16    IoLimitUpper16;
  UINT8     CapabilityPtr;
  UINT8     Reserved[3];
  UINT32    ExpansionRomBAR;
  UINT8     InterruptLine;
  UINT8     InterruptPin;
  UINT16    BridgeControl;
} PCI_BRIDGE_CONTROL_REGISTER;

///
/// PCI-to-PCI Bridge Configuration Space
/// Section 3.2, PCI-PCI Bridge Architecture, Version 1.2
///
typedef struct {
  PCI_DEVICE_INDEPENDENT_REGION    Hdr;
  PCI_BRIDGE_CONTROL_REGISTER      Bridge;
} PCI_TYPE01;

typedef union {
  PCI_TYPE00    Device;
  PCI_TYPE01    Bridge;
} PCI_TYPE_GENERIC;

// PCI CAPABILITY
#define PCI_CAPABILITY_ID 0
#define PCI_CAPABILITY_NEXT 1

typedef enum _PCI_CAPABILITIES {
    PCI_MSI_CAPABILITY = 5
} PCI_CAPABILITIES;

// MSI CAPABILITY
#define MSI_MESSAGE_CONTROL 2
#define MSI_MESSAGE_ADDRESS 4
#define MSI_MESSAGE_DATA 8
#define MSI_MASK 0xC
#define MSI_PENDING 0x10

#define MSI64_MESSAGE_DATA 0xC
#define MSI64_MASK 0x10
#define MSI64_PENDING 0x14

// BIT OFFSETS
typedef struct _MSI_MSGCTL {
    UINT16 MsiEnable : 1;
		UINT16 MultipleMessageCapable : 3;
		UINT16 MultipleMessageEnable : 3;
		UINT16 x64AddressCapable : 1;
		UINT16 PerVectorMaskingCapable : 1;
		UINT16 Reserved : 7;
} MSI_MSGCTL;

#pragma pack(pop)

#ifndef SYSAPI
#define SYSAPI __declspec(dllimport) __fastcall
#endif

typedef union _PCI_DEVICE_LOCATION {
    struct {
      UINT64 Offset : 12; // should be set to 0
      UINT64 Function : 3;
      UINT64 Device : 5;
      UINT64 Bus : 8;
      UINT64 Segment : 16;
    } Fields;
    UINT64 Address; // Mainly used by the driver
} PCI_DEVICE_LOCATION;

typedef struct _PCI_DRIVER_INTERFACE {
    UINT16 NumConfigurations;
    BOOLEAN IsMemoryMapped;
    // Configuration numbers may be arbitrary
    UINT16 (__fastcall* GetConfigurationByIndex)(UINT16 Index);
    PCI_DEVICE_INDEPENDENT_REGION* (__fastcall* GetMemoryMappedConfiguration)(UINT16 Segment, UINT8 Bus, UINT8 Device, UINT8 Function);
    UINT64 (__fastcall* Read64)(PCI_DEVICE_LOCATION* Location, UINT16 Offset);
    UINT32 (__fastcall* Read32)(PCI_DEVICE_LOCATION* Location, UINT16 Offset);
    UINT16 (__fastcall* Read16)(PCI_DEVICE_LOCATION* Location, UINT16 Offset);
    UINT8 (__fastcall* Read8)(PCI_DEVICE_LOCATION* Location, UINT16 Offset);
    void (__fastcall* Write64)(PCI_DEVICE_LOCATION* Location, UINT16 Offset, UINT64 Value);
    void (__fastcall* Write32)(PCI_DEVICE_LOCATION* Location, UINT16 Offset, UINT32 Value);
    void (__fastcall* Write16)(PCI_DEVICE_LOCATION* Location, UINT16 Offset, UINT16 Value);
    void (__fastcall* Write8)(PCI_DEVICE_LOCATION* Location, UINT16 Offset, UINT8 Value);
} PCI_DRIVER_INTERFACE;

BOOLEAN SYSAPI PciQueryInterface(OUT PCI_DRIVER_INTERFACE* DriverInterface);

PVOID SYSAPI PciGetBaseAddress(PCI_DRIVER_INTERFACE* _Pci, PCI_DEVICE_LOCATION* _PciAddress, UINT _BarIndex);
NSTATUS SYSAPI EnableMsiInterrupts(PCI_DRIVER_INTERFACE* Pci, PCI_DEVICE_LOCATION* Location, INTERRUPT_SERVICE_HANDLER Handler, void* Context);
// UINT16 SYSAPI PciGetConfigurationCount();
// BOOLEAN SYSAPI PciMemoryMapped();

// PCI_DEVICE_INDEPENDENT_REGION SYSAPI PciGetMemoryMappedConfiguration(UINT16 Segment, UINT8 Bus, UINT8 Device, UINT8 Function);
// UINT64 SYSAPI PciRead64(PCI_DEVICE_LOCATION* Location, UINT16 Offset);
// UINT32 SYSAPI PciRead32(PCI_DEVICE_LOCATION* Location, UINT16 Offset);
// UINT16 SYSAPI PciRead16(PCI_DEVICE_LOCATION* Location, UINT16 Offset);
// UINT8 SYSAPI PciRead8(PCI_DEVICE_LOCATION* Location, UINT16 Offset);

// void SYSAPI PciWrite64(PCI_DEVICE_LOCATION* Location, UINT16 Offset, UINT64 Value);
// void SYSAPI PciWrite32(PCI_DEVICE_LOCATION* Location, UINT16 Offset, UINT32 Value);
// void SYSAPI PciWrite16(PCI_DEVICE_LOCATION* Location, UINT16 Offset, UINT16 Value);
// void SYSAPI PciWrite8(PCI_DEVICE_LOCATION* Location, UINT16 Offset, UINT8 Value);
