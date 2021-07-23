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
                            std::function<bool(int)> is_in_refresh_range,
                            bool use_map) {
  printf("%s(%2d,%2d)\n", __func__, x, y);
  uint32_t src_mem[12];
  uint32_t dst_mem[12];
  for (uint32_t i = 0; i < 12; i++) {
    src_mem[i] = i + 0xff0000;
    dst_mem[i] = i;
  }
  uint32_t* src_buf = &src_mem[4];
  uint32_t* dst_buf = &dst_mem[4];
  Sheet* dst_map[3 * 3];

  Sheet src, dst;
  src.Init(src_buf, 2, 2, 2, x, y);
  dst.Init(dst_buf, 2, 2, 2, 0, 0);
  if (use_map) {
    dst.SetMap(dst_map);
  }
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

static void TestFlushOverwrapped(
    int x1,
    int y1,
    int x2,
    int y2,
    std::function<uint32_t(int, int)> expected_sheet_idx,
    bool use_map) {
  printf("%s(%2d,%2d,%2d,%2d)\n", __func__, x1, y1, x2, y2);
  // sheet0, 1 and 2 are 3x3 px sheets.
  // sheet0 is a parant of sheet 1 and 2.
  // sheet1 is in front of sheet2.
  uint32_t sheet0_buf[3 * 3];
  uint32_t sheet1_buf[3 * 3];
  uint32_t sheet2_buf[3 * 3];
  Sheet* sheet0_map[3 * 3];

  Sheet s0, s1, s2;
  s0.Init(sheet0_buf, 3, 3, 3, 0, 0);
  if (use_map) {
    s0.SetMap(sheet0_map);
  }
  s1.Init(sheet1_buf, 3, 3, 3, x1, y1);
  s2.Init(sheet2_buf, 3, 3, 3, x2, y2);
  s2.SetParent(&s0);
  s1.SetParent(&s0);

  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      sheet0_buf[y * 3 + x] = 0;
      sheet1_buf[y * 3 + x] = 1;
      sheet2_buf[y * 3 + x] = 2;
    }
  }

  s2.Flush(0, 0, 3, 3);
  s1.Flush(0, 0, 3, 3);
  printf("s2 -> s1\n");
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      printf("%d ", sheet0_buf[y * 3 + x]);
    }
    printf("\n");
  }
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      assert(sheet0_buf[y * 3 + x] == expected_sheet_idx(x, y));
    }
  }
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      sheet0_buf[y * 3 + x] = 0;
    }
  }

  s1.Flush(0, 0, 3, 3);
  s2.Flush(0, 0, 3, 3);
  printf("s1 -> s2\n");
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      printf("%d ", sheet0_buf[y * 3 + x]);
    }
    printf("\n");
  }
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      assert(sheet0_buf[y * 3 + x] == expected_sheet_idx(x, y));
    }
  }

  s1.Flush(0, 0, 3, 3);
  printf("+s1\n");
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      printf("%d ", sheet0_buf[y * 3 + x]);
    }
    printf("\n");
  }
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      assert(sheet0_buf[y * 3 + x] == expected_sheet_idx(x, y));
    }
  }

  s2.Flush(0, 0, 3, 3);
  printf("+s2\n");
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      printf("%d ", sheet0_buf[y * 3 + x]);
    }
    printf("\n");
  }
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      assert(sheet0_buf[y * 3 + x] == expected_sheet_idx(x, y));
    }
  }
}

