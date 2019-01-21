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
