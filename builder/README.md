# liumos-builder

Dockerfile for building & testing liumOS.

```
docker build .
docker build --no-cache .
docker run -it <image id>
docker tag <image id> hikalium/liumos-builder:latest
docker push hikalium/liumos-builder:latest
```
