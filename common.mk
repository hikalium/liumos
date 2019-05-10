include cc_cache.gen.mk
OSNAME=${shell uname -s}
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

