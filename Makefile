OVMF=ovmf/bios64.bin
QEMU=qemu-system-x86_64
QEMU_ARGS=\
					 -bios $(OVMF) \
					 -machine pc,nvdimm \
					 -monitor stdio \
					 -m 8G,slots=2,maxmem=10G \
					 -drive file=fat:ro:mnt
	
QEMU_ARGS_PMEM=\
					 -bios $(OVMF) \
					 -machine pc,nvdimm \
					 -monitor stdio \
					 -m 8G,slots=2,maxmem=10G \
					 -object memory-backend-file,id=mem1,share=on,mem-path=pmem.img,size=2G \
					 -device nvdimm,id=nvdimm1,memdev=mem1 \
					 -drive file=fat:ro:mnt


ifdef SSH_CONNECTION
QEMU_ARGS+=-vnc :5,password
QEMU_ARGS_PMEM+=-vnc :5,password
endif

src/BOOTX64.EFI : .FORCE
	make -C src

.FORCE :

pmem.img :
	qemu-img create $@ 2G

run : src/BOOTX64.EFI
	mkdir -p mnt/EFI/BOOT
	cp src/BOOTX64.EFI mnt/EFI/BOOT/
	$(QEMU) $(QEMU_ARGS)
	
run_pmem : src/BOOTX64.EFI pmem.img
	mkdir -p mnt/EFI/BOOT
	cp src/BOOTX64.EFI mnt/EFI/BOOT/
	$(QEMU) $(QEMU_ARGS_PMEM)

clean :
	make -C src clean
