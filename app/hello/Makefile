default: hello.bin

hello.bin : hello.c hello.S Makefile
	/usr/local/opt/musl-cross/bin/x86_64-linux-musl-gcc -static -nostdlib -fPIE -o $@ -emain hello.c hello.S
