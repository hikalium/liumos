#include <stdint.h>

typedef unsigned long size_t;
int write(int fd, const void*, size_t);
void exit(int);

size_t strlen(const char* s) {
  size_t len = 0;
  while (*s) {
    len++;
    s++;
  }
  return len;
}

void Print(const char* s) {
  write(1, s, strlen(s));
}
void PrintNum(int v) {
  char s[16];
  int i;
  for (i = sizeof(s) - 1; i > 0; i--) {
    s[i] = v % 10 + '0';
    v /= 10;
    if (!v)
      break;
  }
  write(1, &s[i], sizeof(s) - i);
}

int main(int argc, char** argv) {
  Print("argc: ");
  PrintNum(argc);
  Print("\n");
  for (int i = 0; i < argc; i++) {
    Print("argv[");
    PrintNum(i);
    Print("]: ");
    Print(argv[i]);
    Print("\n");
  }
}

