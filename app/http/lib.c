#include "lib.h"

size_t my_strlen(char *s) {
  size_t len = 0;
  while (*s) {
    len++;
    s++;
  }
  return len;
}

char *my_strcpy(char *dest, char *src) {
  char *start = dest;
  while(*src) {
    *dest = *src;
    dest++;
    src++;
  }
  return start;
}

bool is_big_endian(void) {
  union {
    uint32_t i;
    char c[4];
  } bint = {0x01020304};
  return bint.c[0] == 1; 
}

uint16_t htons(uint16_t hostshort) {
  if (is_big_endian()) {
    return hostshort;
  } else {
    return __bswap_16(hostshort);
  }
}

void *my_malloc(int n) {
  if (sizeof(malloc_array) < (malloc_size + n)) {
    write(1, "fail: malloc\n", 13);
    exit(1);
  }

  void* ptr = malloc_array + malloc_size;
  malloc_size = malloc_size + n;
  return ptr;
}
