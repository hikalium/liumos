PROJECT_ROOT:=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))
OVMF?=${PROJECT_ROOT}/ovmf/bios64.bin
QEMU?=qemu-system-x86_64
VNC_PASSWORD?=a
PORT_MONITOR?=2222
PORT_COM1?=1234
PORT_COM2?=1235
# PORT_VNC=N => port 5900 + N
PORT_OFFSET_VNC?=5
PORT_GDB?=3333

#
# Args for `make run` - will be constructed in the code below
#

QEMU_ARGS=

#
# Base machine config
#

QEMU_ARGS_COMMON=\
		-bios $(OVMF) \
		-machine q35,nvdimm=on -cpu qemu64 -smp 4 \
		-m 2G,slots=2,maxmem=4G \
		-object memory-backend-file,id=mem1,share=on,mem-path=pmem.img,size=2G \
		-device nvdimm,id=nvdimm1,memdev=mem1 \
		-device qemu-xhci -device usb-mouse,id=usbmouse1 \
		-drive format=raw,file=fat:rw:mnt \
		-rtc base=localtime \
		-device isa-debug-exit,iobase=0xf4,iosize=0x01 \
		-monitor telnet:0.0.0.0:$(PORT_MONITOR),server,nowait \

QEMU_ARGS+=$(QEMU_ARGS_COMMON)

#
# GUI settings
#
QEMU_ARGS_GUI:=\
		-vga std

QEMU_ARGS_NOGUI:=\
		-vnc 0.0.0.0:$(PORT_OFFSET_VNC),password=on \
		-nographic

GUI?=y
ifeq (${GUI}, y)
QEMU_ARGS+=$(QEMU_ARGS_GUI)
else ifeq (${GUI}, n)
QEMU_ARGS+=$(QEMU_ARGS_NOGUI)
else
$(error Invalid value $(GUI) for GUI)
endif

#
# stdio settings
#
QEMU_ARGS_STDIO_MONITOR:=\
		-monitor stdio \
		-serial tcp::${PORT_COM1},server,nowait \
		-serial tcp::${PORT_COM2},server,nowait \

QEMU_ARGS_STDIO_SERIAL:=\
		-chardev stdio,id=char0,mux=on \
		-serial chardev:char0 \
		-serial chardev:char0

STDIO?=monitor
ifeq (${STDIO}, monitor)
QEMU_ARGS+=$(QEMU_ARGS_STDIO_MONITOR)
else ifeq (${STDIO}, serial)
QEMU_ARGS+=$(QEMU_ARGS_STDIO_SERIAL)
else
$(error Invalid value $(STDIO) for STDIO)
endif

#
# Network configs
#
# guest 10.0.2.1:8888 -> host 127.0.0.1:8888
# guest 10.0.2.1:8889 <- host 127.0.0.1:8889

QEMU_ARGS_NET_VIRTIO_USER:=\
		-chardev udp,id=m8,host=0.0.0.0,port=8888 \
		-nic user,id=u1,model=virtio,guestfwd=::8888-chardev:m8,hostfwd=udp:0.0.0.0:8889-0.0.0.0:8889 \
		-object filter-dump,id=f1,netdev=u1,file=dump.dat

QEMU_ARGS_NET_RTL8139:=\
		-nic user,id=u1,model=rtl8139,hostfwd=tcp::10023-:23 \
		-object filter-dump,id=f1,netdev=u1,file=dump.dat

QEMU_ARGS_NET_TAP:=\
		-nic tap,ifname=tap0,id=u1,model=virtio,script=no \
		-object filter-dump,id=f1,netdev=u1,file=dump.dat

QEMU_ARGS_NET_USB:=\
		-netdev user,id=usbnet0 -device usb-net,netdev=usbnet0 \
		-object filter-dump,id=f2,netdev=usbnet0,file=dump_usb_nic.dat

NET?=virtio_user
ifeq (${NET}, virtio_user)
QEMU_ARGS+=${QEMU_ARGS_NET_VIRTIO_USER}
else ifeq (${NET}, rtl8139)
QEMU_ARGS+=${QEMU_ARGS_NET_RTL8139}
else ifeq (${NET}, tap)
QEMU_ARGS+=${QEMU_ARGS_NET_TAP}
else ifeq (${NET}, usb)
QEMU_ARGS+=${QEMU_ARGS_NET_USB}
else
$(error Invalid value ${NET} for NET)
endif

#
# GDB configs
#
GDB?=n
ifeq (${GDB}, n)
# Do nothing
else ifeq (${GDB}, suspend_on_boot)
QEMU_ARGS+= -gdb tcp::${PORT_GDB} -S
else ifeq (${GDB}, nosuspend)
QEMU_ARGS+= -gdb tcp::${PORT_GDB}
else
$(error Invalid value ${GDB} for GDB)
endif

#
# Audio configs
#
AUDIO?=n
ifeq (${AUDIO}, n)
# Do nothing
else ifeq (${AUDIO}, adlib)
QEMU_ARGS+= -device adlib
else
$(error Invalid value ${AUDIO} for AUDIO)
endif

#
# Utilities
#

pmem.img :
	qemu-img create $@ 2G

watch_com1:
	while ! telnet localhost ${PORT_COM2} ; do sleep 1 ; done ;

watch_com2:
	while ! telnet localhost ${PORT_COM2} ; do sleep 1 ; done ;

#
# run recipes
#

common_run_rust :
	make -C ${PROJECT_ROOT}/loader install
	cd ${PROJECT_ROOT} && $(QEMU) $(QEMU_ARGS_PMEM)

run_root : files pmem.img
	make run_nobuild_root

run_nobuild_root : pmem.img
ifeq (${GUI}, n)
	# Set VNC password
	( echo 'change vnc password $(VNC_PASSWORD)' | while ! nc localhost $(PORT_MONITOR) ; do sleep 1 ; done ) &
endif
	$(QEMU) $(QEMU_ARGS)

internal_run_loader_test :
	@echo Using ${LOADER_TEST_EFI} as a loader...
	pwd
	mkdir -p mnt/
	-rm -rf mnt/*
	mkdir -p mnt/EFI/BOOT
	cp ${LOADER_TEST_EFI} mnt/EFI/BOOT/BOOTX64.EFI
	$(QEMU) $(QEMU_ARGS_COMMON) $(QEMU_ARGS_GUI) $(QEMU_ARGS_STDIO_SERIAL) ; \
		RETCODE=$$? ; \
		if [ $$RETCODE -eq 3 ]; then \
			echo "\nPASS" ; \
			exit 0 ; \
		else \
			echo "\nFAIL: QEMU returned $$RETCODE" ; \
			exit 1 ; \
		fi

.PHONY : run_root
