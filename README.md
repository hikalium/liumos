# liumOS


[![CircleCI](https://circleci.com/gh/hikalium/liumos.svg?style=svg)](https://circleci.com/gh/hikalium/liumos)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

![Screenshot](https://github.com/hikalium/liumos/blob/master/docs/2019-11-24.png)


A toy operating system which supports NVDIMM natively.

## Requirements

### Common

[Install Rust toolchain](https://www.rust-lang.org/tools/install)

Check the installation (below is an example output):
```
$ cargo --version
cargo 1.46.0 (149022b1d 2020-07-17)
```

Then, install nightly toolchain and cargo-xbuild for cross compiling.

```
rustup toolchain install nightly && rustup default nightly
rustup component add rust-src
cargo install cargo-xbuild
```

### macOS

```
brew install wget cmake qemu llvm dosfstools
```

### Ubuntu 20.04

```
sudo apt install wget cmake qemu-system-x86 clang-8 lld-8 libc++-8-dev libc++abi-8-dev clang-format
```

## Prepare tools and libraries

Move to the root of this source tree. Then:

```
make tools
cd src
make newlib && make libcxxabi && make libcxx
```

## How to build

```
make
```

## Run on QEMU

This repository contains [OVMF](https://github.com/tianocore/tianocore.github.io/wiki/OVMF) binary for UEFI emulation.

```
make run
```

You can connect serial console using telnet

```
telnet localhost 1235
```

## Setup tap interface (for linux)

`make run_nic_linux` runs liumOS on QEMU with a tap interface.
To avoid running QEMU with sudo, you need to setup a tap interface in advance.

```
sudo tunctl -u <user>
sudo brctl addif <bridge> tap0
```

## References
- [newlib](https://sourceware.org/newlib/)
- [llvm](https://llvm.org/)
- [tianocore/edk2](https://github.com/tianocore/edk2)
- [cupnes/x86_64_jisaku_os_samples](https://github.com/cupnes/x86_64_jisaku_os_samples)
- [yoppeh/efi-clang](https://github.com/yoppeh/efi-clang)

## Author
[hikalium](https://github.com/hikalium)
