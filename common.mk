include cc_cache.gen.mk
OSNAME=${shell uname -s}
ifeq ($(OSNAME),Darwin)
cc_cache.gen.mk : Makefile
	@LLVM_PREFIX=`brew --prefix llvm` && \
		echo "CC:=$$LLVM_PREFIX/bin/clang" > $@ && \
		echo "LLD_LINK:=$$LLVM_PREFIX/bin/lld-link" >> $@ && \
		echo "LD_LLD:=$$LLVM_PREFIX/bin/ld.lld" >> $@
	cat $@
else
cc_cache.gen.mk : Makefile
	echo "CC:=`which clang`" > $@ && \
		echo "LLD_LINK:=`which lld-link-4.0`" >> $@
	cat $@
endif

