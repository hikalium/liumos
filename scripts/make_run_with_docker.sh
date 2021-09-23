#!/bin/bash -e
docker kill liumos-builder0 && echo "Killed previous container" || true
cd "$(dirname "$0")"
cd ..
echo "Starting a docker container..."
docker run --rm -ti -v `echo $PWD`:/liumos_host \
	-p 0.0.0.0:2222:2222/tcp \
	-p 0.0.0.0:5905:5905/tcp \
	-p 0.0.0.0:8888:8888/udp \
	-p 0.0.0.0:8889:8889/udp \
	-p 0.0.0.0:1235:1235/tcp \
	--name liumos-builder0 \
	hikalium/liumos-builder:v20210129_021600 \
	/liumos_host/scripts/for_docker/run.sh
