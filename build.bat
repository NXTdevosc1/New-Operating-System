@echo off
if not defined LIB ( 
call vcvars64.bat
)
echo Building the kernel...
cd src/nos
call ./compile.bat
cd ../..



wsl sudo make createfs
echo Build finished.