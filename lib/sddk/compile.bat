cl /O2 *.c /I../../inc /GS- /LD /Fe:sddk.dll /Fo:obj/ /link /DYNAMICBASE /SUBSYSTEM:Native /DLL /ENTRY:DllMain

copy sddk.lib ..\

copy sddk.dll ..\..\diskimg\NewOS\System\Libraries