TARGET=efimain
CC=$(shell brew --prefix llvm)/bin/clang
LD=$(shell brew --prefix llvm)/bin/lld-link
OVMF=ovmf/bios64.bin
QEMU=qemu-system-x86_64

LIBOBJS=$(addsuffix .o, $(basename $(wildcard libuefi/*.c)))

default: $(TARGET).elf

%.o : %.c Makefile
	clang \
		-target x86_64-pc-win32-coff \
		-Iinclude \
		-fno-stack-protector -fshort-wchar -mno-red-zone \
		-Wno-incompatible-library-redeclaration \
		-c -o $*.o $*.c

%.elf : %.o $(LIBOBJS) Makefile
	$(LD) \
		-subsystem:efi_application -nodefaultlib -dll \
		-entry:efi_main $*.o $(LIBOBJS) -out:$@

clean :
	-rm *.elf
	-rm *.lib
