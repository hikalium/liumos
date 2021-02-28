default: build

include ../common.mk

.PHONY: build clean

build :
	cargo build -vv --release

clean:
	-rm -r target

install: build
	cp target/x86_64-none-efi/release/loader.efi ../mnt/EFI/BOOT/BOOTX64.EFI

check:
	cargo clippy

test:
	cargo test --verbose

watch_serial:
		while ! telnet localhost 1235 ; do sleep 1 ; done ;
