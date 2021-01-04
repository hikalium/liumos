include ../../common.mk

OBJS=$(TARGET_OBJS) ../liumlib/liumlib.o ../liumlib/syscall.o ../liumlib/entry.o

%.o : %.c Makefile $(TARGET_DEPS)
	$(LLVM_CC) -target x86_64-unknown-none-elf -static -nostdlib -fPIE -I$(CLANG_SYSTEM_INC_PATH) -I../../src/third_party_root/include/ -B/usr/local/opt/llvm/bin/ -c -o $@ $*.c

%.o : %.S Makefile $(TARGET_DEPS)
	$(LLVM_CC) -target x86_64-unknown-none-elf -static -nostdlib -fPIE -B/usr/local/opt/llvm/bin/ -c -o $@ $*.S

$(TARGET) : $(OBJS) Makefile $(TARGET_DEPS)
	$(LLVM_LD_LLD) -static --no-rosegment -e entry -o $@ $(OBJS)

clean:
	rm *.o *.bin ; true

format :
	-clang-format -i *.c
