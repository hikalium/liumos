PROJECT_ROOT:=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))
OVMF=${PROJECT_ROOT}/ovmf/bios64.bin
QEMU=qemu-system-x86_64
VNC_PASSWORD=a
PORT_MONITOR=2222
# PORT_VNC=N => port 5900 + N
PORT_VNC=5

QEMU_ARGS_COMMON_HW_CONFIG=\
		-machine q35,nvdimm=on -cpu qemu64 -smp 4 \
		-bios $(OVMF) \
		-device qemu-xhci -device usb-mouse,id=usbmouse1 \
		-netdev user,id=usbnet0 -device usb-net,netdev=usbnet0 \
		-object filter-dump,id=f2,netdev=usbnet0,file=dump_usb_nic.dat \
		-m 2G,slots=2,maxmem=4G \
		-drive format=raw,file=fat:rw:mnt -net none \
		-rtc base=localtime \

QEMU_ARGS_LOADER_TEST=\
	    $(QEMU_ARGS_COMMON_HW_CONFIG) \
		-device isa-debug-exit,iobase=0xf4,iosize=0x01 \
		-chardev stdio,id=char0,mux=on \
		-monitor none \
		-serial chardev:char0 \
		-serial chardev:char0 \
		-nographic

QEMU_ARGS_COMMON=\
	    $(QEMU_ARGS_COMMON_HW_CONFIG) \
		-monitor stdio \
		-monitor telnet:0.0.0.0:$(PORT_MONITOR),server,nowait \
		-serial tcp::1234,server,nowait \
		-serial tcp::1235,server,nowait

QEMU_ARGS_NET_MACOS=\
		-nic user,id=u1,model=virtio,hostfwd=tcp::10023-:23 \
		-object filter-dump,id=f1,netdev=u1,file=dump.dat

QEMU_ARGS_NET_MACOS_RTL=\
		-nic user,id=u1,model=rtl8139,hostfwd=tcp::10023-:23 \
		-object filter-dump,id=f1,netdev=u1,file=dump.dat

QEMU_ARGS_NET_LINUX_TAP=\
		-nic tap,ifname=tap0,id=u1,model=virtio,script=no \
		-object filter-dump,id=f1,netdev=u1,file=dump.dat

# guest 10.0.2.1:8888 -> host 127.0.0.1:8888
# guest 10.0.2.1:8889 <- host 127.0.0.1:8889

QEMU_ARGS_USER_NET=\
		-chardev udp,id=m8,host=0.0.0.0,port=8888 \
		-nic user,id=u1,model=virtio,guestfwd=::8888-chardev:m8,hostfwd=udp:0.0.0.0:8889-0.0.0.0:8889 \
		-object filter-dump,id=f1,netdev=u1,file=dump.dat

QEMU_ARGS=\
		$(QEMU_ARGS_COMMON) \
		$(QEMU_ARGS_USER_NET)

ifdef SSH_CONNECTION
QEMU_ARGS+= -vnc 0.0.0.0:$(PORT_VNC),password
endif

QEMU_ARGS_PMEM=\
		$(QEMU_ARGS) \
		-object memory-backend-file,id=mem1,share=on,mem-path=pmem.img,size=2G \
		-device nvdimm,id=nvdimm1,memdev=mem1

common_run_rust :
	make -C ${PROJECT_ROOT}/loader install
	cd ${PROJECT_ROOT} && $(QEMU) $(QEMU_ARGS_PMEM)

watch_serial:
	while ! telnet localhost 1235 ; do sleep 1 ; done ;

