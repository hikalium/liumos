#!/bin/bash -x
LIUMOS_THIRDPARTY_PATH=`pwd`/third_party
INSTALL_PREFIX=`pwd`/third_party_root

SRC_ROOT=$LIUMOS_THIRDPARTY_PATH/llvm-project/libunwind
BUILD_ROOT=${LIUMOS_THIRDPARTY_PATH}_build/libunwind
mkdir -p "$BUILD_ROOT" && cd "$BUILD_ROOT" &&
rm CMakeCache.txt ; cmake \
	-DCMAKE_CROSSCOMPILING=True \
	-DCMAKE_SYSTEM_NAME=Generic \
	-DCMAKE_HOST_SYSTEM_NAME=Darwin \
	-DCMAKE_AR=/usr/local/opt/llvm/bin/llvm-ar \
	-DCMAKE_RANLIB=/usr/local/opt/llvm/bin/llvm-ranlib \
	-DCMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++ \
	-DCMAKE_CXX_FLAGS="-I${LIUMOS_THIRDPARTY_PATH}/newlib-cygwin_dest/include -D__ELF__ -D_LIBUNWIND_IS_BAREMETAL=1 -mcmodel=large" \
	-DCMAKE_C_COMPILER=/usr/local/opt/llvm/bin/clang \
	-DCMAKE_C_FLAGS="-I${LIUMOS_THIRDPARTY_PATH}/newlib-cygwin_dest/include -fdeclspec -D__ELF__ -D_LIBUNWIND_IS_BAREMETAL=1 -mcmodel=large" \
	-DCMAKE_LINKER="/usr/local/opt/llvm/bin/ld.lld" \
	-DLIBUNWIND_ENABLE_SHARED=False \
	-DLIBUNWIND_ENABLE_STATIC=True \
	-DLIBUNWIND_ENABLE_THREADS=False \
	-DLIBUNWIND_TARGET_TRIPLE=x86_64-unknown-none-elf \
	-DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
	${SRC_ROOT} && \
make install
