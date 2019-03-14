default: pi.bin

pi.bin : pi.c pi.S Makefile
	/usr/local/opt/musl-cross/bin/x86_64-linux-musl-gcc -static -nostdlib -fPIE -o $@ -emain pi.c pi.S
