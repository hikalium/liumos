#include "sheet_painter.h"

#include "asm.h"

// @font.gen.c
extern uint8_t font[0x100][16];

void SheetPainter::DrawCharacter(Sheet& s,
                                 char c,
                                 int px,
                                 int py,
                                 bool do_flush) {
  if (!s.buf_)
    return;
  uint32_t* b32 = reinterpret_cast<uint32_t*>(s.buf_);
  for (int dy = 0; dy < 16; dy++) {
    for (int dx = 0; dx < 8; dx++) {
      uint32_t col = ((font[(uint8_t)c][dy] >> (7 - dx)) & 1) ? 0xffffff : 0;
      int x = px + dx;
      int y = py + dy;
      b32[y * s.pixels_per_scan_line_ + x] = col;
    }
  }
  if (do_flush)
    s.Flush(px, py, 8, 16);
}

void SheetPainter::DrawCharacterForeground(Sheet& s,
                                           char c,
                                           int px,
                                           int py,
                                           uint32_t col,
                                           bool do_flush) {
  if (!s.buf_)
    return;
  uint32_t* b32 = reinterpret_cast<uint32_t*>(s.buf_);
  for (int dy = 0; dy < 16; dy++) {
    for (int dx = 0; dx < 8; dx++) {
      if (!((font[(uint8_t)c][dy] >> (7 - dx)) & 1))
        continue;
      int x = px + dx;
      int y = py + dy;
      b32[y * s.pixels_per_scan_line_ + x] = col;
    }
  }
  if (do_flush)
    s.Flush(px, py, 8, 16);
}

void SheetPainter::DrawRect(Sheet& s,
                            int px,
                            int py,
                            int w,
                            int h,
                            uint32_t col,
                            bool do_flush) {
  if (!s.buf_)
    return;
  uint32_t* b32 = reinterpret_cast<uint32_t*>(s.buf_);
  if (w & 1) {
    for (int y = py; y < py + h; y++) {
      RepeatStore4Bytes(w, &b32[y * s.pixels_per_scan_line_ + px], col);
    }
  } else {
    for (int y = py; y < py + h; y++) {
      RepeatStore8Bytes(w / 2, &b32[y * s.pixels_per_scan_line_ + px],
                        ((uint64_t)col << 32) | col);
    }
  }
  if (do_flush)
    s.Flush(px, py, w, h);
}

void SheetPainter::DrawPoint(Sheet& s,
                             int px,
                             int py,
                             uint32_t col,
                             bool do_flush) {
  s.buf_[py * s.pixels_per_scan_line_ + px] = col;
  if (do_flush)
    s.Flush(px, py, 1, 1);
}
