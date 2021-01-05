#!/bin/bash -x
cd `dirname $0`

TOP_DIR=`pwd`
echo pwd = ${TOP_DIR}
echo wget https://download.savannah.gnu.org/releases/freetype/freetype-2.10.4.tar.xz
echo tar -xvf freetype-2.10.4.tar.xz
cd freetype-2.10.4
export CFLAGS="-fno-stack-protector \
	-g -std=c99 --target=x86_64-pc-linux-elf  \
	-fuse-ld=/usr/local/opt/llvm/bin/ld.lld  \
	-nostdlib -I${TOP_DIR}/freetype_support/include/"
export CC='/usr/local/opt/llvm/bin/clang-11'
export LLD='/usr/local/opt/llvm/bin/ld.lld'
export LD='/usr/local/opt/llvm/bin/ld.lld'
export AR='/usr/local/opt/llvm/bin/llvm-ar'
export RANLIB='/usr/local/opt/llvm/bin/llvm-ranlib'
./configure \
	--prefix="${TOP_DIR}/freetype_root" \
	--build=x86_64-pc-linux-gnu \
	--host=x86_64-linuxelf \
	--enable-shared=no \
	--without-zlib
make
make install

