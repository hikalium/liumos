TARGET_SERVER=http_server.bin
TARGET_CLIENT=http_client.bin
TARGET_SERVER_STDLIB=http_server_stdlib.bin
TARGET_CLIENT_STDLIB=http_client_stdlib.bin
OBJS_SERVER=server.o lib.o syscall.o ../liumlib/syscall.o ../liumlib/entry.o
OBJS_CLIENT=client.o lib.o syscall.o ../liumlib/syscall.o ../liumlib/entry.o
SERVER_STDLIB=server_stdlib.c
CLIENT_STDLIB=client_stdlib.c

default: all

include ../liumlib/common.mk

%.o : %.c Makefile
	$(LLVM_CC) -target x86_64-unknown-none-elf -static -nostdlib -fPIE -I$(CLANG_SYSTEM_INC_PATH) -I../../src/third_party_root/include/ -B/usr/local/opt/llvm/bin/ -c -o $@ $*.c

%.o : %.S Makefile
	$(LLVM_CC) -target x86_64-unknown-none-elf -static -nostdlib -fPIE -B/usr/local/opt/llvm/bin/ -c -o $@ $*.S

$(TARGET_SERVER): $(OBJS_SERVER) Makefile lib.h
	$(LLVM_LD_LLD) -static --no-rosegment -e entry -o $@ $(OBJS_SERVER)

$(TARGET_CLIENT): $(OBJS_CLIENT) Makefile lib.h
	$(LLVM_LD_LLD) -static --no-rosegment -e entry -o $@ $(OBJS_CLIENT)

$(TARGET_SERVER_STDLIB): $(SERVER_STDLIB)
	$(LLVM_CC) $(SERVER_STDLIB) -o $(TARGET_SERVER_STDLIB)

$(TARGET_CLIENT_STDLIB): $(CLIENT_STDLIB)
	$(LLVM_CC) $(CLIENT_STDLIB) -o $(TARGET_CLIENT_STDLIB)

all: $(TARGET_SERVER) $(TARGET_CLIENT)

all_on_linux: $(TARGET_SERVER) $(TARGET_CLIENT) $(TARGET_CLIENT_STDLIB) $(TARGET_SERVER_STDLIB)

run_on_linux: all
	sudo strace ./$(TARGET_SERVER_STDLIB)
	sudo strace ./$(TARGET_CLIENT_STDLIB)

clean:
	rm *.o
	rm *.bin
