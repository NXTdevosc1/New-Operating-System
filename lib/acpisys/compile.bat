cl /O2 *.c "../noskx64.lib" /I../../inc /Fe:acpisys.dll /link /DYNAMICBASE /SUBSYSTEM:native /ENTRY:AcpiLibEntry
copy acpisys.lib ..
copy acpisys.dll "..\..\diskimg\newos\system"