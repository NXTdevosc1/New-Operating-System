@echo off
echo Building the kernel...
cd src/nos
call ./compile.bat
cd ../..



wsl sudo make createfs
echo Build finished.