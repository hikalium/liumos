#pragma once

#include <stddef.h>
#include <stdint.h>

#define packed_struct struct __attribute__((__packed__))
#define packed_union union __attribute__((__packed__))

#ifndef assert
void __assert(const char* expr_str, const char* file, int line);
#define assert(expr) \
  ((void)((expr) || (__assert(#expr, __FILE__, __LINE__), 0)))
#endif

constexpr uint64_t kPageSizeExponent = 12;
constexpr uint64_t kPageSize = 1 << kPageSizeExponent;
inline uint64_t ByteSizeToPageSize(uint64_t byte_size) {
  return (byte_size + kPageSize - 1) >> kPageSizeExponent;
}
