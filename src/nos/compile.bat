FOR /R "./" %%S IN (*.asm) DO (
	nasm "%%S" -Ox -f win64 -o "../../build/nos/assembly/%%~nS.s.obj"
)

cl /O2 *.c /I../../inc "../../build/nos/assembly/*.obj" /Fo:../../build/nos /Fe:noskx64.exe /link /FIXED:no /LARGEADDRESSAWARE /BASE:0xffff800000000000 /ENTRY:NosKernelEntry /SUBSYSTEM:native

copy noskx64.exe ..\..\diskimg\NewOS\System\