#!/bin/bash -e
PREBUILT_TAG=2021-01-05_220246
PREBUILT_NAME=liumos_prebuilt_${PREBUILT_TAG}.tar.gz
PREBUILT_URL=https://github.com/hikalium/liumos/releases/download/${PREBUILT_TAG}/${PREBUILT_NAME}
cd `dirname $0`
cd ..
wget -N ${PREBUILT_URL} -P prebuilt/
tar -xvf prebuilt/${PREBUILT_NAME}
