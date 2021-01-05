#!/bin/bash -xe
cd `dirname $0`

FT_VERSION=freetype-2.10.4

TOP_DIR=`pwd`
wget https://download.savannah.gnu.org/releases/freetype/${FT_VERSION}.tar.xz
tar -xvf ${FT_VERSION}.tar.xz
cd ${FT_VERSION}
export CFLAGS="-fno-stack-protector \
	-g -std=c99 --target=x86_64-pc-linux-elf  \
	-fuse-ld=${LLD}  \
	-nostdlib -I${TOP_DIR}/freetype_support/include/"
export CC="${CC}"
export LLD="${LLD}"
export LD="${LLD}"
export AR="${AR}"
export RANLIB="${RANLIB}"
./configure \
	--prefix="${TOP_DIR}/freetype_root" \
	--build=x86_64-pc-linux-gnu \
	--host=x86_64-linuxelf \
	--enable-shared=no \
	--without-zlib
make
make install

