NAME=guitest
TARGET=$(NAME).bin
TARGET_OBJS=$(NAME).o freetype_wrapper.o source_code_pro_medium_ttf.o koruri_regular_subset_ttf.o

default: $(TARGET)

include ../../common.mk

OBJS=$(TARGET_OBJS) \
	 ../liumlib/liumlib.o ../liumlib/syscall.o ../liumlib/entry.o ../liumlib/setjmp.o

%.o : %.c Makefile
	$(LLVM_CC) -target x86_64-unknown-none-elf -static -nostdlib \
		-I$(CLANG_SYSTEM_INC_PATH) \
		-I../../src/third_party_root/include/ \
		-fno-builtin \
		-c -o $@ $*.c

freetype_wrapper.o : freetype_wrapper.c Makefile
	$(LLVM_CC) -target x86_64-unknown-none-elf -static -nostdlib \
		-I$(CLANG_SYSTEM_INC_PATH) \
		-Ifreetype-2.10.4/include \
		-Ifreetype_support/include/ \
		-Wno-macro-redefined \
		-fno-builtin \
		-c -o $@ freetype_wrapper.c

%.o : %.S Makefile
	$(LLVM_CC) -target x86_64-unknown-none-elf -static -nostdlib -B/usr/local/opt/llvm/bin/ -c -o $@ $*.S

$(TARGET) : $(OBJS) Makefile
	$(LLVM_LD_LLD) -static -v \
		--no-rosegment -e entry \
		-Lfreetype_root/lib/ -lfreetype \
		-o $@ \
		$(OBJS)

clean:
	rm *.o *.bin ; true

format :
	-clang-format -i *.c
