OBJS=asm.o console.o draw.o efi.o efimain.o guid.o hpet.o liumos.o static.o font.gen.o
HEADERS=$(wildcard *.h)
OVMF=ovmf/bios64.bin
QEMU=qemu-system-x86_64
CFLAGS= \
		-target x86_64-pc-win32-coff \
		-fno-stack-protector -fshort-wchar \
		-mno-red-zone \
		-nostdlibinc \
		-Wall -Wpedantic -Wshorten-64-to-32 -std=c11
CXXFLAGS= \
		-target x86_64-pc-win32-coff \
		-fno-stack-protector -fshort-wchar \
		-mno-red-zone \
		-nostdlibinc \
		-Wall -Wpedantic -Wshorten-64-to-32 -std=c++14



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

font.gen.cc : font.txt
	../tools/fontmaker font.txt > font.gen.cc

%.o : %.c $(HEADERS) Makefile
	$(CC) $(CFLAGS) \
		-c -o $*.o $*.c

%.o : %.cc $(HEADERS) Makefile
	$(CC) $(CXXFLAGS) \
		-c -o $*.o $*.cc

%.o : %.S Makefile
	$(CC) $(CFLAGS) \
		-c -o $*.o $*.S

BOOTX64.EFI : $(OBJS) $(HEADERS) Makefile
	$(LD) \
		-subsystem:efi_application -nodefaultlib \
		-entry:efi_main $(OBJS) -out:$@

run:
	make -C .. run

clean :
	-rm *.EFI
	-rm *.lib
	-rm *.o
	-rm *.gen.c
	-rm *.gen.cc

format :
	clang-format -i *.cc *.h
