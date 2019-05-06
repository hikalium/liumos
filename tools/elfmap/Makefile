CXX=clang++
CFLAGS= -std=c++17 -Wall -Wpedantic
TARGET=elfmap

default: $(TARGET)

elf.h :
	wget http://git.musl-libc.org/cgit/musl/plain/include/elf.h

$(TARGET) : elf.h $(TARGET).cc
	$(CXX) $(CFLAGS) -o $@ $(TARGET).cc

clean:
	-rm $(TARGET)
