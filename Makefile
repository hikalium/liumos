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

src/kernel/LIUMOS.ELF :
	make -C src/kernel LIUMOS.ELF

files : src/BOOTX64.EFI $(APPS) src/kernel/LIUMOS.ELF .FORCE
	mkdir -p mnt/
	-rm -r mnt/*
	mkdir -p mnt/EFI/BOOT
	cp src/BOOTX64.EFI mnt/EFI/BOOT/
	cp dist/logo.ppm mnt/
	cp $(APPS) mnt/
	cp src/kernel/LIUMOS.ELF mnt/LIUMOS.ELF

run_nopmem : files .FORCE
	$(QEMU) $(QEMU_ARGS)
	
run : files pmem.img .FORCE
	$(QEMU) $(QEMU_ARGS_PMEM)

run_vb_dbg : .FORCE
	- VBoxManage storageattach liumOS --storagectl SATA --port 0 --medium none
	- VBoxManage closemedium disk liumos.vdi --delete
	make liumos.vdi
	VBoxManage storageattach liumOS --storagectl SATA --port 0 --medium liumos.vdi --type hdd
	VirtualBoxVM --startvm liumOS --dbg

img : files .FORCE
	dd if=/dev/zero of=liumos.img bs=16384 count=1024
	/usr/local/Cellar/dosfstools/4.1/sbin/mkfs.vfat liumos.img
	mkdir -p mnt_img
	hdiutil attach -mountpoint mnt_img liumos.img
	cp -r mnt/* mnt_img/
	hdiutil detach mnt_img

liumos.vdi : .FORCE img
	-rm liumos.vdi
	vbox-img convert --srcfilename liumos.img --srcformat RAW --dstfilename liumos.vdi --dstformat VDI

unittest :
	make -C src unittest

clean :
	make -C src clean
	make -C src/kernel clean

format :
	make -C src format

commit : format
	git add .
	git diff HEAD --color=always | less -R
	git commit
