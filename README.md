# liumOS


[![CircleCI](https://circleci.com/gh/hikalium/liumos.svg?style=svg)](https://circleci.com/gh/hikalium/liumos)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

![liumOS](https://github.com/hikalium/liumos/blob/master/dist/liumos.png)

A toy operating system supports UEFI boot.

## Requirements

### macOS

- Tested on 10.13.6

```
brew install qemu llvm dosfstools FiloSottile/musl-cross/musl-cross
```

### Ubuntu 18.04

```
sudo apt install clang lld-4.0 qemu

```

## Prepare tools and libraries
```
git submodule update
make tools
make libcxx
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

## References
- [cupnes/x86_64_jisaku_os_samples](https://github.com/cupnes/x86_64_jisaku_os_samples)
- [yoppeh/efi-clang](https://github.com/yoppeh/efi-clang)
- [tianocore/edk2](https://github.com/tianocore/edk2)

## Author
[hikalium](https://github.com/hikalium)
