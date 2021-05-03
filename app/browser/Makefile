NAME=browser
TARGET=$(NAME).bin
TARGET_OBJS=browser.o rendering.o tokenize.o parse.o
TARGET_DEPS=parse.h rendering.h tokenize.h

default: $(TARGET)

include ../liumlib/common.mk

test:
	make
	# ./test.sh
