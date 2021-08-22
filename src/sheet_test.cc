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
  dst.SetMap(dst_map);

  for (uint32_t i = 0; i < 12; i++) {
    assert(dst_mem[i] == i);
  }
  src.SetParent(&dst);  // This flushes the sheet as well
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
    std::function<uint32_t(int, int)> expected_sheet_idx) {
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
  s0.SetMap(sheet0_map);
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

void ExpectEqBuf(uint32_t* actual, uint32_t* expected, int w, int h, int line) {
  printf("%s on line %d:\n", __func__, line);
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      printf("%2d ", actual[y * w + x]);
    }
    printf("\n");
  }
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      assert(actual[y * w + x] == expected[y * w + x]);
    }
  }
}
#define EXPECT_EQ_BUF_3x3(actual, expected) \
  ExpectEqBuf((uint32_t*)actual, (uint32_t*)expected, 3, 3, __LINE__)

void ExpectEqMap(void** actual, void** expected, int w, int h, int line) {
  printf("%s on line %d:\n", __func__, line);
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      printf("%18p ", (void*)actual[y * w + x]);
    }
    printf("\n");
  }
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      assert(actual[y * w + x] == expected[y * w + x]);
    }
  }
}
#define EXPECT_EQ_MAP_3x3(actual, expected) \
  ExpectEqMap((void**)actual, (void**)expected, 3, 3, __LINE__)

void SetBuf(uint32_t* buf, uint32_t value, int w, int h) {
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      buf[y * w + x] = value;
    }
  }
}
#define SET_BUF_3x3(buf, value) SetBuf((uint32_t*)buf, value, 3, 3)

static void TestUpdateMap() {
  printf("%s()\n", __func__);

  uint32_t sheet0_buf[3 * 3];
  Sheet* sheet0_map[3 * 3];
  uint32_t sheet1_buf[3 * 3];
  uint32_t sheet2_buf[3 * 3];

  Sheet s0, s1, s2, s3;

  printf("s0: %p\n", (void*)&s0);
  printf("s1: %p\n", (void*)&s1);
  printf("s2: %p\n", (void*)&s2);
  printf("s3: %p\n", (void*)&s3);

  SET_BUF_3x3(sheet0_buf, 0);
  SET_BUF_3x3(sheet1_buf, 1);
  SET_BUF_3x3(sheet2_buf, 2);

  s0.Init(sheet0_buf, 3, 3, 3, 0, 0);
  s1.Init(sheet1_buf, 3, 3, 3, 2, 0);
  s2.Init(sheet2_buf, 3, 3, 3, -1, -1);
  s0.SetMap(sheet0_map);
  s2.SetParent(&s0);
  s1.SetParent(&s0);

  {
    Sheet* sheet0_map_expected[3 * 3] = {
        &s2,     &s2,     &s1,  // 2 2 1
        &s2,     &s2,     &s1,  // 2 2 1
        nullptr, nullptr, &s1,  // - - 1
    };
    EXPECT_EQ_MAP_3x3(sheet0_map, sheet0_map_expected);
    uint32_t sheet0_buf_expected[3 * 3] = {
        2, 2, 1,  //
        2, 2, 1,  //
        0, 0, 1,  //
    };
    EXPECT_EQ_BUF_3x3(sheet0_buf, sheet0_buf_expected);
  }

  uint32_t sheet3_buf[3 * 3];
  s3.Init(sheet3_buf, 3, 3, 3, 1, 1);
  SET_BUF_3x3(sheet3_buf, 3);
  s3.SetParent(&s0);  // New sheet will be inserted at top
  // Map should be updated automatically

  {
    Sheet* sheet0_map_expected[3 * 3] = {
        &s2,     &s2, &s1,  // 2 2 1
        &s2,     &s3, &s3,  // 2 3 3
        nullptr, &s3, &s3,  // - 3 3
    };
    EXPECT_EQ_MAP_3x3(sheet0_map, sheet0_map_expected);
    uint32_t sheet0_buf_expected[3 * 3] = {
        2, 2, 1,  //
        2, 3, 3,  //
        0, 3, 3,  //
    };
    EXPECT_EQ_BUF_3x3(sheet0_buf, sheet0_buf_expected);
  }
}

