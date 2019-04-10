#pragma once

inline void _mm_clflush(void const* p);

static inline void CLFlush(const void* p) {
  _mm_clflush(p);
}
static inline void CLFlush(const void* buf, size_t size) {
  constexpr uint64_t kCacheLineSize = 64;
  for (size_t i = 0; i < size; i += kCacheLineSize) {
    _mm_clflush(reinterpret_cast<const uint8_t*>(buf) + i);
  }
}

static inline void CLFlush(const void* buf, size_t size, uint64_t& stat) {
  constexpr uint64_t kCacheLineSize = 64;
  for (size_t i = 0; i < size; i += kCacheLineSize) {
    _mm_clflush(reinterpret_cast<const uint8_t*>(buf) + i);
    stat++;
  }
}
