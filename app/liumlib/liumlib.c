#include "liumlib.h"

size_t strlen(const char* s) {
  size_t len = 0;
  while (*s) {
    len++;
    s++;
  }
  return len;
}

char* strcpy(char* dest, const char* src) {
  char* start = dest;
  while (*src) {
    *dest = *src;
    dest++;
    src++;
  }
  return start;
}

char* strcat(char* dest, const char* src) {
  char* ptr = dest + strlen(dest);
  while (*src != '\0')
    *ptr++ = *src++;
  *ptr = '\0';
  return dest;
}

char* strncat(char* dest, const char* src, unsigned long n) {
  char* ptr = dest + strlen(dest);
  while (*src != '\0' && n--)
    *ptr++ = *src++;
  *ptr = '\0';
  return dest;
}

int strcmp(const char* s1, const char* s2) {
  while (*s1) {
    if (*s1 != *s2)
      break;
    s1++;
    s2++;
  }

  return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Return 0 if two strings are identical.
int strncmp(const char* s1, const char* s2, unsigned long n) {
  for (int i = 0; i < n; i++) {
    if (s1[i] == '\0' || s2[i] == '\0')
      return 0;
    if (s1[i] != s2[i])
      return 1;
  }
  return 0;
}

char* strtok(char* str, const char* delim) {
  static char* save_ptr;

  if (str == NULL)
    str = save_ptr;

  char* start = str;

  while (*str) {
    for (int i = 0; i < strlen(delim); i++) {
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

void* malloc(unsigned long n) {
  if (sizeof(malloc_array) < (malloc_size + n)) {
    write(1, "fail: malloc\n", 13);
    exit(1);
  }

  void* ptr = malloc_array + malloc_size;
  malloc_size = malloc_size + n;
  memset(ptr, 0, n);
  return ptr;
}

void* memset(void* s, int c, size_t n) {
  char* dest = (char*)s;
  while (n > 0) {
    *dest = (char)c;
    dest++;
    n--;
  }
  return s;
}

void* memcpy(void* dest, const void* src, size_t n) {
  char* src_char = (char*)src;
  char* dest_char = (char*)dest;
  for (int i = 0; i < n; i++) {
    dest_char[i] = src_char[i];
  }
  return dest;
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

uint32_t inet_addr(const char* cp) {
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

void Print(const char* s) {
  write(1, s, strlen(s));
}

void Println(const char* s) {
  write(1, s, strlen(s));
  write(1, "\n", 1);
}

void PrintNum(int v) {
  char s[16];
  int i;
  if (v < 0) {
    write(1, "-", 1);
    v = -v;
  }
  for (i = sizeof(s) - 1; i > 0; i--) {
    s[i] = v % 10 + '0';
    v /= 10;
    if (!v)
      break;
  }
  write(1, &s[i], sizeof(s) - i);
}

char NumToHexChar(char v) {
  if (v < 10)
    return v + '0';
  if (v < 16)
    return v - 10 + 'A';
  return '?';
}

uint8_t StrToByte(const char* s, const char** next) {
  uint32_t v = 0;
  while ('0' <= *s && *s <= '9') {
    v = v * 10 + *s - '0';
    s++;
  }
  if (next) {
    *next = s;
  }
  return v;
}

uint16_t StrToNum16(const char* s, const char** next) {
  uint32_t v = 0;
  while ('0' <= *s && *s <= '9') {
    v = v * 10 + *s - '0';
    s++;
  }
  if (next) {
    *next = s;
  }
  return v;
}

void __assert(const char* expr_str, const char* file, int line) {
  Print("\nAssertion failed: ");
  Print(expr_str);
  Print(" at ");
  Print(file);
  Print(":");
  PrintNum(line);
  Print("\n");
  exit(EXIT_FAILURE);
}

in_addr_t MakeIPv4Addr(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  // a.b.c.d -> in_addr_t (=uint32_t)
  uint8_t buf[4];
  buf[0] = a;
  buf[1] = b;
  buf[2] = c;
  buf[3] = d;
  return *(uint32_t*)buf;
}

in_addr_t MakeIPv4AddrFromString(const char* s) {
  // "a.b.c.d" -> in_addr_t (=uint32_t)
  uint8_t buf[4];
  for (int i = 0;; i++) {
    buf[i] = StrToByte(s, &s);
    if (i == 3)
      break;
    assert(*s == '.');
    s++;
  }
  return *(uint32_t*)buf;
}

void PrintHex8ZeroFilled(uint8_t v) {
  char s[2];
  s[0] = NumToHexChar((v >> 4) & 0xF);
  s[1] = NumToHexChar(v & 0xF);
  write(1, s, 2);
}

void PrintIPv4Addr(in_addr_t addr) {
  uint8_t buf[4];
  *(uint32_t*)buf = addr;
  for (int i = 0;; i++) {
    PrintNum(buf[i]);
    if (i == 3)
      break;
    Print(".");
  }
}

void panic(const char* s) {
  write(1, s, strlen(s));
  exit(EXIT_FAILURE);
}
