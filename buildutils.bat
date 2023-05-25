if not defined LIB ( 
call vcvars64.bat
)
cd src/bedit
call ./compile.bat
cd ../..

bedit bootinit
bedit drvadd 2 3 "NewOS\System\acpi.sys"
copy boot.nos diskimg\NewOS\System