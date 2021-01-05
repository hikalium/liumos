default: default_parallel

include ../common.mk

MAKE_ARGS=-j $(LIUM_NCPU) -f Makefile.targets

default_parallel:
	make $(MAKE_ARGS)

CLEAN_TARGETS= \
	*.EFI \
	*.lib \
	*.o \
	*.d \
	*.gen.c \
	*.gen.cc \
	*.bin \
	githash.cc

clean :
	rm $(CLEAN_TARGETS) >/dev/null 2>&1 ; true

test:
	make $(MAKE_ARGS) unittest

format :
	clang-format -i *.cc *.h

test_% : %_test.cc Makefile
	make $(MAKE_ARGS) $@
