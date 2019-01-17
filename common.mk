include cc_cache.gen.mk
OSNAME=${shell uname -s}
ifeq ($(OSNAME),Darwin)
cc_cache.gen.mk : Makefile
	@LLVM_PREFIX=`brew --prefix llvm` && \
		echo "CC:=$$LLVM_PREFIX/bin/clang" > $@ && \
		echo "LD:=$$LLVM_PREFIX/bin/lld-link" >> $@
	cat $@
else
cc_cache.gen.mk : Makefile
	echo "CC:=`which clang`" > $@ && \
		echo "LD:=`which lld-link-4.0`" >> $@
	cat $@
endif

