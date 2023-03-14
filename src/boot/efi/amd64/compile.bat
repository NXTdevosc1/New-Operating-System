
set EFILIBS="a" "b"

cl boot.c %EFILIBS% /I../../../../inc/efi /I../../../../inc/efi/X64 /Fe:bootx64.efi /link /MACHINE:x64 /SUBSYSTEM:EFI_APPLICATION /Entry:UefiEntry