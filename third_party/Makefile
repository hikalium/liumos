default: freetype

include ../common.mk

freetype:
	CC="$(LLVM_CC)" \
	   LLD="$(LLVM_LD_LLD)" \
	   AR="$(LLVM_AR)" \
	   RANLIB="$(LLVM_RANLIB)" \
	   ./build_freetype.sh
