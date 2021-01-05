#!/bin/bash -e
cd `dirname $0`
NEWLIB_NAME=newlib-3.1.0

THIRD_PARTY_DIR=`pwd`
NEWLIB_BUILD_DIR=${THIRD_PARTY_DIR}/build/${NEWLIB_NAME}
NEWLIB_SRC_DIR=${THIRD_PARTY_DIR}/src/${NEWLIB_NAME}
BUILD_ROOT=${THIRD_PARTY_DIR}/out/root_for_kernel

echo NEWLIB_BUILD_DIR=${NEWLIB_BUILD_DIR}
echo NEWLIB_SRC_DIR=${NEWLIB_SRC_DIR}
echo BUILD_ROOT=${BUILD_ROOT}

mkdir -p src
wget -N http://sourceware.org/pub/newlib/${NEWLIB_NAME}.tar.gz -P src/
tar -xvf src/${NEWLIB_NAME}.tar.gz -C src/

echo CC=${CC}
echo AR=${AR}
echo RANLIB=${RANLIB}
echo CFLAGS=${RANLIB}
mkdir -p ${NEWLIB_BUILD_DIR}
cd ${NEWLIB_BUILD_DIR}
${NEWLIB_SRC_DIR}/newlib/configure \
--target=x86_64-elf --disable-multilib \
--prefix=${BUILD_ROOT} \
CFLAGS="-g -nostdlibinc -O2 -target x86_64-unknown-none-elf -fPIC -mcmodel=large"
make clean
make -j4 install
