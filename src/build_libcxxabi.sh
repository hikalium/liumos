#!/bin/bash -e
cd `dirname $0`

# Download and extract llvm sources
ARCHIVE_VERSION=8.0.1
ARCHIVE_URL=https://github.com/llvm/llvm-project/archive/llvmorg-${ARCHIVE_VERSION}.tar.gz
wget -N ${ARCHIVE_URL} -P third_party/
tar -xvf third_party/llvmorg-${ARCHIVE_VERSION}.tar.gz -C third_party/

PWD=`pwd`
LIUMOS_THIRDPARTY_PATH=`pwd`/third_party
INSTALL_PREFIX=`pwd`/third_party_root
BUILD_DIR=${PWD}/third_party_build/llvm-project-llvmorg-8.0.1/libcxxabi
SRC_DIR=${PWD}/third_party/llvm-project-llvmorg-8.0.1/libcxxabi
NEWLIB_INC_PATH=${INSTALL_PREFIX}/include
LLVM_PROJ_PATH=${LIUMOS_THIRDPARTY_PATH}/llvm-project-llvmorg-8.0.1
LIBCXX_INC_PATH=$LLVM_PROJ_PATH/libcxx/include

echo BUILD_DIR=${BUILD_DIR}
echo SRC_DIR=${SRC_DIR}

echo CC=${CC}
echo CXX=${CXX}
echo AR=${AR}
echo RANLIB=${RANLIB}
echo LD_LLD=${LD_LLD}

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

COMMON_COMPILER_FLAGS="\
	-nostdlibinc -I$NEWLIB_INC_PATH \
	-fdeclspec -mcmodel=large \
	-D__ELF__ -U__APPLE__ \
	-D_LDBL_EQ_DBL -D_LIBCPP_HAS_NO_THREADS -D_LIBCPP_HAS_NO_LIBRARY_ALIGNED_ALLOCATION \
	" 
rm -f CMakeCache.txt
cmake \
	-DCMAKE_AR=${AR} \
	-DCMAKE_CXX_COMPILER=${CXX} \
	-DCMAKE_CXX_FLAGS="$COMMON_COMPILER_FLAGS" \
	-DCMAKE_C_COMPILER=${CC} \
	-DCMAKE_C_FLAGS="$COMMON_COMPILER_FLAGS" \
	-DCMAKE_LINKER=${LD_LLD} \
	-DCMAKE_RANLIB=${RANLIB} \
	-DCMAKE_SHARED_LINKER_FLAGS="-L${INSTALL_PREFIX}/lib" \
	-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
	-DLIBCXXABI_LIBCXX_INCLUDES="$LIBCXX_INC_PATH" \
	-DLIBCXXABI_ENABLE_THREADS=OFF \
	-DLIBCXXABI_ENABLE_EXCEPTIONS=OFF \
	-DLIBCXXABI_TARGET_TRIPLE=x86_64-unknown-none-elf \
	-DLIBCXXABI_ENABLE_SHARED=OFF \
	-DLIBCXXABI_ENABLE_STATIC=ON \
	-DLLVM_PATH=${LLVM_PROJ_PATH} \
	-DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
	-DLIBCXXABI_USE_LLVM_UNWINDER=ON \
	-DLIBCXXABI_ENABLE_ASSERTIONS=OFF \
	-DLIBCXXABI_ENABLE_NEW_DELETE_DEFINITIONS=OFF \
	-DLIBCXXABI_BAREMETAL=ON \
	${SRC_DIR}
make clean
make -j8 install
