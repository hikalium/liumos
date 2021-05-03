default: \
	target/x86_64-unknown-elf/release/liumlib.rlib \
	# dummy line

include ../../common.mk

CARGO:= export GIT_HASH='$(shell ./githash.sh)' && \
	export LLVM_CC="$(LLVM_CC)" && \
	export LLVM_AR="$(LLVM_AR)" && \
	cargo

CARGO_ARGS:= -vv --target=x86_64-unknown-elf.json --release \
		-Z build-std=core,alloc \
		-Z build-std-features=compiler-builtins-mem

target/x86_64-unknown-elf/release/liumlib.rlib : .FORCE
		$(CARGO) build $(CARGO_ARGS)
	ls -lah target/x86_64-unknown-elf/release/libliumlib.rlib

test.bin: test.c liumlib.c liumlib.h Makefile
	$(CC) -o $@ test.c liumlib.c

.PHONY : test .FORCE

test: test.bin
	./test.bin

clippy :
		$(CARGO) clippy $(CARGO_ARGS)

doc :
		$(CARGO) doc --open $(CARGO_ARGS)
