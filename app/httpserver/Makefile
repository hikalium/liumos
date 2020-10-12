NAME=httpserver
TARGET=$(NAME).bin
TARGET_OBJS=$(NAME).o

TARGET_STDLIB=$(NAME)_stdlib.bin

default: $(TARGET)

include ../liumlib/common.mk

$(TARGET_STDLIB): $(NAME)_stdlib.c
	$(LLVM_CC) $(NAME)_stdlib.c -o $(TARGET_STDLIB)

all: $(TARGET) $(TARGET_STDLIB)
