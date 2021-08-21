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
  bool operator==(const Rect& rhs) const {
    return x == rhs.x && y == rhs.y && xsize == rhs.xsize && ysize == rhs.ysize;
  }
  bool operator!=(const Rect& rhs) const { return !(*this == rhs); }
};
