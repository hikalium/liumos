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
