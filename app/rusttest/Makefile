TARGET_BIN=target/x86_64-unknown-elf/release/rusttest
default: $(TARGET_BIN)

include ../../common.mk

.FORCE : 
$(TARGET_BIN) : .FORCE
	export LLVM_CC="$(LLVM_CC)" && \
		export LLVM_AR="$(LLVM_AR)" && \
		cargo xbuild -vv --target=x86_64-unknown-elf.json --release
install: $(TARGET_BIN)
	cp $(TARGET_BIN) ../../mnt/
clean:
	-rm -r target
dump: $(TARGET_BIN)
	objdump -d target/x86_64-unknown-elf/release/rusttest
