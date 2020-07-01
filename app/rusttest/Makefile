TARGET_BIN=target/x86_64-unknown-elf/release/rusttest
default: $(TARGET_BIN)
.FORCE : 
$(TARGET_BIN) : .FORCE
	cargo xbuild --target=x86_64-unknown-elf.json --release
install: $(TARGET_BIN)
	cp $(TARGET_BIN) ../../mnt/
clean:
	-rm -r target
