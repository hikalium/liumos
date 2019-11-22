version=`/usr/local/opt/llvm/bin/clang --version | head -1 | awk '{print $3}'`
basepath="$(dirname $(dirname /usr/local/opt/llvm/bin/clang))"
echo ${basepath}/lib/clang/${version}/include
