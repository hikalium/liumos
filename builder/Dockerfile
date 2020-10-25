FROM ubuntu:20.04
# Use bash
SHELL ["/bin/bash", "-c"]
# To avoid being asked about tzdata
ARG DEBIAN_FRONTEND=noninteractive
# Install dependencies
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
	wget
# Install rust toolchain
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | \
	sh -s -- -y --default-toolchain=nightly
ENV PATH $PATH:/root/.cargo/bin
RUN rustup component add rust-src && \
	cargo install cargo-xbuild

WORKDIR /prebuilt
RUN git clone https://github.com/hikalium/liumos.git
WORKDIR /prebuilt/liumos
RUN make tools
WORKDIR /prebuilt/liumos/src
RUN make newlib && make libcxxabi && make libcxx

# Ensure liumOS is buildable
WORKDIR /prebuilt/liumos
RUN make
RUN make -C app/rusttest

CMD ["/bin/bash"]
