default: BOOTX64.EFI LIUMOS.ELF

include ../common.mk

COMMON_SRCS= \
			 acpi.cc apic.cc asm.S inthandler.S \
			 console.cc \
			 efi.cc elf.cc execution_context.cc \
			 gdt.cc generic.cc githash.cc graphics.cc guid.cc \
			 interrupt.cc \
			 paging.cc phys_page_allocator.cc pmem.cc process.cc \
			 serial.cc sheet.cc sheet_painter.cc \
			 sys_constant.cc \
			 text_box.cc \
			 font.gen.cc \

LOADER_SRCS= $(COMMON_SRCS) \
			 efimain.cc \
			 efi_file_manager.cc \
			 libfunc.cc loader.cc

KERNEL_SRCS= $(COMMON_SRCS) \
			 command.cc \
			 hpet.cc \
			 kernel.cc keyboard.cc \
			 libcxx_support.cc \
			 newlib_support.cc \
			 pci.cc \
			 scheduler.cc subtask.cc \
			 sleep_handler.S syscall.cc syscall_handler.S \
			 xhci.cc

LOADER_OBJS= $(addsuffix .o, $(basename $(LOADER_SRCS)))
LOADER_DEPS= $(addsuffix .o.d, $(basename $(LOADER_SRCS)))
KERNEL_OBJS= $(addsuffix .elf.o, $(basename $(KERNEL_SRCS)))
KERNEL_DEPS= $(addsuffix .elf.d, $(basename $(KERNEL_SRCS)))
OVMF=ovmf/bios64.bin
QEMU=qemu-system-x86_64

CLANG_WARNINGS = \
				 -Wall -Wpedantic -Wextra -Wconditional-uninitialized -Wshorten-64-to-32 \
				 -Werror \
				 -Wno-keyword-macro
C_STD = -std=c11
CXX_STD = -std=c++17

LIBCXX_FOR_KERNEL_INC_PATH=third_party_root/include/c++/v1
LIBCXX_FOR_KERNEL_LIB_PATH=third_party_root/lib

CFLAGS_LOADER= \
			   -target x86_64-pc-win32-coff \
			   -fno-stack-protector -fno-exceptions -fshort-wchar \
			   -mno-red-zone \
			   -nostdlibinc \
			   $(CLANG_WARNINGS) $(C_STD) \
			   -Ithird_party_root/include

CXXFLAGS_LOADER= \
				 -DLIUMOS_LOADER \
				 -target x86_64-pc-win32-coff \
				 -fno-stack-protector -fno-exceptions -fshort-wchar \
				 -mno-red-zone \
				 -nostdlibinc -nostdlib \
				 -D_LIBCPP_HAS_NO_THREADS -D_LIBCPP_OBJECT_FORMAT_COFF -Ithird_party_root/include \
				 $(CLANG_WARNINGS) $(CXX_STD)

CFLAGS_KERNEL= \
			   -target x86_64-unknown-none-elf \
			   -fno-stack-protector -fno-exceptions -fshort-wchar \
			   -mno-red-zone \
			   -nostdlib -nostdinc\
			   $(CLANG_WARNINGS) $(C_STD)

# -Wno-gnu-include-next: avoid warning from third_party/newlib-cygwin_dest/include/limits.h:132:3
CXXFLAGS_KERNEL= \
				 -target x86_64-unknown-none-elf \
				 -fno-stack-protector -fno-exceptions -fshort-wchar -fno-rtti \
				 -mno-red-zone -mcmodel=large \
				 -nostdlibinc \
				 -D__ELF__ -D_LDBL_EQ_DBL \
				 -I$(LIBCXX_FOR_KERNEL_INC_PATH) \
				 -Ithird_party_root/include/ \
				 -I$(CLANG_SYSTEM_INC_PATH) \
				 $(CLANG_WARNINGS) \
				 -Wno-gnu-include-next \
				 $(CXX_STD)


LDFLAGS_KERNEL= \
				-L$(LIBCXX_FOR_KERNEL_LIB_PATH) -lc++ -lc++abi -lm \
				-Lthird_party_root/lib -lc \
				-static # --verbose

CXXFLAGS_FOR_TEST= \
				   $(CLANG_WARNINGS) $(CXX_STD) -g -stdlib=libc++ \
				   -DLIUMOS_TEST

OSX_STD_HEADER_INCLUDE_FLAGS= \
							  -I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include \
							  -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include

FONTMAKER=../tools/fontmaker

$(FONTMAKER) :
	make -C $(dir $(FONTMAKER)) fontmaker

font.gen.cc : font.txt $(FONTMAKER)
	$(FONTMAKER) font.txt > font.gen.cc

# Test rules
test_% : %_test.cc Makefile
	$(CXX) $(CXXFLAGS_FOR_TEST) -o $*_test.bin $*_test.cc
	@./$*_test.bin

