@echo off

if not defined LIB ( 
call vcvars64.bat
)

set DRVLIBS="..\..\..\lib"

echo Building acpi.sys
cd src/drivers/acpi
call ./compile.bat
echo building eodx.sys
cd ../eodx
call ./compile

cd ../rtc
call ./compile

cd ../../..




echo building boot config
bedit bootinit
bedit drvadd 2 3 "NewOS\System\acpi.sys"
bedit drvadd 2 3 "NewOS\System\eodx.sys"

bedit drvadd 0 5 "NewOS\System\rtc.sys"

copy boot.nos diskimg\NewOS\System