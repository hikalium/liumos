include ../../common.mk

OBJS=$(TARGET_OBJS) ../liumlib/liumlib.o ../liumlib/syscall.o ../liumlib/entry.o
LIUMLIB_DEPS=../liumlib/liumlib.h

%.o : %.c Makefile $(TARGET_DEPS) $(LIUMLIB_DEPS)
	$(LLVM_CC) -target x86_64-unknown-none-elf -static -nostdlib -nostdinc -Wno-macro-redefined -fPIE -I$(CLANG_SYSTEM_INC_PATH) -c -o $@ $*.c

%.o : %.S Makefile $(TARGET_DEPS) $(LIUMLIB_DEPS)
	$(LLVM_CC) -target x86_64-unknown-none-elf -static -nostdlib -nostdinc -Wno-macro-redefined -fPIE -c -o $@ $*.S

$(TARGET) : $(OBJS) Makefile $(TARGET_DEPS)
	$(LLVM_LD_LLD) -static --no-rosegment -e entry -o $@ $(OBJS)

.PHONY :  clean format test

clean:
	rm *.o *.bin ; true

format :
	-clang-format -i *.c

# Each apps should implement following targets:
# test
