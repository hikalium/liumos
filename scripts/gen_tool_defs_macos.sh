LLVM_PREFIX=`brew --prefix llvm@8`
echo "LLVM_CC:=${LLVM_PREFIX}/bin/clang"
echo "LLVM_CXX:=${LLVM_PREFIX}/bin/clang++"
echo "HOST_CC:=clang"
echo "HOST_CXX:=clang++"
echo "LLVM_LLD_LINK:=${LLVM_PREFIX}/bin/lld-link"
echo "LLVM_LD_LLD:=${LLVM_PREFIX}/bin/ld.lld"
echo "LLVM_AR:=${LLVM_PREFIX}/bin/llvm-ar"
echo "LLVM_RANLIB:=${LLVM_PREFIX}/bin/llvm-ranlib"
echo "LIUM_NCPU:=`sysctl -n hw.logicalcpu`"
