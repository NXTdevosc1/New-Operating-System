

img:
	make efiboot
	make createfs
clearimg:
	dd if=/dev/zero of=build/uefi.img bs=512 count=93750

createfs:
# Copy Files to diskimg
	cp edk2/Build/MdeModule/RELEASE_GCC5/X64/bootx64.efi diskimg/efi/boot
	losetup --offset 1048576 --sizelimit 46934528 /dev/loop0 build/uefi.img
	mkdosfs -F 32 /dev/loop0
	mount /dev/loop0 /hdd
	-cp -r diskimg/. /hdd/
	umount /hdd
	losetup -d /dev/loop0
efiboot:
	cd edk2 && build -a X64 -t GCC5 -p MdeModulePkg/MdeModulePkg.dsc