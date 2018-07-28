OBJS=efimain.o
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

LIBOBJS=$(addsuffix .o, $(basename $(wildcard libuefi/*.c)))

default: BOOTX64.EFI

%.o : %.c Makefile
	$(CC) \
		-target x86_64-pc-win32-coff \
		-Iinclude \
		-fno-stack-protector -fshort-wchar -mno-red-zone \
		-Wno-incompatible-library-redeclaration \
		-c -o $*.o $*.c

BOOTX64.EFI : $(OBJS) $(LIBOBJS) Makefile
	$(LD) \
		-subsystem:efi_application -nodefaultlib -dll \
		-entry:efi_main $(OBJS) $(LIBOBJS) -out:$@

clean :
	-rm *.EFI
	-rm *.lib
	-rm *.o
