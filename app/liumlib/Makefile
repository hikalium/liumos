default: \
	target/x86_64-unknown-elf/release/liumlib.rlib \
	# dummy line

include ../../common.mk

target/x86_64-unknown-elf/release/liumlib.rlib : .FORCE
	export LLVM_CC="$(LLVM_CC)" && \
		export LLVM_AR="$(LLVM_AR)" && \
		cargo build \
		-vv --target=x86_64-unknown-elf.json --release \
		-Z build-std=core,alloc \
		-Z build-std-features=compiler-builtins-mem
	ls -lah target/x86_64-unknown-elf/release/libliumlib.rlib

test.bin: test.c liumlib.c liumlib.h Makefile
	$(CC) -o $@ test.c liumlib.c

.PHONY : test .FORCE

test: test.bin
	./test.bin

clippy :
	export LLVM_CC="$(LLVM_CC)" && \
		export LLVM_AR="$(LLVM_AR)" && \
		cargo clippy \
		-vv --target=x86_64-unknown-elf.json --release \
		-Z build-std=core,alloc \
		-Z build-std-features=compiler-builtins-mem
