#include <stdio.h>
#include <stdlib.h>
#include <cassert>

[[noreturn]] void Panic(const char* s) {
  puts(s);
  exit(EXIT_FAILURE);
}
#include "sheet.h"

static void TestFlushSheets() {
  uint32_t src_mem[12];
  uint32_t dst_mem[12];
  for (uint32_t i = 0; i < 12; i++) {
    src_mem[i] = i + 0xff0000;
    dst_mem[i] = i;
  }
  uint32_t* src_buf = &src_mem[4];
  uint32_t* dst_buf = &dst_mem[4];

  Sheet src, dst;
  src.Init(src_buf, 2, 2, 2, 0, 0);
  dst.Init(dst_buf, 2, 2, 2, 0, 0);
  src.SetParent(&dst);

  for (uint32_t i = 0; i < 12; i++) {
    assert(dst_mem[i] == i);
  }
  src.Flush(0, 0, 1, 1);
  for (uint32_t i = 0; i < 12; i++) {
    if (i == 4) {
      assert(dst_mem[i] == i + 0xff0000);
    } else {
      assert(dst_mem[i] == i);
    }
  }
  src.Flush(0, 0, 2, 2);
  for (uint32_t i = 0; i < 12; i++) {
    if (4 <= i && i < 8) {
      assert(dst_mem[i] == i + 0xff0000);
    } else {
      assert(dst_mem[i] == i);
    }
  }
}

static void TestFlushSheetsNotFullyOverlapped() {
  uint32_t src_mem[12];
  uint32_t dst_mem[12];
  for (uint32_t i = 0; i < 12; i++) {
    src_mem[i] = i + 0xff0000;
    dst_mem[i] = i;
  }
  uint32_t* src_buf = &src_mem[4];
  uint32_t* dst_buf = &dst_mem[4];

  Sheet src, dst;
  src.Init(src_buf, 2, 2, 2, 1, 1);
  dst.Init(dst_buf, 2, 2, 2, 0, 0);
  src.SetParent(&dst);

  for (uint32_t i = 0; i < 12; i++) {
    assert(dst_mem[i] == i);
  }
  src.Flush(0, 0, 1, 1);
  for (uint32_t i = 0; i < 12; i++) {
    if (i == 7) {
      assert(dst_mem[i] == 4 + 0xff0000);
    } else {
      assert(dst_mem[i] == i);
    }
  }
  src.Flush(0, 0, 2, 2);
  for (uint32_t i = 0; i < 12; i++) {
    if (i == 7) {
      assert(dst_mem[i] == 4 + 0xff0000);
    } else {
      assert(dst_mem[i] == i);
    }
  }
}

int main() {
  TestFlushSheets();
  TestFlushSheetsNotFullyOverlapped();
  puts("PASS");
  return 0;
}
