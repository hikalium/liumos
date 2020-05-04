#include "libfunc.h"

#include <stdio.h>

#include <cassert>

int main() {
  assert(TEST_TARGET(strncmp)("", "", 0) == 0);
  assert(TEST_TARGET(strncmp)("\0a", "\0b", 3) == 0);

  assert(TEST_TARGET(memcmp)("", "", 0) == 0);
  assert(TEST_TARGET(memcmp)("\0a", "\0b", 3) != 0);
  assert(TEST_TARGET(memcmp)("\0a", "\0b", 2) != 0);
  assert(TEST_TARGET(memcmp)("\0a", "\0b", 1) == 0);
  assert(TEST_TARGET(memcmp)("\0a", "\0b", 0) == 0);

  puts("PASS");
  return 0;
}
