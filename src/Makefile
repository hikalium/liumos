LOADER_SRCS= acpi.cc apic.cc asm.S \
						 console.cc \
						 efi.cc efimain.cc elf.cc execution_context.cc \
						 file.cc \
						 gdt.cc generic.cc graphics.cc guid.cc \
						 interrupt.cc \
						 libfunc.cc loader.cc \
						 paging.cc phys_page_allocator.cc pmem.cc process.cc \
						 serial.cc sheet.cc \
						 sys_constant.cc \
						 text_box.cc \
						 font.gen.c
KERNEL_SRCS= acpi.cc apic.cc asm.S \
						 console.cc command.cc\
						 efi.cc elf.cc execution_context.cc \
						 gdt.cc generic.cc guid.cc \
						 hpet.cc \
						 interrupt.cc \
						 kernel.cc keyboard.cc \
						 newlib_support.cc \
						 paging.cc phys_page_allocator.cc pmem.cc process.cc\
						 scheduler.cc serial.cc sheet.cc subtask.cc \
						 sleep_handler.S sys_constant.cc syscall.cc syscall_handler.S\
						 text_box.cc \
						 font.gen.cc
KERNEL_LIBS=third_party/newlib-cygwin_dest/lib/libc.a
LOADER_OBJS= $(addsuffix .o, $(basename $(LOADER_SRCS)))
KERNEL_OBJS= $(addsuffix .elf.o, $(basename $(KERNEL_SRCS)))
HEADERS=$(filter-out githash.h, $(wildcard *.h)) lib/musl/include/elf.h
OVMF=ovmf/bios64.bin
QEMU=qemu-system-x86_64

CLANG_WARNINGS = \
		-Wall -Wpedantic -Wextra -Wconditional-uninitialized -Wshorten-64-to-32 \
		-Wno-keyword-macro
C_STD = -std=c11
CXX_STD = -std=c++17
		
CFLAGS_LOADER= \
		-target x86_64-pc-win32-coff \
		-fno-stack-protector -fno-exceptions -fshort-wchar \
		-mno-red-zone \
		-nostdlibinc \
		$(CLANG_WARNINGS) $(C_STD) \
		-Imusl_supplement -Imusl/include

CXXFLAGS_LOADER= \
		-target x86_64-pc-win32-coff \
		-fno-stack-protector -fno-exceptions -fshort-wchar \
		-mno-red-zone \
		-nostdlibinc -nostdlib \
		$(CLANG_WARNINGS) $(CXX_STD)

CFLAGS_KERNEL= \
		-target x86_64-unknown-none-elf \
		-fno-stack-protector -fno-exceptions -fshort-wchar \
		-mno-red-zone -mstack-alignment=8 \
		-nostdlibinc \
		-Ithird_party/newlib-cygwin_dest/include/ \
		$(CLANG_WARNINGS) $(C_STD)

CXXFLAGS_KERNEL= \
		-target x86_64-unknown-none-elf \
		-fno-stack-protector -fno-exceptions -fshort-wchar \
		-mno-red-zone -mstack-alignment=8 \
		-nostdlibinc \
		-Ithird_party/newlib-cygwin_dest/include/ \
		$(CLANG_WARNINGS) $(CXX_STD)

CXXFLAGS_FOR_TEST= \
		$(CLANG_WARNINGS) $(CXX_STD)

OSX_STD_HEADER_INCLUDE_FLAGS= \
		-I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include \
		-I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include

default: BOOTX64.EFI LIUMOS.ELF

include ../common.mk

font.gen.cc : font.txt
	../tools/fontmaker font.txt > font.gen.cc

lib/musl/include/%.h : lib/musl/include/Makefile
	make -C lib/musl/include/ $*.h

%.o : %.c $(HEADERS) Makefile
	$(CC) $(CFLAGS_LOADER) \
		-c -o $*.o $*.c

liumos.o : liumos.cc githash.h $(HEADERS) Makefile
	$(CC) $(CXXFLAGS_LOADER) \
		-c -o $*.o $*.cc

%.o : %.cc $(HEADERS) Makefile
	$(CC) $(CXXFLAGS_LOADER) \
		-c -o $*.o $*.cc

%.o : %.S Makefile
	$(CC) $(CFLAGS_LOADER) \
		-c -o $*.o $*.S

%.o.dump.txt : %.o Makefile
	objdump -d $*.o > $@

test_% : %_test.cc Makefile
	$(CC) -g -o $*_test.bin $*_test.cc -DLIUMOS_TEST $(CXXFLAGS_FOR_TEST) \
		$(OSX_STD_HEADER_INCLUDE_FLAGS) -Wno-nullability-completeness -Wno-nullability-extension
	@./$*_test.bin

BOOTX64.EFI : $(LOADER_OBJS) $(HEADERS) Makefile
	$(LLD_LINK) \
		-subsystem:efi_application -nodefaultlib \
		-entry:efi_main $(LOADER_OBJS) -out:$@

%.elf.o : %.cc $(HEADERS) Makefile
	$(CC) $(CXXFLAGS_KERNEL) \
		-g -c -fPIC -o $*.elf.o $*.cc

%.elf.o : %.c $(HEADERS) Makefile
	$(CC) $(CFLAGS_KERNEL) \
		-g -c -fPIC -o $*.elf.o $*.c

%.elf.o : %.S Makefile
	$(CC) $(CXXFLAGS_KERNEL) \
		-c -o $*.elf.o $*.S

LIUMOS.ELF : $(KERNEL_OBJS) kernel.ld Makefile
	$(LD_LLD) -o $@ -e KernelEntry -T kernel.ld -L third_party/newlib-cygwin_dest/lib -lc -static $(KERNEL_OBJS)

NEWLIB_BUILD_DIR=third_party/newlib-cygwin_build

newlib:
	git submodule update
	mkdir -p $(NEWLIB_BUILD_DIR)
	cd $(NEWLIB_BUILD_DIR) \
	&& \
	CC=/usr/local/opt/llvm/bin/clang \
		AR=/usr/local/opt/llvm/bin/llvm-ar \
		RANLIB=/usr/local/opt/llvm/bin/llvm-ranlib \
		../newlib-cygwin/newlib/configure \
		CFLAGS="-nostdlibinc -O2 -target x86_64-unknown-none-elf -mcmodel=large" \
		--target=x86_64-elf --disable-multilib \
		--prefix=$$(dirname `pwd`)/newlib-cygwin_dest \
	&& \
	make -j4 install

run:
	make -C .. run

unittest:
	make test_ring_buffer
	make test_paging

clean :
	-make -C $(NEWLIB_BUILD_DIR) clean
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

sym : LIUMOS.ELF
	/usr/local/opt/binutils/bin/gobjdump -t LIUMOS.ELF | more

print_cc :
	@echo "$(CC) $(LD)"

dump : $(addsuffix .dump.txt, $(OBJS))

FORCE : 

githash.h: FORCE
	echo '#define GIT_HASH "$(shell git rev-parse HEAD | tr -d "\n") $(shell ./git_modification_check.sh)"' > $@
