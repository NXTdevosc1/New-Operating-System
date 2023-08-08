#pragma once
#include <nosdef.h>
#include <pcidef.h>
#define MAX_DEVICE_NAME_LENGTH 0xFF


typedef struct _PNPID {
    PDEVICE Parent;
    union {
        struct {
            UINT8 Segment, Bus, Device, Function;
            UINT32 Rsv;
        } Pci; // Could be normal PCI or a PCI Bridge
        struct {
            UINT64 Rsv;
        } Usb;
        UINT64 Raw;
    } DeviceLocation;
} PNPID;

typedef enum _BUSTYPE {
    BUS_UNKNOWN,
    BUS_ACPI,
    BUS_PCI,
    BUS_USB
} BUSTYPE;

typedef struct _SYSTEM_DEVICE_ADD_CONTEXT {
    UINT BusType;
    union {
        struct {
            UINT8 Object[4];
        } AcpiDeviceData;
        struct {
            PCI_DEVICE_LOCATION PciDeviceLocation;
            // These are stored in memory to avoid repeatable pci accesses
            UINT16 VendorId, DeviceId;
            UINT8 Class, Subclass, ProgIf;
        } PciDeviceData;
        struct {
            HANDLE Controller;
            
        } UsbDeviceData;
    } DeviceData;
} SYSTEM_DEVICE_ADD_CONTEXT;

// Standard device types

// DEVICE CHARACTERISTICS

typedef enum _DEVICE_TYPES {
    DEVICE_UNKNOWN_TYPE = 0,
    DEVICE_DISK,
    DEVICE_MOUSE,
    DEVICE_KEYBOARD,
    DEVICE_CONTROLLER,
    DEVICE_COMPUTER_MANAGEMENT,
    DEVICE_OTHER_USB,
    DEVICE_TIMER,
    VIRTUAL_DEVICE, // Used to provide a service from a driver to the operating system
    DEVICE_OTHER = 0xFF
} DEVICE_TYPES;

typedef struct _DEVICE DEVICE, *PDEVICE;

PDEVICE KRNLAPI KeCreateDevice(
    UINT DeviceType,
    UINT64 DeviceCharacteristics,
    UINT16* DisplayName,
    void* Context
);

BOOLEAN KRNLAPI KeRemoveDevice(PDEVICE Device);

BOOLEAN KRNLAPI KeGetDeviceName(PDEVICE Device, UINT16* DeviceName, UINT8* NameLength);

BOOLEAN KRNLAPI KeEnumerateDevices(
    IN OPT PDEVICE ParentDevice,
    IN PDEVICE* Device,
    IN OPT UINT16* DeviceName,
    IN BOOLEAN AbsoluteName,
    IN UINT64* EnumValue
);

POBJECT KRNLAPI KeGetDeviceObjectHeader(PDEVICE Device);
