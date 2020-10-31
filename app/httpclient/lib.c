#include "../liumlib/liumlib.h"
#include "lib.h"

void printnum(int v) {
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
  write(1, "\n", 1);
}

void println(char* text) {
  char output[100000];
  int i = 0;
  while (text[i] != '\0') {
    output[i] = text[i];
    i++;
  }
  write(1, output, i + 1);
  write(1, "\n", 1);
}

uint16_t str_to_num16(const char* s, const char** next) {
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
