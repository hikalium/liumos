#pragma once

#include <stddef.h>
#include <stdint.h>

#include "immintrin.h"

#define packed_struct struct __attribute__((__packed__))
#define packed_union union __attribute__((__packed__))

[[noreturn]] void Panic(const char* s);

#ifdef LIUMOS_TEST
#include <cassert>
#else
void __assert(const char* expr_str, const char* file, int line);
#undef assert
#define assert(expr) \
  ((void)((expr) || (__assert(#expr, __FILE__, __LINE__), 0)))
#endif

constexpr uint64_t kPageSizeExponent = 12;
constexpr uint64_t kPageSize = 1 << kPageSizeExponent;
constexpr uint64_t kPageAddrMask = kPageSize - 1;
inline uint64_t ByteSizeToPageSize(uint64_t byte_size) {
  return (byte_size + kPageSize - 1) >> kPageSizeExponent;
}

bool IsEqualString(const char* a, const char* b);

#ifdef LIUMOS_LOADER
#include "loader_support.h"
#else
#include <new>
#endif
