#!/bin/bash -e
cd `dirname $0`

PWD=`pwd`
LIUMOS_THIRDPARTY_PATH=`pwd`/third_party
INSTALL_PREFIX=`pwd`/third_party_root
NEWLIB_INC_PATH=${INSTALL_PREFIX}/include
LLVM_PROJ_PATH=${LIUMOS_THIRDPARTY_PATH}/llvm-project-llvmorg-8.0.1
LIBCXX_INC_PATH=$LLVM_PROJ_PATH/libcxx/include
BUILD_DIR=${PWD}/third_party_build/llvm-project-llvmorg-8.0.1/libcxx
SRC_DIR=${PWD}/third_party/llvm-project-llvmorg-8.0.1/libcxx

echo BUILD_DIR=${BUILD_DIR}
echo SRC_DIR=${SRC_DIR}

echo CC=${CC}
echo CXX=${CXX}
echo AR=${AR}
echo RANLIB=${RANLIB}
echo LD_LLD=${LD_LLD}

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

rm -f CMakeCache.txt
cmake \
	-DCMAKE_AR=${AR} \
	-DCMAKE_CXX_COMPILER=${CXX} \
	-DCMAKE_CXX_COMPILER_TARGET=x86_64-unknown-none-elf \
	-DCMAKE_CXX_FLAGS="-v -I${NEWLIB_INC_PATH} -D__ELF__ -D_LIBUNWIND_IS_BAREMETAL=1 -D_LDBL_EQ_DBL -U__APPLE__ -D_LIBCPP_HAS_NO_LIBRARY_ALIGNED_ALLOCATION -D_GNU_SOURCE -D_POSIX_TIMERS -mcmodel=large" \
	-DCMAKE_C_COMPILER=${CC} \
	-DCMAKE_C_FLAGS="-I${NEWLIB_INC_PATH} -fdeclspec -D__ELF__ -D_LIBUNWIND_IS_BAREMETAL=1 -D_LDBL_EQ_DBL -U__APPLE__ -D_LIBCPP_HAS_NO_LIBRARY_ALIGNED_ALLOCATION -D_GNU_SOURCE -D_POSIX_TIMERS -mcmodel=large" \
	-DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
	-DCMAKE_LINKER=${LD_LLD} \
	-DCMAKE_RANLIB=${RANLIB} \
	-DCMAKE_SHARED_LINKER_FLAGS="-L${INSTALL_PREFIX}/lib" \
	-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
	-DLIBCXX_CXX_ABI=libcxxabi \
	-DLIBCXX_CXX_ABI_INCLUDE_PATHS="${INSTALL_PREFIX}/include" \
	-DLIBCXX_CXX_ABI_LIBRARY_PATH="${INSTALL_PREFIX}/lib" \
	-DLIBCXX_ENABLE_FILESYSTEM=False \
	-DLIBCXX_ENABLE_MONOTONIC_CLOCK=False \
	-DLIBCXX_ENABLE_RTTI=False \
	-DLIBCXX_ENABLE_SHARED=False \
	-DLIBCXX_ENABLE_STATIC=True \
	-DLIBCXX_ENABLE_THREADS=False \
	-DLIBCXX_ENABLE_EXCEPTIONS=False \
	-DLLVM_PATH=${LLVM_PROJ_PATH} \
	${SRC_DIR} 
make clean
make -j8 install
