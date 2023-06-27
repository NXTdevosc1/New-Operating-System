cl /O2 *.c /Fo:obj/ /LD /I../../inc /Fe:osusr.dll /link /DYNAMICBASE /SUBSYSTEM:windows
copy osusr.dll "..\..\diskimg\newos\system"
copy osusr.lib "..\"