#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <functional>

[[noreturn]] void Panic(const char* s) {
  puts(s);
  exit(EXIT_FAILURE);
}
#include "sheet.h"

static void TestFlushSheets(int x,
                            int y,
                            std::function<bool(int)> is_in_refresh_range) {
  printf("TestFlushSheets(%2d,%2d)\n", x, y);
  uint32_t src_mem[12];
  uint32_t dst_mem[12];
  for (uint32_t i = 0; i < 12; i++) {
    src_mem[i] = i + 0xff0000;
    dst_mem[i] = i;
  }
  uint32_t* src_buf = &src_mem[4];
  uint32_t* dst_buf = &dst_mem[4];

  Sheet src, dst;
  src.Init(src_buf, 2, 2, 2, x, y);
  dst.Init(dst_buf, 2, 2, 2, 0, 0);
  src.SetParent(&dst);

  for (uint32_t i = 0; i < 12; i++) {
    assert(dst_mem[i] == i);
  }
  src.Flush(0, 0, 2, 2);
  for (uint32_t i = 0; i < 12; i++) {
    if (is_in_refresh_range(i)) {
      assert(dst_mem[i] == i + 0xff0000 - (2 * y) - x);
    } else {
      assert(dst_mem[i] == i);
    }
  }
}

int main() {
  TestFlushSheets(0, 0, [](int i) { return 4 <= i && i < 8; });
  TestFlushSheets(2, 2, [](int) { return false; });
  TestFlushSheets(-2, 2, [](int) { return false; });
  TestFlushSheets(2, -2, [](int) { return false; });
  TestFlushSheets(-2, -2, [](int) { return false; });
  TestFlushSheets(1, 0, [](int i) { return i == 5 || i == 7; });
  TestFlushSheets(0, 1, [](int i) { return i == 6 || i == 7; });
  TestFlushSheets(1, 1, [](int i) { return i == 7; });
  TestFlushSheets(-1, 0, [](int i) { return i == 4 || i == 6; });
  TestFlushSheets(0, -1, [](int i) { return i == 4 || i == 5; });
  TestFlushSheets(-1, -1, [](int i) { return i == 4; });
  puts("PASS");
  return 0;
}
