#include "liumlib.h"

size_t strlen(const char *s) {
  size_t len = 0;
  while (*s) {
    len++;
    s++;
  }
  return len;
}

char *strcpy(char *dest, const char *src) {
  char *start = dest;
  while (*src) {
    *dest = *src;
    dest++;
    src++;
  }
  return start;
}

char *strcat(char *dest, const char *src) {
  char* ptr = dest + strlen(dest);
  while (*src != '\0')
    *ptr++ = *src++;
  *ptr = '\0';
  return dest;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1) {
    if (*s1 != *s2)
      break;
    s1++;
    s2++;
  }

  return *(const unsigned char*) s1 - *(const unsigned char*) s2;
}

char *strtok(char *str, const char *delim) {
  static char *save_ptr;

  if (str == NULL)
    str = save_ptr;

  char *start = str;

  while (*str) {
    for (int i=0; i<strlen(delim); i++) {
      if (*str == delim[i]) {
        *str = '\0';
        str++;
        save_ptr = str;
        return start;
      }
    }
    str++;
  }

  save_ptr = str;
  return start;
}

void *malloc(unsigned long n) {
  if (sizeof(malloc_array) < (malloc_size + n)) {
    write(1, "fail: malloc\n", 13);
    exit(1);
  }

  void* ptr = malloc_array + malloc_size;
  malloc_size = malloc_size + n;
  return ptr;
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

uint32_t htonl(uint32_t hostlong) {
  if (is_big_endian()) {
    return hostlong;
  } else {
    return __bswap_32(hostlong);
  }
}

uint32_t inet_addr(const char *cp) {
  int dots = 0;
  uint32_t acc = 0;
  uint32_t addrs[4];
  uint32_t addr = 0;
  int index = 0;

  while (*cp) {
	switch (*cp) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  acc = acc * 10 + (*cp - '0');
	  break;
	case '.':
	  if (++dots > 3) {
	    return 0;
	  }
      addrs[index++] = acc;
      acc = 0;
      break;
	case '\0':
	  if (acc > 255) {
		return 0;
	  }
      addrs[index++] = acc;
      acc = 0;
      break;
    default:
	  return 0;
    }
    cp++;
  }

  addrs[index++] = acc;
  acc = 0;

  switch (dots) {
    case 3:
      addr = addrs[3] << 24 | addrs[2] << 16 | addrs[1] << 8 | addrs[0];
      break;
    case 2:
      addr = addrs[2] << 24 | addrs[1] << 8 | addrs[0];
      break;
    case 1:
      addr = addrs[1] << 24 | addrs[0];
      break;
    default:
      addr = addrs[0] << 24;
  }

  return addr;
}
