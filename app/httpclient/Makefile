NAME=httpclient
TARGET=$(NAME).bin
TARGET_OBJS=$(NAME).o

TARGET_STDLIB=$(NAME)_stdlib.bin
TARGET_TCP=$(NAME)_tcp.bin

default: $(TARGET)

include ../liumlib/common.mk

$(TARGET_STDLIB): $(NAME)_stdlib.c
	$(LLVM_CC) $(NAME)_stdlib.c -o $(TARGET_STDLIB)

$(TARGET_TCP): $(NAME)_tcp.c
	$(LLVM_CC) $(NAME)_tcp.c -o $(TARGET_TCP)

all: $(TARGET) $(TARGET_STDLIB) $(TARGET_TCP)
