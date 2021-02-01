default: build

include ../common.mk

.PHONY: build clean

build :
	cargo build -vv --target=x86_64-none-efi.json --release \
		-Z build-std=core,alloc \
		-Z build-std-features=compiler-builtins-mem
clean:
	-rm -r target

install:
	cp target/x86_64-none-efi/release/loader.efi ../mnt/EFI/BOOT/BOOTX64.EFI

check:
	cargo clippy
