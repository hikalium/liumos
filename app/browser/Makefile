NAME=browser
TARGET=$(NAME).bin
TARGET_OBJS=browser.o rendering.o tokenize.o dom.o

default: $(TARGET)

include ../liumlib/common.mk

test: make
	./test.sh
