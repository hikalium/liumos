TARGET=browser
OBJS=browser.o rendering.o malloc.o syscall.o

default: $(TARGET)

#include ../../common.mk
LLVM_CC:=/usr/local/opt/llvm/bin/clang
LLVM_CXX:=/usr/local/opt/llvm/bin/clang++
LLVM_LLD_LINK:=/usr/local/opt/llvm/bin/lld-link
LLVM_LD_LLD:=/usr/local/opt/llvm/bin/ld.lld
LLVM_AR:=/usr/local/opt/llvm/bin/llvm-ar
LLVM_RANLIB:=/usr/local/opt/llvm/bin/llvm-ranlib

test: make
	./test.sh

%.o : %.c Makefile
	$(LLVM_CC) -target x86_64-unknown-none-elf -static -nostdlib -fPIE -B/usr/local/opt/llvm/bin/ -c -o $@ $*.c

%.o : %.S Makefile
	$(LLVM_CC) -target x86_64-unknown-none-elf -static -nostdlib -fPIE -B/usr/local/opt/llvm/bin/ -c -o $@ $*.S

$(TARGET) : $(OBJS) Makefile
	$(LLVM_LD_LLD) -static --no-rosegment -e main -o $@ $(OBJS)
