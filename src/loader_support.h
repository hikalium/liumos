#pragma once

#include <stddef.h>
#include <stdint.h>
inline void* operator new(size_t, void* where) {
  return where;
}

namespace std {
inline int max(const int a, const int b) {
  return a > b ? a : b;
}
inline int min(const int a, const int b) {
  return a < b ? a : b;
}
}  // namespace std
