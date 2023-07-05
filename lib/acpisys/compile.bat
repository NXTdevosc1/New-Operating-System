cl /O2 *.c "../noskx64.lib" "../syscruntime.lib" /GS- /I../../inc /Fe:acpisys.dll /link /DYNAMICBASE /SUBSYSTEM:native /ENTRY:AcpiLibEntry
copy acpisys.lib ..
copy acpisys.dll "..\..\diskimg\newos\system\libraries"