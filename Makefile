default: src/BOOTX64.EFI

include common.mk
include qemu.mk

src/BOOTX64.EFI : .FORCE
	make -C src

src/LIUMOS.ELF : .FORCE
	make -C src LIUMOS.ELF

.FORCE :

tools : .FORCE
	make -C tools

prebuilt : .FORCE
	./scripts/deploy_prebuilt.sh

apps : .FORCE
	-git submodule update --init --recursive
	make -C app/

deploy_apps : apps .FORCE
	make -C app/ deploy

files : src/BOOTX64.EFI src/LIUMOS.ELF .FORCE
	mkdir -p mnt/
	-rm -rf mnt/*
	mkdir -p mnt/EFI/BOOT
	cp src/BOOTX64.EFI mnt/EFI/BOOT/
	cp dist/* mnt/
	make deploy_apps
	cp src/LIUMOS.ELF mnt/LIUMOS.ELF
	mkdir -p mnt/EFI/EFI/
	echo 'FS0:\\EFI\\BOOT\\BOOTX64.EFI' > mnt/startup.nsh

.PHONY : internal_run_loader_test

LLDB_ARGS = -o 'settings set interpreter.prompt-on-quit false' \
			-o 'process launch' \
			-o 'process handle -s false SIGUSR1 SIGUSR2'

gdb_root : .FORCE
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

run_docker_root : .FORCE
	scripts/make_run_with_docker.sh

stop_docker_root : .FORCE
	docker kill liumos-builder0 && echo "Killed previous container" || true

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

test :
	make spellcheck
	make -C app test
	make -C loader test
	make -C src test

e2etest_root :
	make -C e2etest test

ci :
	circleci config validate
	circleci local execute

clean :
	-rm cc_cache.gen.mk
	make -C src/ clean
	make -C app/ clean

format :
	make -C src format

spellcheck :
	@scripts/spellcheck.sh recieve receive

commit_root : format stop_docker
	make -C e2etest presubmit
	make test
	git submodule update
	git add .
	./scripts/ensure_objs_are_not_under_git_control.sh
	git diff HEAD --color=always | less -R
	git commit

dump_usb_net_packets :
	tcpdump -XX -r dump_usb_nic.dat

dump_virtio_net_packets :
	tcpdump -XX -r dump.dat
