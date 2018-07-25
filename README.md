# liumOS
A toy operating system supports UEFI boot.

## Requirements

Currently, this project is built and tested on macOS (10.13.6).

- qemu (for qemu-system-x86_64)
- llvm (for clang, lld-link)

Install these packages via [Homebrew](https://brew.sh/).

```
brew install qemu llvm
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
