cl /O2i- *.c /I../../inc /LD /Fe:syscruntime.dll /Fo:obj/ /link /DYNAMICBASE /FIXED:no /MACHINE:x64 /subsystem:native /dll /nodefaultlib
cl /O2i- *.c /I../../inc /LD /Fe:cruntime.dll /Fo:obj/ /link /DYNAMICBASE /FIXED:no /MACHINE:x64 /subsystem:windows /entry:DllMain /dll /nodefaultlib

copy cruntime.lib ..\
copy syscruntime.lib ..\

copy cruntime.dll ..\..\diskimg\NewOS\System\Libraries
copy syscruntime.dll ..\..\diskimg\NewOS\System\Libraries
