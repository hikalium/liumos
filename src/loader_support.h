#pragma once

#include <stddef.h>
#include <stdint.h>
inline void* operator new(size_t, void* where) _NOEXCEPT {
  return where;
}
