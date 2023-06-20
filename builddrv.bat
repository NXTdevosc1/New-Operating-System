@echo off

if not defined LIB ( 
call vcvars64.bat
)

echo Building acpy.sys
cd src/drivers/acpi
call ./compile.bat
echo building eodx.sys
cd ../eodx
call ./compile

cd ../../..


echo building boot config
bedit bootinit
bedit drvadd 2 3 "NewOS\System\acpi.sys"
bedit drvadd 2 3 "NewOS\System\eodx.sys"
copy boot.nos diskimg\NewOS\System