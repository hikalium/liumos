#include "liumos.h"

int strncmp(const char* s1, const char* s2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (s1[i] - s2[i])
      return s1[i] - s2[i];
  }
  return 0;
}

void* memcpy(void* dst, const void* src, size_t n) {
  for (size_t i = 0; i < n; i++) {
    reinterpret_cast<uint8_t*>(dst)[i] =
        reinterpret_cast<const uint8_t*>(src)[i];
  }
  return dst;
}
