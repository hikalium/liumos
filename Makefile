OVMF=ovmf/bios64.bin
QEMU=qemu-system-x86_64
QEMU_ARGS=-bios $(OVMF) \
		  -drive file=fat:ro:mnt \
		  -monitor stdio

ifdef $(SSH_CONNECTION)
QEMU_ARGS+=-vnc :5,password
else
QEMU_ARGS+=
endif

src/BOOTX64.EFI :
	make -C src

run : src/BOOTX64.EFI
	mkdir -p mnt/EFI/BOOT
	cp src/BOOTX64.EFI mnt/EFI/BOOT/
	$(QEMU) $(QEMU_ARGS)
clean :
	make -C src clean
