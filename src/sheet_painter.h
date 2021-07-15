#pragma once
#include "sheet.h"

class SheetPainter {
 public:
  static void DrawCharacter(Sheet&,
                            char c,
                            int px,
                            int py,
                            bool do_flush = false);
  static void DrawString(Sheet& sheet,
                         const char* s,
                         int px,
                         int py,
                         bool do_flush = false) {
    int i;
    for (i = 0; s[i]; i++) {
      DrawCharacter(sheet, s[i], px + 8 * i, py, false);
    }
    if (!do_flush) {
      return;
    }
    sheet.Flush(px, py, px + 8 * i, py + 16);
  };
  static void DrawCharacterForeground(Sheet&,
                                      char c,
                                      int px,
                                      int py,
                                      uint32_t col,
                                      bool do_flush = false);
  static void DrawRect(Sheet& s,
                       int px,
                       int py,
                       int w,
                       int h,
                       uint32_t col,
                       bool do_flush = false);
  static void DrawPoint(Sheet& s,
                        int px,
                        int py,
                        uint32_t col,
                        bool do_flush = false);
};
