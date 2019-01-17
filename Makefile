OVMF=ovmf/bios64.bin
QEMU=qemu-system-x86_64
QEMU_ARGS=\
					 -bios $(OVMF) \
					 -machine q35,nvdimm -cpu qemu64 -smp 4 \
					 -monitor stdio \
					 -m 8G,slots=2,maxmem=10G \
					 -drive format=raw,file=fat:rw:mnt -net none

QEMU_ARGS_PMEM=\
					 $(QEMU_ARGS) \
					 -object memory-backend-file,id=mem1,share=on,mem-path=pmem.img,size=2G \
					 -device nvdimm,id=nvdimm1,memdev=mem1

APPS=app/hello/hello.bin

ifdef SSH_CONNECTION
QEMU_ARGS+= -vnc :5,password
endif
	
src/BOOTX64.EFI : .FORCE
	make -C src

.FORCE :

tools : .FORCE
	make -C tools

pmem.img :
	qemu-img create $@ 2G

app/% :
	make -C $(dir $@)

run_nopmem : src/BOOTX64.EFI $(APPS)
	mkdir -p mnt/
	-rm -r mnt/*
	mkdir -p mnt/EFI/BOOT
	cp src/BOOTX64.EFI mnt/EFI/BOOT/
	cp $(APPS) mnt/
	$(QEMU) $(QEMU_ARGS)
	
run : src/BOOTX64.EFI pmem.img $(APPS)
	mkdir -p mnt/
	-rm -r mnt/*
	mkdir -p mnt/EFI/BOOT
	cp src/BOOTX64.EFI mnt/EFI/BOOT/
	cp dist/logo.ppm mnt/
	cp $(APPS) mnt/
	$(QEMU) $(QEMU_ARGS_PMEM)

unittest :
	make -C src unittest

clean :
	make -C src clean

format :
	make -C src format

commit : format
	git add .
	git diff HEAD
	git commit
