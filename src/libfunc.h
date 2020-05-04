#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef LIUMOS_TEST
#define TEST_TARGET(f) test_##f
#else
#define TEST_TARGET(f) f
#endif

extern "C" {
int TEST_TARGET(strncmp)(const char* s1, const char* s2, size_t n);
int TEST_TARGET(memcmp)(const void* b1, const void* b2, size_t n);
int strcmp(const char* s1, const char* s2);
void* memcpy(void* dst, const void* src, size_t n);
void bzero(volatile void* s, size_t n);
int atoi(const char* str);
}
template <typename T>
inline T min(T a, T b) {
  return a < b ? a : b;
}
