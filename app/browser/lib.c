#include "lib.h"

#include "../liumlib/liumlib.h"

void println(char *text) {
  char output[100000];
  int i = 0;
  while (text[i] != '\0') {
    output[i] = text[i];
    i++;
  }
  write(1, output, i);
  write(1, "\n", 1);
}

void printnum(int n) {
  char s[16];
  int i;
  if (n < 0) {
    write(1, "-", 1);
    n = -n;
  }
  for (i = sizeof(s) - 1; i > 0; i--) {
    s[i] = n % 10 + '0';
    n /= 10;
    if (!n)
      break;
  }
  write(1, &s[i], sizeof(s) - i);
}
