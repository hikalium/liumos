#include "browser.h"

#define MAX_SIZE 100000

int size;
char array[MAX_SIZE];

void* my_malloc(int n) {
  if (sizeof(array) < (size + n)) {
    write(1, "fail: malloc\n", 13);
    exit(1);
  }

  void* ptr = array + size;
  size = size + n;
  return ptr;
}
