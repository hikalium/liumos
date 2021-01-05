#!/bin/bash -xe
cd `dirname $0`
FT_VERSION=freetype-2.10.4

THIRD_PARTY_DIR=`pwd`

mkdir -p src
wget -N https://download.savannah.gnu.org/releases/freetype/${FT_VERSION}.tar.xz -P src/
tar -xvf src/${FT_VERSION}.tar.xz -C src/

cd src/${FT_VERSION}
export CFLAGS="-fno-stack-protector \
	-g -std=c99 --target=x86_64-pc-linux-elf  \
	-fuse-ld=${LLD}  \
	-nostdlib -I${THIRD_PARTY_DIR}/freetype_support/include/"
export CC="${CC}"
export LLD="${LLD}"
export LD="${LLD}"
export AR="${AR}"
export RANLIB="${RANLIB}"
./configure \
	--prefix="${THIRD_PARTY_DIR}/out/root_for_app" \
	--build=x86_64-pc-linux-gnu \
	--host=x86_64-linuxelf \
	--enable-shared=no \
	--without-zlib
make
make install

