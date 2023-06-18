cl /MT /O2 src/*.c src/user/*.c /Iinc /I../../inc /LD /Fe:eodx.dll /Fo:obj/ /link /DYNAMICBASE /FIXED:no /MACHINE:x64 /subsystem:windows /dll /nodefaultlib /entry:DllMain
cl /MT /O2 src/*.c src/kernel/*.c /Iinc /I../../inc /LD /Fe:eodxsys.dll /Fo:obj/ /link /DYNAMICBASE /FIXED:no /MACHINE:x64 /subsystem:native /entry:DllMain /dll /nodefaultlib

copy eodx.lib ..\
copy eodxsys.lib ..\

copy eodx.dll ..\..\diskimg\NewOS\System
copy eodxsys.dll ..\..\diskimg\NewOS\System
