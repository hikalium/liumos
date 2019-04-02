#include "liumos.h"

void Sheet::DrawCharacter(char c, int px, int py) {
  if (!buf_)
    return;
  uint32_t* b32 = reinterpret_cast<uint32_t*>(buf_);
  for (int dy = 0; dy < 16; dy++) {
    for (int dx = 0; dx < 8; dx++) {
      uint32_t col = ((font[(uint8_t)c][dy] >> (7 - dx)) & 1) ? 0xffffff : 0;
      int x = px + dx;
      int y = py + dy;
      b32[y * pixels_per_scan_line_ + x] = col;
    }
  }
  Flush(px, py, 8, 16);
}

void Sheet::DrawRectWithoutFlush(int px, int py, int w, int h, uint32_t col) {
  if (!buf_)
    return;
  uint32_t* b32 = reinterpret_cast<uint32_t*>(buf_);
  if (w & 1) {
    for (int y = py; y < py + h; y++) {
      RepeatStore4Bytes(w, &b32[y * pixels_per_scan_line_ + px], col);
    }
  } else {
    for (int y = py; y < py + h; y++) {
      RepeatStore8Bytes(w / 2, &b32[y * pixels_per_scan_line_ + px],
                        ((uint64_t)col << 32) | col);
    }
  }
}
void Sheet::DrawRect(int px, int py, int w, int h, uint32_t col) {
  DrawRectWithoutFlush(px, py, w, h, col);
  Flush(px, py, w, h);
}

void Sheet::BlockTransfer(int to_x,
                          int to_y,
                          int from_x,
                          int from_y,
                          int w,
                          int h) {
  uint32_t* b32 = reinterpret_cast<uint32_t*>(buf_);
  if (w & 1) {
    for (int dy = 0; dy < h; dy++) {
      RepeatMove4Bytes(
          w, &b32[(to_y + dy) * pixels_per_scan_line_ + (to_x)],
          &b32[(from_y + dy) * pixels_per_scan_line_ + (from_x + 0)]);
    }
  } else {
    for (int dy = 0; dy < h; dy++) {
      RepeatMove8Bytes(
          w / 2, &b32[(to_y + dy) * pixels_per_scan_line_ + (to_x)],
          &b32[(from_y + dy) * pixels_per_scan_line_ + (from_x + 0)]);
    }
  }
  Flush(to_x, to_y, w, h);
}

void Sheet::Flush(int px, int py, int w, int h) {
  // Assume parent has the same resolution with this sheet
  if (!parent_)
    return;
  if (w & 1) {
    for (int y = py; y < py + h; y++) {
      RepeatMove4Bytes(
          w,
          &reinterpret_cast<uint32_t*>(
              parent_->buf_)[y * pixels_per_scan_line_ + px],
          &reinterpret_cast<uint32_t*>(buf_)[y * pixels_per_scan_line_ + px]);
    }
  } else {
    for (int y = py; y < py + h; y++) {
      RepeatMove8Bytes(
          w / 2,
          &reinterpret_cast<uint32_t*>(
              parent_->buf_)[y * pixels_per_scan_line_ + px],
          &reinterpret_cast<uint32_t*>(buf_)[y * pixels_per_scan_line_ + px]);
    }
  }
}
