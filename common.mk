include cc_cache.gen.mk

THIS_DIR:=$(dir $(lastword $(MAKEFILE_LIST)))
OSNAME=${shell uname -s}
CLANG_SYSTEM_INC_PATH=$(shell $(THIS_DIR)./scripts/get_clang_builtin_include_dir.sh)

ifeq ($(OSNAME),Darwin)
cc_cache.gen.mk : Makefile
	@ echo "" > $@ && \
		LLVM_PREFIX=`brew --prefix llvm` && \
		echo "LLVM_CC:=$$LLVM_PREFIX/bin/clang" >> $@ && \
		echo "LLVM_CXX:=$$LLVM_PREFIX/bin/clang++" >> $@ && \
		echo "LLVM_LLD_LINK:=$$LLVM_PREFIX/bin/lld-link" >> $@ && \
		echo "LLVM_LD_LLD:=$$LLVM_PREFIX/bin/ld.lld" >> $@
	cat $@
else
cc_cache.gen.mk : Makefile
	@ echo "" > $@ && \
		echo "LLVM_CC:=`which clang`" >> $@ && \
		echo "LLVM_CXX:=`which clang++`" >> $@ && \
		echo "LLVM_LLD_LINK:=`which lld-link-4.0`" >> $@
	cat $@
endif

