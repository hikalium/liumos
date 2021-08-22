#pragma once

// for std::min, max
#ifdef LIUMOS_LOADER
#include "loader_support.h"
#else
#include <algorithm>
#endif

struct Rect {
  int x, y, xsize, ysize;
  int GetRight() const { return x + xsize; }
  int GetBottom() const { return y + ysize; }
  Rect GetIntersectionWith(Rect t) {
    if (xsize < 0 || ysize < 0 || t.xsize < 0 || t.ysize < 0)
      return {0, 0, 0, 0};
    int bx = std::max(x, t.x);
    int by = std::max(y, t.y);
    int bxsize = std::max(std::min(xsize + x - bx, t.xsize + t.x - bx), 0);
    int bysize = std::max(std::min(ysize + y - by, t.ysize + t.y - by), 0);
    return {bx, by, bxsize, bysize};
  }
  Rect GetUnionWith(Rect s) const {
    int l1 = x;
    int r1 = x + xsize;
    int u1 = y;
    int d1 = y + ysize;
    int l2 = s.x;
    int r2 = s.x + s.xsize;
    int u2 = s.y;
    int d2 = s.y + s.ysize;
    int l = std::min(l1, l2);
    int r = std::max(r1, r2);
    int u = std::min(u1, u2);
    int d = std::max(d1, d2);
    return {l, u, r - l, d - u};
  }
  bool IsPointInRect(int px, int py) {
    return x <= px && px < x + xsize && y <= py && py < y + ysize;
  }
  bool IsEmptyRect() { return x == 0 && y == 0 && xsize == 0 && ysize == 0; }
  bool operator==(const Rect& rhs) const {
    return x == rhs.x && y == rhs.y && xsize == rhs.xsize && ysize == rhs.ysize;
  }
  bool operator!=(const Rect& rhs) const { return !(*this == rhs); }
};
