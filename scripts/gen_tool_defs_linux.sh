errcho(){ >&2 echo $@; }
EXPECT='clang-8'
FILE=`which ${EXPECT}`
if [ ! -f "${FILE}" ]; then
	errcho "${EXPECT} not found. (Not installed?)"
	errcho "Try 'apt install clang-8 lld-8'"
	return 1
fi
echo "LLVM_CC:=`which clang-8`"
echo "LLVM_CXX:=`which clang++-8`"
echo "LLVM_LLD_LINK:=`which lld-link-8`"
echo "LLVM_LD_LLD:=`which ld.lld-8`"
echo "LLVM_AR:=`which llvm-ar-8`"
echo "LLVM_RANLIB:=`which llvm-ranlib-8`"
echo "LIUM_NCPU:=`nproc --all`"
