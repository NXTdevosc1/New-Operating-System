

FOR /R "./" %%S IN (*.asm) DO (
	nasm "%%S" -Ox -f win64 -o "../../build/nos/assembly/%%~nS.s.obj"
)


set srcfiles=*.c processor/*.c console/*.c pnp/*.c ob/*.c lock/*.c mm/*.c task/*.c sys/*.c loader/*.c
set libsource=../../lib
cl /O2 /GL %srcfiles% /GS- /I../../inc "../../build/nos/assembly/*.obj" "../../lib/syscruntime.lib" /KERNEL /Fo:../../build/nos/ /Fe:noskx64.exe  /link /PDB:no /FIXED /LARGEADDRESSAWARE /RELEASE /BASE:0xffff800000000000 /ENTRY:NosKernelEntry /SUBSYSTEM:native

copy noskx64.exe ..\..\diskimg\NewOS\System\
copy noskx64.lib ..\..\diskimg\NewOS\System\
copy noskx64.lib ..\..\lib
