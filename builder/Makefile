CIRCLECI_DOCKER_VERSION := $(shell cat ../.circleci/config.yml | grep image | grep liumos-builder | cut -d ':' -f 2- | tr -d ' ')

run_ci_version:
	docker run -it $(CIRCLECI_DOCKER_VERSION)
