.PHONY : clean deploy_release

default : deploy_release

include ../common.mk

clean :
	-rm -rf target/

deploy_release :
	cargo clean --release
	cargo build --release
	cp target/x86_64-liumos_kernel/release/rust_kernel ../mnt/LIUMOSRS.ELF
