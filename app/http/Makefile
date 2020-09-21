TARGET_SERVER=http_server.bin
TARGET_CLIENT=http_client.bin
TARGET_SERVER_STDLIB=http_server_stdlib.bin
TARGET_CLIENT_STDLIB=http_client_stdlib.bin
OBJS_SERVER=server.o lib.o syscall.o
OBJS_CLIENT=client.o lib.o syscall.o
SERVER_STDLIB=server_stdlib.c
CLIENT_STDLIB=client_stdlib.c

default: all

#include ../../common.mk
# for mac
#LLVM_CC:=/usr/local/opt/llvm/bin/clang
#LLVM_CXX:=/usr/local/opt/llvm/bin/clang++
#LLVM_LD_LLD:=/usr/local/opt/llvm/bin/ld.lld

# for ubuntu
LLVM_CC:=/usr/bin/clang
LLVM_CXX:=/usr/bin/clang++
LLVM_LD_LLD:=/usr/bin/ld.lld

#CLANG_SYSTEM_INC_PATH=$(shell $(THIS_DIR)./get_clang_builtin_include_dir.sh $(LLVM_CXX))

%.o : %.c Makefile
	#$(LLVM_CC) -target x86_64-unknown-none-elf -static -nostdlib -fPIE -I$(CLANG_SYSTEM_INC_PATH) -I../../src/third_party_root/include/ -B/usr/local/opt/llvm/bin/ -c -o $@ $*.c
	$(LLVM_CC) -nostdlib -fPIE -I$(CLANG_SYSTEM_INC_PATH) -I../../src/third_party_root/include/ -B/usr/local/opt/llvm/bin/ -g -c -o $@ $*.c

%.o : %.S Makefile
	#$(LLVM_CC) -target x86_64-unknown-none-elf -static -nostdlib -fPIE -B/usr/local/opt/llvm/bin/ -c -o $@ $*.S
	$(LLVM_CC) -static -nostdlib -fPIE -B/usr/local/opt/llvm/bin/ -g -c -o $@ $*.S

all: $(TARGET_SERVER) $(TARGET_CLIENT) $(TARGET_CLIENT_STDLIB) $(TARGET_SERVER_STDLIB)

$(TARGET_SERVER): $(OBJS_SERVER) Makefile lib.h
	$(LLVM_LD_LLD) -static --no-rosegment -e main -o $@ $(OBJS_SERVER)

$(TARGET_CLIENT): $(OBJS_CLIENT) Makefile lib.h
	$(LLVM_LD_LLD) -static --no-rosegment -e main -o $@ $(OBJS_CLIENT)

$(TARGET_SERVER_STDLIB): $(SERVER_STDLIB)
	$(LLVM_CC) $(SERVER_STDLIB) -o $(TARGET_SERVER_STDLIB)

$(TARGET_CLIENT_STDLIB): $(CLIENT_STDLIB)
	$(LLVM_CC) $(CLIENT_STDLIB) -o $(TARGET_CLIENT_STDLIB)

run_on_linux: $(TARGET)
	sudo strace ./$(TARGET_SERVER)
	sudo strace ./$(TARGET_CLIENT)

clean:
	rm *.o
	rm *.bin
