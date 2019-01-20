OBJS= acpi.o apic.o asm.o console.o console_command.o \
		 efi.o efimain.o elf.o file.o \
		 gdt.o generic.o graphics.o guid.o \
		 hpet.o interrupt.o keyboard.o\
		 libfunc.o liumos.o phys_page_allocator.o scheduler.o \
		 font.gen.o
HEADERS=$(wildcard *.h) lib/musl/include/elf.h
OVMF=ovmf/bios64.bin
QEMU=qemu-system-x86_64

CLANG_WARNINGS = \
		-Wall -Wpedantic -Wextra -Wconditional-uninitialized -Wshorten-64-to-32
CXX_STD = -std=c++17
		
CFLAGS= \
		-target x86_64-pc-win32-coff \
		-fno-stack-protector -fshort-wchar \
		-mno-red-zone \
		-nostdlibinc \
		$(CLANG_WARNINGS) -std=c11 \
		-Imusl_supplement -Imusl/include

CXXFLAGS= \
		-target x86_64-pc-win32-coff \
		-fno-stack-protector -fshort-wchar \
		-mno-red-zone \
		-nostdlibinc \
		$(CLANG_WARNINGS) $(CXX_STD)

CXXFLAGS_FOR_TEST= \
		$(CLANG_WARNINGS) $(CXX_STD)

OSX_STD_HEADER_INCLUDE_FLAGS= \
		-I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include \
		-I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include

default: BOOTX64.EFI

include ../common.mk

font.gen.cc : font.txt
	../tools/fontmaker font.txt > font.gen.cc

lib/musl/include/%.h : lib/musl/include/Makefile
	make -C lib/musl/include/ $*.h

%.o : %.c $(HEADERS) Makefile
	$(CC) $(CFLAGS) \
		-c -o $*.o $*.c

%.o : %.cc $(HEADERS) Makefile
	$(CC) $(CXXFLAGS) \
		-c -o $*.o $*.cc

%.o : %.S Makefile
	$(CC) $(CFLAGS) \
		-c -o $*.o $*.S

test_% : %_test.cc Makefile
	$(CC) -o $*_test.bin $*_test.cc -DLIUMOS_TEST $(CXXFLAGS_FOR_TEST) \
		$(OSX_STD_HEADER_INCLUDE_FLAGS) -Wno-nullability-completeness -Wno-nullability-extension
	@./$*_test.bin

BOOTX64.EFI : $(OBJS) $(HEADERS) Makefile
	$(LD) \
		-subsystem:efi_application -nodefaultlib \
		-entry:efi_main $(OBJS) -out:$@

run:
	make -C .. run

unittest:
	make test_ring_buffer

clean :
	-rm *.EFI
	-rm *.lib
	-rm *.o
	-rm *.gen.c
	-rm *.gen.cc
	-rm *.bin

format :
	clang-format -i *.cc *.h

commit :
	make -C .. commit

print_cc :
	@echo "$(CC) $(LD)"


