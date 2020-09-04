CC=clang
CFLAGS= -Wall -Wpedantic -Weverything

default: fontmaker 

include ../common.mk

fontmaker: Makefile fontmaker.c
	$(LLVM_CC) -o $@ fontmaker.c

clean:
	-rm fontmaker
