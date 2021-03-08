default: build

include ../qemu.mk

.PHONY: build clean run install check test watch_serial

build :
	cargo build -vv --release

clean:
	-rm -r target

run:
	make common_run_rust

install: build
	cp target/x86_64-unknown-uefi/release/loader.efi ../mnt/EFI/BOOT/BOOTX64.EFI

check:
	cargo clippy

test:
	cargo test --verbose

watch_serial:
		while ! telnet localhost 1235 ; do sleep 1 ; done ;
