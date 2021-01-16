#!/bin/bash -e
cd `dirname $0`
cd ..
VER=`date +%Y-%m-%d_%H%M%S`
PREBUILT_DIR=prebuilt
PREBUILT_PATH=${PREBUILT_DIR}/liumos_prebuilt_${VER}.tar.gz
echo Generating ${PREBUILT_PATH}...
mkdir -p ${PREBUILT_DIR}
tar -czf ${PREBUILT_PATH} third_party/out
gh release create --title "liumOS build env prebuilt update ${VER} " ${VER} ${PREBUILT_PATH}