static void TestUpdateMap() {
  printf("%s()\n", __func__);

  uint32_t sheet0_buf[3 * 3];
  Sheet* sheet0_map[3 * 3];
  uint32_t sheet1_buf[3 * 3];
  uint32_t sheet2_buf[3 * 3];

  Sheet s0, s1, s2;
  s0.Init(sheet0_buf, 3, 3, 3, 0, 0);
  s1.Init(sheet1_buf, 3, 3, 3, 2, 0);
  s2.Init(sheet2_buf, 3, 3, 3, -1, -1);
  s2.SetParent(&s0);
  s1.SetParent(&s0);
  s0.SetMap(sheet0_map);

  {
    Sheet* sheet0_map_expected[3 * 3] = {
        &s2,     &s2,     &s1,  // 2 2 1
        &s2,     &s2,     &s1,  // 2 2 1
        nullptr, nullptr, &s1,  // - - 1
    };
    printf("s0: %p\n", (void*)&s0);
    printf("s1: %p\n", (void*)&s1);
    printf("s2: %p\n", (void*)&s2);
    for (int y = 0; y < 3; y++) {
      for (int x = 0; x < 3; x++) {
        printf("%p ", (void*)sheet0_map[y * 3 + x]);
      }
      printf("\n");
    }
    for (int y = 0; y < 3; y++) {
      for (int x = 0; x < 3; x++) {
        assert(sheet0_map[y * 3 + x] == sheet0_map_expected[y * 3 + x]);
      }
    }
  }

  Sheet s3;
  uint32_t sheet3_buf[3 * 3];
  s3.Init(sheet3_buf, 3, 3, 3, 1, 1);
  s3.SetParent(&s0);  // New sheet will be inserted at top
  // Map should be updated automatically

  {
    Sheet* sheet0_map_expected[3 * 3] = {
        &s2,     &s2, &s1,  // 2 2 1
        &s2,     &s3, &s3,  // 2 3 3
        nullptr, &s3, &s3,  // - 3 3
    };
    printf("s0: %p\n", (void*)&s0);
    printf("s1: %p\n", (void*)&s1);
    printf("s2: %p\n", (void*)&s2);
    printf("s3: %p\n", (void*)&s3);
    for (int y = 0; y < 3; y++) {
      for (int x = 0; x < 3; x++) {
        printf("%p ", (void*)sheet0_map[y * 3 + x]);
      }
      printf("\n");
    }
    for (int y = 0; y < 3; y++) {
      for (int x = 0; x < 3; x++) {
        assert(sheet0_map[y * 3 + x] == sheet0_map_expected[y * 3 + x]);
      }
    }
  }
}

int main() {
  TestUpdateMap();

  for (int use_map = 0; use_map < 2; use_map++) {
    TestFlushSheets(
        0, 0, [](int i) { return 4 <= i && i < 8; }, use_map);
    TestFlushSheets(
        2, 2, [](int) { return false; }, use_map);
    TestFlushSheets(
        -2, 2, [](int) { return false; }, use_map);
    TestFlushSheets(
        2, -2, [](int) { return false; }, use_map);
    TestFlushSheets(
        -2, -2, [](int) { return false; }, use_map);
    TestFlushSheets(
        1, 0, [](int i) { return i == 5 || i == 7; }, use_map);
    TestFlushSheets(
        0, 1, [](int i) { return i == 6 || i == 7; }, use_map);
    TestFlushSheets(
        1, 1, [](int i) { return i == 7; }, use_map);
    TestFlushSheets(
        -1, 0, [](int i) { return i == 4 || i == 6; }, use_map);
    TestFlushSheets(
        0, -1, [](int i) { return i == 4 || i == 5; }, use_map);
    TestFlushSheets(
        -1, -1, [](int i) { return i == 4; }, use_map);

    TestFlushOverwrapped(
        3, 3, 3, 3, [](int, int) { return 0; }, use_map);
    TestFlushOverwrapped(
        0, 0, 3, 3, [](int, int) { return 1; }, use_map);
    TestFlushOverwrapped(
        1, 1, 3, 3, [](int x, int y) { return (x == 0 || y == 0) ? 0 : 1; },
        use_map);
    TestFlushOverwrapped(
        3, 3, 0, 0, [](int, int) { return 2; }, use_map);
    TestFlushOverwrapped(
        3, 3, 1, 1, [](int x, int y) { return (x == 0 || y == 0) ? 0 : 2; },
        use_map);
    TestFlushOverwrapped(
        1, 1, 2, 2,
        [](int x, int y) {
          if (x == 0 || y == 0)
            return 0;
          return 1;
        },
        use_map);

    TestFlushOverwrapped(
        2, 2, 1, 1,
        [](int x, int y) {
          if (x == 0 || y == 0)
            return 0;
          if (x == 1 || y == 1)
            return 2;
          return 1;
        },
        use_map);
  }

  puts("PASS");
  return 0;
}
