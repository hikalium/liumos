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

clean_third_party :
	-rm -rf *.gen.cc
	-rm -rf third_party
	-rm -rf third_party_build
	-rm -rf third_party_root

test:
	make $(MAKE_ARGS) unittest

format :
	clang-format -i *.cc *.h

newlib:
	CC=$(LLVM_CC) AR=$(LLVM_AR) RANLIB=$(LLVM_RANLIB) ./build_newlib.sh

libcxxabi:
	CC=$(LLVM_CC) \
	   CXX=$(LLVM_CXX) \
	   LD_LLD=$(LLVM_LD_LLD) \
	   AR=$(LLVM_AR) \
	   RANLIB=$(LLVM_RANLIB) \
	   ./build_libcxxabi.sh

libcxx:
	CC=$(LLVM_CC) \
	   CXX=$(LLVM_CXX) \
	   LD_LLD=$(LLVM_LD_LLD) \
	   AR=$(LLVM_AR) \
	   RANLIB=$(LLVM_RANLIB) \
	   ./build_libcxx.sh

test_% : %_test.cc Makefile
	make $(MAKE_ARGS) $@
