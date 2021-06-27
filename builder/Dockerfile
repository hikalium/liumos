FROM ubuntu:20.04
# Use bash
SHELL ["/bin/bash", "-c"]
# To avoid being asked about tzdata
ARG DEBIAN_FRONTEND=noninteractive
# Install dependencies
# gcc-multilib for bits/libc-header-start.h
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
	build-essential \
	ca-certificates \
	clang \
	clang-8 \
	clang-format \
	cmake \
	curl \
	gcc \
	git \
	libc++-8-dev \
	libc++abi-8-dev \
	lld \
	lld-8 \
	llvm-8-tools \
	make \
	netcat \
	qemu-system-x86 \
	qemu-utils \
	rsync \
	wget \
	gcc-multilib \
	python3-pip \
	telnet
# Install rust toolchain
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | \
	sh -s -- -y --default-toolchain=nightly-x86_64-unknown-linux-gnu
ENV PATH $PATH:/root/.cargo/bin
ARG RUST_VERSION_DATE=2021-06-27
RUN rustup toolchain install nightly-${RUST_VERSION_DATE} && rustup default nightly-${RUST_VERSION_DATE}
RUN rustup component add rust-src

RUN pip3 install pexpect

# Finalize the workdir and prompt
WORKDIR /liumos/
RUN echo 'export PS1="(liumos-builder)$ "' >> /root/.bashrc

CMD ["/bin/bash"]
