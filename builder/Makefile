CIRCLECI_DOCKER_VERSION := $(shell cat ../.circleci/config.yml | grep image | grep liumos-builder | cut -d ':' -f 2- | tr -d ' ')

IMAGE_TAG:=hikalium/liumos-builder:v$(shell date +%Y%m%d_%H%M%S)

build:
	docker build -t ${IMAGE_TAG} .
	@ echo
	@ echo ${IMAGE_TAG}
	@ echo Run \"docker push ${IMAGE_TAG}\" to upload this image.
	@ echo Also, please make sure to update .circleci/config.yml.

run_ci_version:
	docker run -it $(CIRCLECI_DOCKER_VERSION)
