cl /O2 /GL /GS- "../noskx64.lib" *.c /Iinc /I../../inc /LD /Fe:syscruntime.dll /Fo:obj/ /link /LTCG /DYNAMICBASE /MACHINE:x64 /subsystem:native /dll /nodefaultlib /EXPORT:memset /EXPORT:memcpy /EXPORT:memcmp /EXPORT:strlen
cl /O2 /GL /GS- "../noskx64.lib" *.c /Iinc /I../../inc /LD /Fe:cruntime.dll /Fo:obj/ /link /LTCG /DYNAMICBASE /MACHINE:x64 /subsystem:windows /entry:DllMain /dll /nodefaultlib /EXPORT:memset /EXPORT:memcpy /EXPORT:memcmp /EXPORT:strlen

copy cruntime.lib ..\
copy syscruntime.lib ..\

copy cruntime.dll ..\..\diskimg\NewOS\System\Libraries
copy syscruntime.dll ..\..\diskimg\NewOS\System\Libraries
