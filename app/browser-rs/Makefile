TARGET=browser-rs
TARGET_BIN=$(TARGET).bin
default: $(TARGET_BIN)

include ../../common.mk

CARGO:= export GIT_HASH='$(shell ../shelium/githash.sh)' && \
	export LLVM_CC="$(LLVM_CC)" && \
	export LLVM_AR="$(LLVM_AR)" && \
	cargo

CARGO_ARGS:= -vv --target=../liumlib/x86_64-unknown-elf.json --release \
		-Z build-std=core,alloc \
		-Z build-std-features=compiler-builtins-mem

$(TARGET_BIN) : .FORCE
	$(CARGO) build $(CARGO_ARGS) --bin browser-rs
	cp target/x86_64-unknown-elf/release/$(TARGET) $(TARGET_BIN)

.PHONY : install clean dump clippy .FORCE

install: $(TARGET_BIN)
clean:
	-rm -r target
	-rm *.bin
dump : $(TARGET_BIN)
	objdump -d target/x86_64-unknown-elf/release/$(TARGET)
clippy :
	$(CARGO) clippy $(CARGO_ARGS)

ifeq ($(OSNAME),Darwin)
test :
	echo "Skipping test for $(PWD) because we can't run the binary on this host." ;
else
test :
	$(CARGO) test $(CARGO_ARGS) --tests -- --test-threads=1 --nocapture ;
endif

test_dom :
	$(CARGO) test $(CARGO_ARGS) --test dom -- --test-threads=1 --nocapture ;

test_tokenizer :
	$(CARGO) test $(CARGO_ARGS) --test tokenizer -- --test-threads=1 --nocapture ;

test_css :
	$(CARGO) test $(CARGO_ARGS) --test css -- --test-threads=1 --nocapture ;
