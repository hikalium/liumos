OBJS= elf_entry.o
OVMF=ovmf/bios64.bin
QEMU=qemu-system-x86_64

CLANG_WARNINGS = \
		-Wall -Wpedantic -Wextra -Wconditional-uninitialized -Wshorten-64-to-32
CXX_STD = -std=c++17
		
CFLAGS= \
		-target x86_64-unknown-none-elf \
		-fno-stack-protector -fshort-wchar \
		-mno-red-zone \
		-nostdlibinc \
		$(CLANG_WARNINGS) $(CXX_STD)

default: LIUMOS.ELF

include ../../common.mk

%.o : %.cc $(HEADERS) Makefile
	$(CC) $(CFLAGS) \
		-c -fPIC -o $*.o $*.cc

LIUMOS.ELF : $(OBJS) kernel.ld Makefile
	$(LD_LLD) -o $@ -e ELFEntry -T kernel.ld $(OBJS)

format:
	clang-format -i *.cc

clean:
	-rm LIUMOS.ELF
	-rm *.o
