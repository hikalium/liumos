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

format:
	cargo fmt

test:
	cargo test --verbose

commit :
	make -C .. spellcheck
	make format
	make check
	git add .
	../scripts/ensure_objs_are_not_under_git_control.sh
	git diff HEAD --color=always | less -R
	make test
	git commit
