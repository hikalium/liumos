default:
	make newlib
	make libcxxabi
	make libcxx
	make freetype

include ../common.mk

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

freetype:
	CC="$(LLVM_CC)" \
	   LLD="$(LLVM_LD_LLD)" \
	   AR="$(LLVM_AR)" \
	   RANLIB="$(LLVM_RANLIB)" \
	   ./build_freetype.sh

clean:
	-rm -rf ./build ./out

clean_full: clean
	-rm -rf ./src
