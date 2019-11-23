#!/bin/bash -e
cd `dirname $0`
NEWLIB_NAME=newlib-3.1.0

PWD=`pwd`
NEWLIB_BUILD_DIR=${PWD}/third_party_build/${NEWLIB_NAME}
NEWLIB_SRC_DIR=${PWD}/third_party/${NEWLIB_NAME}
BUILD_ROOT=${PWD}/third_party_root/

echo NEWLIB_BUILD_DIR=${NEWLIB_BUILD_DIR}
echo NEWLIB_SRC_DIR=${NEWLIB_SRC_DIR}
echo BUILD_ROOT=${BUILD_ROOT}

wget -N http://sourceware.org/pub/newlib/${NEWLIB_NAME}.tar.gz -P third_party/
tar -xvf third_party/${NEWLIB_NAME}.tar.gz -C third_party/
mkdir -p ${NEWLIB_BUILD_DIR}

echo CC=${CC}
echo AR=${AR}
echo RANLIB=${RANLIB}
echo CFLAGS=${RANLIB}

cd ${NEWLIB_BUILD_DIR}
${NEWLIB_SRC_DIR}/newlib/configure \
--target=x86_64-elf --disable-multilib \
--prefix=${BUILD_ROOT} \
CFLAGS="-nostdlibinc -O2 -target x86_64-unknown-none-elf -mcmodel=large"
make -j4 install
