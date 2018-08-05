OBJS=efimain.o
HEADERS=efi.h
OVMF=ovmf/bios64.bin
QEMU=qemu-system-x86_64

OSNAME=${shell uname -s}
ifeq ($(OSNAME),Darwin)
	LLVM_PREFIX:=$(shell brew --prefix llvm)
	CC:=$(LLVM_PREFIX)/bin/clang
	LD:=$(LLVM_PREFIX)/bin/lld-link	
else
	CC:=clang
	LD:=lld-link-4.0
endif

default: BOOTX64.EFI

%.o : %.c $(HEADERS) Makefile
	$(CC) \
		-target x86_64-pc-win32-coff \
		-fno-stack-protector -fshort-wchar \
		-mno-red-zone \
		-nostdlibinc \
		-Wall -Wpedantic \
		-c -o $*.o $*.c

BOOTX64.EFI : $(OBJS) $(HEADERS) Makefile
	$(LD) \
		-subsystem:efi_application -nodefaultlib -dll \
		-entry:efi_main $(OBJS) -out:$@

run:
	make -C .. run

clean :
	-rm *.EFI
	-rm *.lib
	-rm *.o
