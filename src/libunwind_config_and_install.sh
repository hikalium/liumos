#!/bin/bash -x
LIUMOS_THIRDPARTY_PATH=`pwd`/third_party
INSTALL_PREFIX=`pwd`/third_party_root

NEWLIB_INC_PATH=${LIUMOS_THIRDPARTY_PATH}/newlib-cygwin_dest/include

COMMON_COMPILER_FLAGS="\
	-I${LIUMOS_THIRDPARTY_PATH}/newlib-cygwin_dest/include \
	-D__ELF__ -mcmodel=large -U__APPLE__\
	-D_LIBUNWIND_IS_BAREMETAL=1 \
	" 

SRC_ROOT=$LIUMOS_THIRDPARTY_PATH/llvm-project/libunwind
BUILD_ROOT=${LIUMOS_THIRDPARTY_PATH}_build/libunwind
mkdir -p "$BUILD_ROOT" && cd "$BUILD_ROOT" &&
rm CMakeCache.txt ; cmake \
	-DCMAKE_AR=/usr/local/opt/llvm/bin/llvm-ar \
	-DCMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++ \
	-DCMAKE_CXX_FLAGS="${COMMON_COMPILER_FLAGS}" \
	-DCMAKE_C_COMPILER=/usr/local/opt/llvm/bin/clang \
	-DCMAKE_C_FLAGS="${COMMON_COMPILER_FLAGS}" \
	-DCMAKE_LINKER="/usr/local/opt/llvm/bin/ld.lld" \
	-DCMAKE_RANLIB=/usr/local/opt/llvm/bin/llvm-ranlib \
	-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
	\
	-DLIBUNWIND_ENABLE_SHARED=False \
	-DLIBUNWIND_ENABLE_STATIC=True \
	-DLIBUNWIND_ENABLE_THREADS=False \
	-DLIBUNWIND_TARGET_TRIPLE=x86_64-unknown-none-elf \
	\
	-DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
	${SRC_ROOT} && \
make install
