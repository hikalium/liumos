default: src/BOOTX64.EFI

include common.mk

OVMF=ovmf/bios64.bin
QEMU=qemu-system-x86_64
QEMU_ARGS=\
					 -bios $(OVMF) \
					 -machine q35,nvdimm -cpu qemu64 -smp 4 \
					 -monitor stdio \
					 -monitor telnet:127.0.0.1:$(PORT_MONITOR),server,nowait \
					 -m 8G,slots=2,maxmem=10G \
					 -drive format=raw,file=fat:rw:mnt -net none \
					 -serial tcp::1234,server,nowait \
					 -serial tcp::1235,server,nowait \
                     -device qemu-xhci -device usb-mouse -device usb-kbd


QEMU_ARGS_PMEM=\
					 $(QEMU_ARGS) \
					 -object memory-backend-file,id=mem1,share=on,mem-path=pmem.img,size=2G \
					 -device nvdimm,id=nvdimm1,memdev=mem1
VNC_PASSWORD=a
PORT_MONITOR=1240

APPS=app/hello/hello.bin app/pi/pi.bin

ifdef SSH_CONNECTION
QEMU_ARGS+= -vnc :5,password
endif
	
src/BOOTX64.EFI : .FORCE
	make -C src

src/LIUMOS.ELF : .FORCE
	make -C src LIUMOS.ELF

.FORCE :

tools : .FORCE
	make -C tools

pmem.img :
	qemu-img create $@ 2G

app/% : .FORCE
	make -C $(dir $@)

files : src/BOOTX64.EFI $(APPS) src/LIUMOS.ELF .FORCE
	mkdir -p mnt/
	-rm -r mnt/*
	mkdir -p mnt/EFI/BOOT
	cp src/BOOTX64.EFI mnt/EFI/BOOT/
	cp dist/* mnt/
	cp $(APPS) mnt/
	cp src/LIUMOS.ELF mnt/LIUMOS.ELF

run_nopmem : files .FORCE
	$(QEMU) $(QEMU_ARGS)

LLDB_ARGS = -o 'settings set interpreter.prompt-on-quit false' \
			-o 'process launch' \
			-o 'process handle -s false SIGUSR1 SIGUSR2'

run_xhci_gdb : files .FORCE
	lldb $(LLDB_ARGS) -- $(QEMU) $(QEMU_ARGS_XHCI) $(QEMU_ARGS)
	
run : files pmem.img .FORCE
	$(QEMU) $(QEMU_ARGS_PMEM) || reset

run_gdb : files pmem.img .FORCE
	$(QEMU) $(QEMU_ARGS_PMEM) -gdb tcp::1192 -S || reset

run_gdb_nogui : files pmem.img .FORCE
	( echo 'change vnc password $(VNC_PASSWORD)' | while ! nc localhost 1240 ; do sleep 1 ; done ) &
	$(QEMU) $(QEMU_ARGS_PMEM) -gdb tcp::1192 -S -vnc :0,password || reset

run_vnc : files pmem.img .FORCE
	( echo 'change vnc password $(VNC_PASSWORD)' | while ! nc localhost 1240 ; do sleep 1 ; done ) &
	$(QEMU) $(QEMU_ARGS_PMEM) -vnc :0,password || reset

gdb : .FORCE
	gdb -ex 'target remote localhost:1192' src/LIUMOS.ELF


install : files .FORCE
	@read -p "Write LIUMOS to /Volumes/LIUMOS. Are you sure? [Enter to proceed, or Ctrl-C to abort] " && \
		cp -r mnt/* /Volumes/LIUMOS/ && diskutil eject /Volumes/LIUMOS/ && echo "install done."

run_vb_dbg : .FORCE
	- VBoxManage storageattach liumOS --storagectl SATA --port 0 --medium none
	- VBoxManage closemedium disk liumos.vdi --delete
	make liumos.vdi
	VBoxManage storageattach liumOS --storagectl SATA --port 0 --medium liumos.vdi --type hdd
	VirtualBoxVM --startvm liumOS --dbg

img : files .FORCE
	dd if=/dev/zero of=liumos.img bs=16384 count=1024
	/usr/local/Cellar/dosfstools/4.1/sbin/mkfs.vfat liumos.img || /usr/local/Cellar/dosfstools/4.1/sbin/mkfs.fat liumos.img
	mkdir -p mnt_img
	hdiutil attach -mountpoint mnt_img liumos.img
	cp -r mnt/* mnt_img/
	hdiutil detach mnt_img

liumos.vdi : .FORCE img
	-rm liumos.vdi
	vbox-img convert --srcfilename liumos.img --srcformat RAW --dstfilename liumos.vdi --dstformat VDI

serial :
	screen /dev/tty.usbserial-* 115200

vnc :
	open vnc://localhost:5900

unittest :
	make -C src unittest

clean :
	make -C src clean

format :
	make -C src format

commit : format unittest
	git add .
	git diff HEAD --color=always | less -R
	git commit
