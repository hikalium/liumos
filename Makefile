TARGET=efimain
OVMF=ovmf/bios64.bin
QEMU=qemu-system-x86_64

src/$(TARGET).elf :
	make -C src

run : src/$(TARGET).elf
	mkdir -p mnt/EFI/BOOT
	cp src/$(TARGET).elf mnt/EFI/BOOT/BOOTX64.EFI
	$(QEMU) -bios $(OVMF) -drive file=fat:ro:mnt

clean :
	make -C src clean
