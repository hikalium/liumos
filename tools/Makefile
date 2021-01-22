CC=clang
CFLAGS= -Wall -Wpedantic -Weverything

default: fontmaker 

include ../common.mk

fontmaker: Makefile fontmaker.c
	$(HOST_CC) -o $@ fontmaker.c

clean:
	-rm fontmaker
