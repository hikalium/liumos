NAME=guitest
TARGET=$(NAME).bin
TARGET_OBJS=$(NAME).o freetype_wrapper.o source_code_pro_medium_ttf.o koruri_regular_subset_ttf.o

default: $(TARGET)

include ../../common.mk

OBJS=$(TARGET_OBJS) \
	 ../liumlib/liumlib.o ../liumlib/syscall.o ../liumlib/entry.o ../liumlib/setjmp.o \
	 freetype-2.10.4/objs/autofit.o \
	 freetype-2.10.4/objs/bdf.o \
	 freetype-2.10.4/objs/cff.o \
	 freetype-2.10.4/objs/ftbase.o \
	 freetype-2.10.4/objs/ftbbox.o \
	 freetype-2.10.4/objs/ftbdf.o \
	 freetype-2.10.4/objs/ftbitmap.o \
	 freetype-2.10.4/objs/ftbzip2.o \
	 freetype-2.10.4/objs/ftcache.o \
	 freetype-2.10.4/objs/ftcid.o \
	 freetype-2.10.4/objs/ftdebug.o \
	 freetype-2.10.4/objs/ftfstype.o \
	 freetype-2.10.4/objs/ftgasp.o \
	 freetype-2.10.4/objs/ftglyph.o \
	 freetype-2.10.4/objs/ftgxval.o \
	 freetype-2.10.4/objs/ftgzip.o \
	 freetype-2.10.4/objs/ftinit.o \
	 freetype-2.10.4/objs/ftlzw.o \
	 freetype-2.10.4/objs/ftmm.o \
	 freetype-2.10.4/objs/ftotval.o \
	 freetype-2.10.4/objs/ftpatent.o \
	 freetype-2.10.4/objs/ftpfr.o \
	 freetype-2.10.4/objs/ftstroke.o \
	 freetype-2.10.4/objs/ftsynth.o \
	 freetype-2.10.4/objs/ftsystem.o \
	 freetype-2.10.4/objs/fttype1.o \
	 freetype-2.10.4/objs/ftwinfnt.o \
	 freetype-2.10.4/objs/pcf.o \
	 freetype-2.10.4/objs/pfr.o \
	 freetype-2.10.4/objs/psaux.o \
	 freetype-2.10.4/objs/pshinter.o \
	 freetype-2.10.4/objs/psnames.o \
	 freetype-2.10.4/objs/raster.o \
	 freetype-2.10.4/objs/sfnt.o \
	 freetype-2.10.4/objs/smooth.o \
	 freetype-2.10.4/objs/truetype.o \
	 freetype-2.10.4/objs/type1.o \
	 freetype-2.10.4/objs/type1cid.o \
	 freetype-2.10.4/objs/type42.o \
	 freetype-2.10.4/objs/winfnt.o

%.o : %.c Makefile
	$(LLVM_CC) -target x86_64-unknown-none-elf -static -nostdlib \
		-I$(CLANG_SYSTEM_INC_PATH) \
		-I../../src/third_party_root/include/ \
		-fno-builtin \
		-c -o $@ $*.c

freetype_wrapper.o : freetype_wrapper.c Makefile
	$(LLVM_CC) -target x86_64-unknown-none-elf -static -nostdlib \
		-I$(CLANG_SYSTEM_INC_PATH) \
		-Ifreetype-2.10.4/include \
		-Ifreetype_support/include/ \
		-Wno-macro-redefined \
		-fno-builtin \
		-c -o $@ freetype_wrapper.c

%.o : %.S Makefile
	$(LLVM_CC) -target x86_64-unknown-none-elf -static -nostdlib -mcmodel=small -B/usr/local/opt/llvm/bin/ -c -o $@ $*.S

$(TARGET) : $(OBJS) Makefile
	$(LLVM_LD_LLD) -static \
		--no-rosegment -e entry \
		-o $@ \
		$(OBJS)

clean:
	rm *.o *.bin ; true

format :
	-clang-format -i *.c
