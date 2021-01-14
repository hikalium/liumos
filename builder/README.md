# liumos-builder

Dockerfile for building & testing liumOS.

```
docker build .
docker build --no-cache .
docker run -it <image id>

export IMAGE_TAG=v`date +%Y%m%d_%H%M%S` && echo ${IMAGE_TAG}
docke rtag <image id> hikalium/liumos-builder:${IMAGE_TAG}
docker push hikalium/liumos-builder:${IMAGE_TAG}
```
