# liumOS


[![CircleCI](https://circleci.com/gh/hikalium/liumos.svg?style=svg)](https://circleci.com/gh/hikalium/liumos)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

![Screenshot](https://github.com/hikalium/liumos/blob/master/docs/2019-11-24.png)


A toy operating system supports UEFI boot.

## Requirements

### macOS

```
brew install wget cmake qemu llvm dosfstools
```

### Ubuntu 18.04

```
sudo apt install wget cmake qemu clang-8 lld-8
```

## Prepare tools and libraries
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

## References
- [newlib](https://sourceware.org/newlib/)
- [llvm](https://llvm.org/)
- [tianocore/edk2](https://github.com/tianocore/edk2)
- [cupnes/x86_64_jisaku_os_samples](https://github.com/cupnes/x86_64_jisaku_os_samples)
- [yoppeh/efi-clang](https://github.com/yoppeh/efi-clang)

## Author
[hikalium](https://github.com/hikalium)
