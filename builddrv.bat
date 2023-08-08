@echo off

if not defined LIB ( 
call vcvars64.bat
)

set DRVLIBS="..\..\..\lib"

echo Building acpi.sys
cd src/drivers/acpi
call ./compile.bat

echo Building pci.sys
cd ../pci
call ./compile

echo Building rtc.sys

cd ../rtc
call ./compile


echo Building eodx.sys
cd ../eodx
call ./compile

echo Building OS Window Management Subsystem
cd ../oswm
call ./compile

echo Building ahci.sys
cd ../ahci
call ./compile

echo Building ehci.sys
cd ../ehci
call ./compile

echo Building xhci.sys
cd ../xhci
call ./compile

echo Building USB Flash Drive Driver
cd ../usbflash
call ./compile

echo Building USB Mouse Driver
cd ../usbmouse
call ./compile

echo Building USB Keyboard Driver
cd ../usbkeyboard
call ./compile

echo Building FAT32 Driver
cd ../fat32
call ./compile

echo Building VMSVGA Driver
cd ../vmsvga
call ./compile

cd ../../..




echo building boot config
bedit bootinit

@REM System Drivers
bedit drvadd 2 3 "NewOS\System\acpi.sys"
bedit drvadd 2 3 "NewOS\System\pci.sys"
bedit drvadd 0 5 "NewOS\System\rtc.sys"

bedit drvadd 2 3 "NewOS\System\eodx.sys"

bedit drvadd 2 3 "NewOS\System\oswm.sys"

@REM PCI Device Drivers run at boot (trying to find root partition and drive)
bedit drvadd 0 5 "NewOS\System\vmsvga.sys" @REM Prior to initialize graphics first so we can have text display
bedit drvadd 0 3 "NewOS\System\ahci.sys"
bedit drvadd 0 3 "NewOS\System\ehci.sys"
bedit drvadd 0 3 "NewOS\System\xhci.sys"


@REM USB Drivers (Manually ran)
bedit drvadd 0 1 "NewOS\System\usbflash.sys"
bedit drvadd 0 1 "NewOS\System\usbmouse.sys"
bedit drvadd 0 1 "NewOS\System\usbkeyboard.sys"

@REM File System Drivers run at preboot phase (to register themselves as fs drivers)
bedit drvadd 1 3 "NewOS\System\fat32.sys"

copy boot.nos diskimg\NewOS\System

wsl sudo make createfs
echo Build finished.