test_sheet : sheet_test.cc sheet.cc Makefile
	$(CXX) $(CXXFLAGS_FOR_TEST) -o sheet_test.bin sheet_test.cc sheet.cc asm.S
	@./sheet_test.bin

# Loader rules

%.o : %.c Makefile
	$(LLVM_CC) $(CFLAGS_LOADER) \
		-c -o $*.o $*.c
%.o.d : %.c Makefile
	@$(LLVM_CC) $(CXXFLAGS_LOADER) -MD -MF $@ -c -o $*.o $*.c >/dev/null 2>&1
	@echo 'DEP $@'

%.o : %.cc Makefile
	$(LLVM_CXX) $(CXXFLAGS_LOADER) \
		-c -o $*.o $*.cc
%.o.d : %.cc Makefile
	@$(LLVM_CXX) $(CXXFLAGS_LOADER) -MD -MF $@ -c -o $*.o $*.cc >/dev/null 2>&1
	@echo 'DEP $@'

%.o : %.S Makefile
	$(LLVM_CC) $(CFLAGS_LOADER) \
		-c -o $*.o $*.S
%.o.d : %.S Makefile
	@touch $@ # generate dummy
	@echo 'DEP $@'

%.o.dump.txt : %.o Makefile
	objdump -d $*.o > $@

BOOTX64.EFI : $(LOADER_OBJS) Makefile
	$(LLVM_LLD_LINK) \
		-subsystem:efi_application -nodefaultlib \
		-entry:efi_main $(LOADER_OBJS) -out:BOOTX64.EFI

# Kernel rules

%.elf.o : %.cc Makefile
	$(LLVM_CXX) $(CXXFLAGS_KERNEL) \
		-g -c -o $*.elf.o $*.cc
%.elf.d : %.cc Makefile
	@$(LLVM_CXX) $(CXXFLAGS_KERNEL) -MD -MF $@ -c -o $*.elf.o $*.cc >/dev/null 2>&1
	@echo 'DEP $@'

%.elf.o : %.c Makefile
	$(LLVM_CC) $(CFLAGS_KERNEL) \
		-g -c -o $*.elf.o $*.c
%.elf.d : %.c Makefile
	@$(LLVM_CXX) $(CXXFLAGS_KERNEL) -MD -MF $@ -c -o $*.elf.o $*.c >/dev/null 2>&1
	@echo 'DEP $@'

%.elf.o : %.S Makefile
	$(LLVM_CC) $(CXXFLAGS_KERNEL) \
		-c -o $*.elf.o $*.S
%.elf.d : %.S Makefile
	@touch $@ # generate dummy
	@echo 'DEP $@'

LIUMOS.ELF : $(KERNEL_OBJS) kernel.ld Makefile
	$(LLVM_LD_LLD) $(LDFLAGS_KERNEL) \
		-o LIUMOS.ELF -e KernelEntry -T kernel.ld $(KERNEL_OBJS)

newlib:
	CC=$(LLVM_CC) AR=$(LLVM_AR) RANLIB=$(LLVM_RANLIB) ./build_newlib.sh

libcxxabi:
	CC=$(LLVM_CC) \
	   CXX=$(LLVM_CXX) \
	   LD_LLD=$(LLVM_LD_LLD) \
	   AR=$(LLVM_AR) \
	   RANLIB=$(LLVM_RANLIB) \
	   ./build_libcxxabi.sh

libcxx:
	CC=$(LLVM_CC) \
	   CXX=$(LLVM_CXX) \
	   LD_LLD=$(LLVM_LD_LLD) \
	   AR=$(LLVM_AR) \
	   RANLIB=$(LLVM_RANLIB) \
	   ./build_libcxx.sh

run:
	make -C .. run

unittest:
	make test_ring_buffer
	make test_paging
	make test_xhci_trbring
	make test_sheet

clean :
	-rm *.EFI
	-rm *.lib
	-rm *.o
	-rm *.d
	-rm *.gen.c
	-rm *.gen.cc
	-rm *.bin
	-rm githash.cc

clean_third_party :
	-rm -rf third_party
	-rm -rf third_party_build
	-rm -rf third_party_root


format :
	clang-format -i *.cc *.h

commit :
	make -C .. commit

sym : LIUMOS.ELF
	/usr/local/opt/binutils/bin/gobjdump -t LIUMOS.ELF | more

print_cc :
	@echo "$(LLVM_CC)"

dump : $(addsuffix .dump.txt, $(OBJS))

kerneldump :
	/usr/local/opt/llvm/bin/llvm-objdump --x86-asm-syntax=intel -d LIUMOS.ELF | more

FORCE : 

githash.cc: FORCE
	./update_githash_cc.sh

-include $(LOADER_DEPS)
-include $(KERNEL_DEPS)