static void TestMoveRelative() {
  printf("%s()\n", __func__);

  uint32_t sheet0_buf[3 * 3];
  Sheet* sheet0_map[3 * 3];
  uint32_t sheet1_buf[3 * 3];
  uint32_t sheet2_buf[3 * 3];
  uint32_t sheet3_buf[3 * 3];

  Sheet s0, s1, s2, s3;

  printf("s0: %p\n", (void*)&s0);
  printf("s1: %p\n", (void*)&s1);
  printf("s2: %p\n", (void*)&s2);
  printf("s3: %p\n", (void*)&s3);

  SET_BUF_3x3(sheet0_buf, 0);
  SET_BUF_3x3(sheet1_buf, 1);
  SET_BUF_3x3(sheet2_buf, 2);
  SET_BUF_3x3(sheet3_buf, 3);

  s0.Init(sheet0_buf, 3, 3, 3, 0, 0);
  s0.SetMap(sheet0_map);

  s1.Init(sheet1_buf, 3, 3, 3, 2, 0);
  s2.Init(sheet2_buf, 3, 3, 3, -1, -1);
  s3.Init(sheet3_buf, 3, 3, 3, 0, 0);

  s3.SetParent(&s0);
  s2.SetParent(&s0);
  s1.SetParent(&s0);

  {
    Sheet* sheet0_map_expected[3 * 3] = {
        &s2, &s2, &s1,  // 2 2 1
        &s2, &s2, &s1,  // 2 2 1
        &s3, &s3, &s1,  // 3 3 1
    };
    EXPECT_EQ_MAP_3x3(sheet0_map, sheet0_map_expected);
    uint32_t sheet0_buf_expected[3 * 3] = {
        2, 2, 1,  //
        2, 2, 1,  //
        3, 3, 1,  //
    };
    EXPECT_EQ_BUF_3x3(sheet0_buf, sheet0_buf_expected);
  }

  // Move s1 1px left
  s1.MoveRelative(-1, 0);
  {
    Sheet* sheet0_map_expected[3 * 3] = {
        &s2, &s1, &s1,  // 2 1 1
        &s2, &s1, &s1,  // 2 1 1
        &s3, &s1, &s1,  // 3 1 1
    };
    EXPECT_EQ_MAP_3x3(sheet0_map, sheet0_map_expected);
    uint32_t sheet0_buf_expected[3 * 3] = {
        2, 1, 1,  //
        2, 1, 1,  //
        3, 1, 1,  //
    };
    EXPECT_EQ_BUF_3x3(sheet0_buf, sheet0_buf_expected);
  }

  // Move s1 1px down
  s1.MoveRelative(0, 1);
  {
    Sheet* sheet0_map_expected[3 * 3] = {
        &s2, &s2, &s3,  // 2 2 3
        &s2, &s1, &s1,  // 2 1 1
        &s3, &s1, &s1,  // 3 1 1
    };
    EXPECT_EQ_MAP_3x3(sheet0_map, sheet0_map_expected);
    uint32_t sheet0_buf_expected[3 * 3] = {
        2, 2, 3,  //
        2, 1, 1,  //
        3, 1, 1,  //
    };
    EXPECT_EQ_BUF_3x3(sheet0_buf, sheet0_buf_expected);
  }
}

static void TestTransparent() {
  printf("%s()\n", __func__);

  uint32_t sheet0_buf[3 * 3];  // vram
  Sheet* sheet0_map[3 * 3];
  uint32_t sheet1_buf[3 * 3];  // background
  uint32_t sheet2_buf[3 * 3];  // foreground

  Sheet s0, s1, s2;

  printf("s0: %p\n", (void*)&s0);
  printf("s1: %p\n", (void*)&s1);
  printf("s2: %p\n", (void*)&s2);

  SET_BUF_3x3(sheet0_buf, 0);
  SET_BUF_3x3(sheet1_buf, 1);
  SET_BUF_3x3(sheet2_buf, 2);

  s0.Init(sheet0_buf, 3, 3, 3, 0, 0);
  s0.SetMap(sheet0_map);

  s1.Init(sheet1_buf, 3, 3, 3, 0, 0);
  s1.SetParent(&s0);

  s2.Init(sheet2_buf, 3, 3, 3, 0, 0);
  s2.SetParent(&s0);

  {
    Sheet* sheet0_map_expected[3 * 3] = {
        &s2, &s2, &s2,  //
        &s2, &s2, &s2,  //
        &s2, &s2, &s2,  //
    };
    EXPECT_EQ_MAP_3x3(sheet0_map, sheet0_map_expected);
    uint32_t sheet0_buf_expected[3 * 3] = {
        2, 2, 2,  //
        2, 2, 2,  //
        2, 2, 2,  //
    };
    EXPECT_EQ_BUF_3x3(sheet0_buf, sheet0_buf_expected);
  }

  s2.SetAlphaEnabled(true);

  {
    Sheet* sheet0_map_expected[3 * 3] = {
        &s1, &s1, &s1,  //
        &s1, &s1, &s1,  //
        &s1, &s1, &s1,  //
    };
    EXPECT_EQ_MAP_3x3(sheet0_map, sheet0_map_expected);
    uint32_t sheet0_buf_expected[3 * 3] = {
        1, 1, 1,  //
        1, 1, 1,  //
        1, 1, 1,  //
    };
    EXPECT_EQ_BUF_3x3(sheet0_buf, sheet0_buf_expected);
  }
}

int main() {
  TestTransparent();
  TestMoveRelative();
  TestUpdateMap();

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

  TestFlushOverwrapped(3, 3, 3, 3, [](int, int) { return 0; });
  TestFlushOverwrapped(0, 0, 3, 3, [](int, int) { return 1; });
  TestFlushOverwrapped(1, 1, 3, 3,
                       [](int x, int y) { return (x == 0 || y == 0) ? 0 : 1; });
  TestFlushOverwrapped(3, 3, 0, 0, [](int, int) { return 2; });
  TestFlushOverwrapped(3, 3, 1, 1,
                       [](int x, int y) { return (x == 0 || y == 0) ? 0 : 2; });
  TestFlushOverwrapped(1, 1, 2, 2, [](int x, int y) {
    if (x == 0 || y == 0)
      return 0;
    return 1;
  });

  TestFlushOverwrapped(2, 2, 1, 1, [](int x, int y) {
    if (x == 0 || y == 0)
      return 0;
    if (x == 1 || y == 1)
      return 2;
    return 1;
  });
  puts("PASS");
  return 0;
}
